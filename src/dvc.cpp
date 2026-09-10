#include "dvc/dvc.h"
#include <onnxruntime_cxx_api.h>
#if defined(DVC_ENABLE_DIRECTML)
#include <dml_provider_factory.h>
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
thread_local std::array<char, 2048> error_text{};
struct failure : std::runtime_error {
    dvc_status status;
    failure(dvc_status s, const char* message) : std::runtime_error(message), status(s) {}
};
void require(bool ok, const char* message, dvc_status status = DVC_INVALID_ARGUMENT) {
    if (!ok) throw failure(status, message);
}
template<class F> dvc_status guarded(F&& f) noexcept {
    error_text[0] = '\0';
    try { f(); return DVC_OK; }
    catch (const failure& e) { std::snprintf(error_text.data(), error_text.size(), "%s", e.what()); return e.status; }
    catch (const Ort::Exception& e) { std::snprintf(error_text.data(), error_text.size(), "%s", e.what()); return DVC_MODEL_ERROR; }
    catch (const std::exception& e) { std::snprintf(error_text.data(), error_text.size(), "%s", e.what()); return DVC_RUNTIME_ERROR; }
    catch (...) { std::snprintf(error_text.data(), error_text.size(), "Unknown native exception"); return DVC_RUNTIME_ERROR; }
}
// Windowed-sinc resampling with no additional DSP dependency.
std::vector<float> resample(const std::vector<float>& in, uint32_t from, uint32_t to) {
    if (from == to) return in;
    const size_t count = static_cast<size_t>(std::llround(static_cast<double>(in.size()) * to / from));
    std::vector<float> out(count);
    constexpr double pi = 3.14159265358979323846;
    constexpr int radius = 24;
    const double cutoff = std::min(1.0, static_cast<double>(to) / from) * 0.94;
    for (size_t i = 0; i < count; ++i) {
        const double position = static_cast<double>(i) * from / to;
        const auto center = static_cast<int64_t>(position);
        double sum = 0, weights = 0;
        for (int tap = -radius + 1; tap <= radius; ++tap) {
            const int64_t at = center + tap;
            const double distance = position - static_cast<double>(at);
            if (std::abs(distance) >= radius) continue;
            const double x = pi * distance * cutoff;
            const double sinc = std::abs(x) < 1e-12 ? 1.0 : std::sin(x) / x;
            const double weight = cutoff * sinc * (0.5 + 0.5 * std::cos(pi * distance / radius));
            const auto index = static_cast<size_t>(std::clamp<int64_t>(at, 0, static_cast<int64_t>(in.size()) - 1));
            sum += in[index] * weight;
            weights += weight;
        }
        out[i] = static_cast<float>(sum / weights);
    }
    return out;
}
std::vector<int64_t> shape(const Ort::Value& tensor, ONNXTensorElementDataType type) {
    require(tensor.IsTensor(), "Expected a tensor", DVC_MODEL_ERROR);
    const auto info = tensor.GetTensorTypeAndShapeInfo();
    require(info.GetElementType() == type, "Unexpected tensor element type", DVC_MODEL_ERROR);
    const auto result = info.GetShape();
    require(info.GetElementCount() > 0, "Empty model output", DVC_MODEL_ERROR);
    return result;
}
void finite(const float* data, size_t n, dvc_status status) {
    require(std::all_of(data, data + n, [](float v) { return std::isfinite(v); }), "PCM/tensor contains NaN or infinity", status);
}
struct port { const char* name; ONNXTensorElementDataType type; size_t rank; };
void contract(Ort::Session& s, bool input, std::initializer_list<port> ports) {
    Ort::AllocatorWithDefaultOptions allocator;
    require((input ? s.GetInputCount() : s.GetOutputCount()) == ports.size(), "Unexpected model input/output count; see docs/models.md", DVC_MODEL_ERROR);
    for (const auto& p : ports) {
        bool found = false;
        for (size_t i = 0; i < ports.size(); ++i) {
            auto name = input ? s.GetInputNameAllocated(i, allocator) : s.GetOutputNameAllocated(i, allocator);
            if (std::strcmp(name.get(), p.name) != 0) continue;
            auto type = input ? s.GetInputTypeInfo(i) : s.GetOutputTypeInfo(i);
            auto info = type.GetTensorTypeAndShapeInfo();
            require(info.GetElementType() == p.type && info.GetShape().size() == p.rank, "Model tensor type/rank mismatch; see docs/models.md", DVC_MODEL_ERROR);
            found = true;
        }
        require(found, "Model tensor name mismatch; see docs/models.md", DVC_MODEL_ERROR);
    }
}
struct models {
    Ort::Session voice, content, pitch;
    uint32_t rate, dimension, speakers;
    models(Ort::Env& env, Ort::SessionOptions& options, const dvc_model_config& c)
        : voice(env, std::filesystem::u8path(c.voice_path).c_str(), options),
          content(env, std::filesystem::u8path(c.content_path).c_str(), options),
          pitch(env, std::filesystem::u8path(c.pitch_path).c_str(), options),
          rate(c.sample_rate), dimension(c.feature_dimension), speakers(c.speaker_count) {
        constexpr auto f = ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
        constexpr auto i = ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64;
        contract(content, true, {{"source", f, 3}});
        contract(content, false, {{"embed", f, 3}});
        contract(pitch, true, {{"waveform", f, 2}, {"threshold", f, 1}});
        contract(pitch, false, {{"f0", f, 2}, {"uv", f, 2}});
        contract(voice, true, {{"phone", f, 3}, {"phone_lengths", i, 1}, {"pitch", i, 2}, {"pitchf", f, 2}, {"ds", i, 1}, {"rnd", f, 3}});
        contract(voice, false, {{"audio", f, 3}});
    }
};
}
struct dvc_context {
    dvc_config config;
    dvc_params params{};
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "dvc"};
    Ort::SessionOptions options;
    Ort::MemoryInfo memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::unique_ptr<models> model;
    std::mt19937_64 random;
    std::vector<float> history, overlap;
    explicit dvc_context(const dvc_config& c) : config(c) {
        dvc_default_params(&params);
        env.DisableTelemetryEvents();
        options.SetIntraOpNumThreads(static_cast<int>(c.threads));
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        if (c.provider == DVC_DIRECTML) {
#if defined(DVC_ENABLE_DIRECTML)
            const OrtDmlApi* api = nullptr;
            Ort::ThrowOnError(Ort::GetApi().GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&api)));
            options.DisableMemPattern();
            options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
            Ort::ThrowOnError(api->SessionOptionsAppendExecutionProvider_DML(options, static_cast<int>(c.device_id)));
#else
            throw failure(DVC_UNSUPPORTED, "This build does not enable DirectML");
#endif
        }
        history.resize(static_cast<size_t>(c.context_samples) + c.block_size + c.crossfade_samples + c.search_samples);
        overlap.resize(c.crossfade_samples);
        reset();
    }
    void reset() { std::fill(history.begin(), history.end(), 0.f); std::fill(overlap.begin(), overlap.end(), 0.f); random.seed(params.seed); }
    std::vector<float> infer(const std::vector<float>& input, std::mt19937_64& rng) {
        auto audio = resample(input, config.sample_rate, 16000);
        const int64_t content_shape[] = {1, 1, static_cast<int64_t>(audio.size())};
        auto source = Ort::Value::CreateTensor<float>(memory, audio.data(), audio.size(), content_shape, 3);
        const char* content_in[] = {"source"}; const char* content_out[] = {"embed"};
        auto embedding = model->content.Run(Ort::RunOptions{nullptr}, content_in, &source, 1, content_out, 1);
        const auto dimensions = shape(embedding[0], ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT);
        require(dimensions.size() == 3 && dimensions[0] == 1 && dimensions[1] > 0 && dimensions[1] <= 10000 && dimensions[2] == model->dimension, "Invalid content output shape", DVC_MODEL_ERROR);
        const int64_t frames = dimensions[1] * 2;
        const size_t width = model->dimension;
        const float* data = embedding[0].GetTensorData<float>();
        finite(data, static_cast<size_t>(dimensions[1]) * width, DVC_MODEL_ERROR);
        std::vector<float> features(static_cast<size_t>(frames) * width);
        for (size_t t = 0; t < static_cast<size_t>(frames); ++t) std::copy_n(data + (t / 2) * width, width, features.data() + t * width);
        int64_t pitch_shape[] = {1, static_cast<int64_t>(audio.size())}, scalar_shape[] = {1};
        float threshold = params.pitch_threshold;
        std::array<Ort::Value, 2> pitch_inputs{
            Ort::Value::CreateTensor<float>(memory, audio.data(), audio.size(), pitch_shape, 2),
            Ort::Value::CreateTensor<float>(memory, &threshold, 1, scalar_shape, 1)};
        const char* pitch_in[] = {"waveform", "threshold"}; const char* pitch_out[] = {"f0", "uv"};
        auto pitch = model->pitch.Run(Ort::RunOptions{nullptr}, pitch_in, pitch_inputs.data(), 2, pitch_out, 2);
        auto ps = shape(pitch[0], ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT);
        require(ps.size() == 2 && ps[0] == 1 && ps[1] > 0, "Invalid F0 output shape", DVC_MODEL_ERROR);
        const auto pitch_count = static_cast<size_t>(ps[1]);
        const float* hz = pitch[0].GetTensorData<float>();
        finite(hz, pitch_count, DVC_MODEL_ERROR);
        std::vector<float> f0(static_cast<size_t>(frames));
        std::vector<int64_t> coarse(f0.size());
        const double factor = std::pow(2.0, params.pitch_semitones / 12.0);
        const double mel_min = 1127.0 * std::log1p(50.0 / 700.0), mel_max = 1127.0 * std::log1p(1100.0 / 700.0);
        for (size_t t = 0; t < f0.size(); ++t) {
            const double position = f0.size() == 1 ? 0 : static_cast<double>(t) * (pitch_count - 1) / (f0.size() - 1);
            const size_t left = static_cast<size_t>(position), right = std::min(left + 1, pitch_count - 1);
            const double frac = position - static_cast<double>(left);
            // Preserve unvoiced frames at voiced/unvoiced boundaries.
            double value = hz[left] <= 0 || hz[right] <= 0 ? hz[frac < 0.5 ? left : right] : hz[left] * (1.0 - frac) + hz[right] * frac;
            f0[t] = static_cast<float>(std::clamp(value * factor, 0.0, 24000.0));
            const double mel = 1127.0 * std::log1p(f0[t] / 700.0);
            coarse[t] = static_cast<int64_t>(std::llround(std::clamp((mel - mel_min) * 254.0 / (mel_max - mel_min) + 1.0, 1.0, 255.0)));
        }
        std::vector<float> noise(static_cast<size_t>(frames) * 192);
        std::normal_distribution<float> distribution(0.f, 1.f);
        for (auto& value : noise) value = distribution(rng) * params.noise_scale;
        int64_t feature_shape[] = {1, frames, static_cast<int64_t>(width)}, frame_shape[] = {1, frames}, noise_shape[] = {1, 192, frames};
        int64_t length = frames, speaker = params.speaker_id;
        std::array<Ort::Value, 6> inputs{
            Ort::Value::CreateTensor<float>(memory, features.data(), features.size(), feature_shape, 3),
            Ort::Value::CreateTensor<int64_t>(memory, &length, 1, scalar_shape, 1),
            Ort::Value::CreateTensor<int64_t>(memory, coarse.data(), coarse.size(), frame_shape, 2),
            Ort::Value::CreateTensor<float>(memory, f0.data(), f0.size(), frame_shape, 2),
            Ort::Value::CreateTensor<int64_t>(memory, &speaker, 1, scalar_shape, 1),
            Ort::Value::CreateTensor<float>(memory, noise.data(), noise.size(), noise_shape, 3)};
        const char* names[] = {"phone", "phone_lengths", "pitch", "pitchf", "ds", "rnd"}; const char* outputs[] = {"audio"};
        auto result = model->voice.Run(Ort::RunOptions{nullptr}, names, inputs.data(), inputs.size(), outputs, 1);
        auto rs = shape(result[0], ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT);
        require(rs.size() == 3 && rs[0] == 1 && rs[1] == 1 && rs[2] > 0 && rs[2] <= static_cast<int64_t>(model->rate) * 32, "Invalid voice output shape", DVC_MODEL_ERROR);
        const auto n = static_cast<size_t>(rs[2]);
        const float* output = result[0].GetTensorData<float>();
        finite(output, n, DVC_MODEL_ERROR);
        auto converted = resample(std::vector<float>(output, output + n), model->rate, config.sample_rate);
        // Exporters can produce a partial final frame. Trim or zero-pad to caller's duration.
        converted.resize(input.size(), 0.f);
        return converted;
    }
};
extern "C" {
const char* dvc_version() { return "0.1.0-dev"; }
const char* dvc_last_error() { return error_text.data(); }
void dvc_default_config(dvc_config* c) { if(c) *c = {sizeof(*c), DVC_ABI_VERSION, 48000, 4800, 24000, 480, 240, DVC_CPU, 0, 0}; }
void dvc_default_model_config(dvc_model_config* c) { if(c) *c = {sizeof(*c), nullptr, nullptr, nullptr, 40000, 768, 1}; }
void dvc_default_params(dvc_params* p) { if(p) *p = {sizeof(*p), 0.f, 0.3f, 0.03f, 0, 114514}; }
dvc_status dvc_create(const dvc_config* c, dvc_context** out) {
    if (out) *out = nullptr;
    return guarded([&] {
        require(c && out, "config/out must not be null");
        require(c->struct_size == sizeof(*c) && c->abi_version == DVC_ABI_VERSION, "ABI/config size mismatch");
        require(c->sample_rate >= 8000 && c->sample_rate <= 96000, "Sample rate must be 8000..96000");
        require(c->block_size >= c->sample_rate / 50 && c->block_size <= c->sample_rate, "Block must be 20..1000 ms");
        require(c->context_samples <= c->sample_rate * 2 && c->crossfade_samples <= c->block_size && c->search_samples <= c->block_size / 2, "Invalid context/crossfade/search size");
        require(c->crossfade_samples > 0 || c->search_samples == 0, "SOLA search requires crossfade");
        require(c->threads <= 256 && c->device_id <= 255 && c->provider <= DVC_DIRECTML, "Invalid execution provider options");
        *out = new dvc_context(*c);
    });
}
void dvc_destroy(dvc_context* c) { delete c; }
dvc_status dvc_load_model(dvc_context* c, const dvc_model_config* m) {
    return guarded([&] {
        require(c && m && m->struct_size == sizeof(*m), "Invalid model config");
        require(m->voice_path && *m->voice_path && m->content_path && *m->content_path && m->pitch_path && *m->pitch_path, "Three ONNX model paths are required");
        require(m->sample_rate >= 8000 && m->sample_rate <= 96000 && (m->feature_dimension == 256 || m->feature_dimension == 768) && m->speaker_count > 0 && m->speaker_count <= 65536, "Invalid model metadata");
        require(c->params.speaker_id < m->speaker_count, "Current speaker id is outside the new model range");
        auto candidate = std::make_unique<models>(c->env, c->options, *m);
        c->model = std::move(candidate); c->reset();
    });
}
dvc_status dvc_set_params(dvc_context* c, const dvc_params* p) {
    return guarded([&] {
        require(c && p && p->struct_size == sizeof(*p), "Invalid parameters");
        require(std::isfinite(p->pitch_semitones) && std::abs(p->pitch_semitones) <= 48 && std::isfinite(p->noise_scale) && p->noise_scale >= 0 && p->noise_scale <= 2 && std::isfinite(p->pitch_threshold) && p->pitch_threshold >= 0 && p->pitch_threshold <= 1, "Invalid pitch/noise/threshold");
        require(p->speaker_id < (c->model ? c->model->speakers : 65536u), "Speaker id outside model range");
        if(c->params.seed != p->seed) c->random.seed(p->seed);
        c->params = *p;
    });
}
dvc_status dvc_reset(dvc_context* c) { return guarded([&] { require(c, "Null context"); c->reset(); }); }
static void check_io(dvc_context* c, const float* in, size_t count, float* out, size_t capacity, size_t* written) {
    require(c && in && out && written, "Null context or audio buffer");
    require(count >= c->config.sample_rate / 50 && count <= static_cast<size_t>(c->config.sample_rate) * 30, "Input duration must be 20 ms..30 sec");
    require(capacity >= count, "Output capacity must be at least input_count", DVC_BUFFER_TOO_SMALL);
    require(c->model != nullptr, "Load the three models first", DVC_NOT_LOADED);
    finite(in, count, DVC_INVALID_ARGUMENT);
}
dvc_status dvc_convert(dvc_context* c, const float* in, size_t count, float* out, size_t capacity, size_t* written) {
    if(written) *written = 0;
    return guarded([&] {
        check_io(c, in, count, out, capacity, written);
        std::mt19937_64 rng(c->params.seed);
        auto result = c->infer(std::vector<float>(in, in + count), rng);
        std::copy(result.begin(), result.end(), out); *written = result.size();
    });
}
dvc_status dvc_process(dvc_context* c, const float* in, size_t count, float* out, size_t capacity, size_t* written) {
    if(written) *written = 0;
    return guarded([&] {
        check_io(c, in, count, out, capacity, written);
        require(count == c->config.block_size, "Streaming requires exactly block_size samples");
        auto history = c->history; auto rng = c->random;
        std::move(history.begin() + count, history.end(), history.begin());
        std::copy_n(in, count, history.end() - count);
        auto audio = c->infer(history, rng);
        const size_t start = c->config.context_samples, cross = c->config.crossfade_samples;
        size_t offset = 0; double best = -std::numeric_limits<double>::infinity();
        for (size_t shift = 0; cross && shift <= c->config.search_samples; ++shift) {
            double dot = 0, energy = 1e-12;
            for(size_t j = 0; j < cross; ++j) { const double v = audio[start + shift + j]; dot += v * c->overlap[j]; energy += v * v; }
            const double score = dot / std::sqrt(energy);
            if(score > best) { best = score; offset = shift; }
        }
        std::vector<float> next_overlap(audio.begin() + start + offset + count, audio.begin() + start + offset + count + cross);
        for(size_t j = 0; j < cross; ++j) {
            const float weight = static_cast<float>(j + 1) / static_cast<float>(cross + 1);
            audio[start + offset + j] = audio[start + offset + j] * weight + c->overlap[j] * (1.f - weight);
        }
        std::copy_n(audio.data() + start + offset, count, out);
        c->history.swap(history); c->overlap.swap(next_overlap); c->random = rng; *written = count;
    });
}
}
