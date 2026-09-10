#include "dvc/dvc.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
uint16_t u16(const unsigned char* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
uint32_t u32(const unsigned char* p) { return p[0] | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
void put16(std::ostream& f, uint16_t n) { const char p[] = {char(n), char(n >> 8)}; f.write(p, 2); }
void put32(std::ostream& f, uint32_t n) { put16(f, uint16_t(n)); put16(f, uint16_t(n >> 16)); }
struct wave { uint32_t rate; std::vector<float> samples; };
wave read(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f || f.tellg() < 12 || f.tellg() > 32 * 1024 * 1024) throw std::runtime_error("Expected a WAV file smaller than 32 MiB");
    std::vector<unsigned char> bytes(static_cast<size_t>(f.tellg())); f.seekg(0); f.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!f || std::memcmp(bytes.data(), "RIFF", 4) || std::memcmp(bytes.data() + 8, "WAVE", 4)) throw std::runtime_error("Expected RIFF/WAVE");
    uint16_t format = 0, channels = 0, bits = 0, align = 0; uint32_t rate = 0;
    size_t offset = 0, length = 0;
    const size_t riff_end = static_cast<size_t>(u32(bytes.data() + 4)) + 8;
    if (riff_end > bytes.size() || riff_end < 12) throw std::runtime_error("Truncated RIFF");
    for (size_t at = 12; at + 8 <= riff_end;) {
        const size_t n = u32(bytes.data() + at + 4);
        if (n > riff_end - at - 8) throw std::runtime_error("Truncated WAV chunk");
        const auto* p = bytes.data() + at + 8;
        if (!std::memcmp(bytes.data() + at, "fmt ", 4)) {
            if (n < 16) throw std::runtime_error("Invalid fmt chunk");
            format = u16(p); channels = u16(p + 2); rate = u32(p + 4); align = u16(p + 12); bits = u16(p + 14);
        } else if (!std::memcmp(bytes.data() + at, "data", 4)) { offset = at + 8; length = n; }
        at += 8 + n + (n & 1);
    }
    if (channels != 1 || !((format == 1 && bits == 16) || (format == 3 && bits == 32)) || align != bits / 8 || !length || length % align) throw std::runtime_error("Use mono PCM16 or IEEE float32 WAV");
    if (rate < 8000 || rate > 96000 || length / align > static_cast<size_t>(rate) * 30) throw std::runtime_error("Use 8..96 kHz audio, at most 30 seconds");
    wave result{rate, std::vector<float>(length / align)};
    for (size_t i = 0; i < result.samples.size(); ++i) {
        auto* p = bytes.data() + offset + i * align;
        if (format == 1) { const auto v = u16(p); result.samples[i] = static_cast<float>(v >= 32768 ? int(v) - 65536 : int(v)) / 32768.f; }
        else { const uint32_t v = u32(p); std::memcpy(&result.samples[i], &v, 4); }
    }
    return result;
}
void write(const std::filesystem::path& path, const wave& audio) {
    std::ofstream f(path, std::ios::binary); if (!f) throw std::runtime_error("Cannot create output");
    const auto bytes = static_cast<uint32_t>(audio.samples.size() * 2);
    f.write("RIFF", 4); put32(f, 36 + bytes); f.write("WAVEfmt ", 8); put32(f, 16);
    put16(f, 1); put16(f, 1); put32(f, audio.rate); put32(f, audio.rate * 2); put16(f, 2); put16(f, 16);
    f.write("data", 4); put32(f, bytes);
    for(float x : audio.samples) put16(f, static_cast<uint16_t>(static_cast<int16_t>(std::lround(std::clamp(x, -1.f, 1.f) * 32767))));
    if (!f) throw std::runtime_error("Output write failed");
}
void check(dvc_status status) { if(status != DVC_OK) throw std::runtime_error(dvc_last_error()); }
}
int main(int argc, char** argv) {
    if (argc != 8) {
        std::cerr << "Usage: dvc_wav voice.onnx content.onnx pitch.onnx model_rate feature_dim input.wav output.wav\n"
                     "Example: dvc_wav voice.onnx content.onnx pitch.onnx 40000 768 input.wav output.wav\n";
        return 2;
    }
    try {
        auto input_path = std::filesystem::u8path(argv[6]), output_path = std::filesystem::u8path(argv[7]);
        if(std::filesystem::exists(output_path)) throw std::runtime_error("Output already exists; choose a new file");
        auto audio = read(input_path);
        dvc_config config; dvc_default_config(&config); config.sample_rate = audio.rate;
        config.block_size = audio.rate / 10; config.context_samples = audio.rate / 2;
        config.crossfade_samples = audio.rate / 100; config.search_samples = audio.rate / 200;
        dvc_context* handle = nullptr; check(dvc_create(&config, &handle));
        std::unique_ptr<dvc_context, decltype(&dvc_destroy)> context(handle, dvc_destroy);
        dvc_model_config model; dvc_default_model_config(&model);
        model.voice_path = argv[1]; model.content_path = argv[2]; model.pitch_path = argv[3];
        model.sample_rate = static_cast<uint32_t>(std::stoul(argv[4])); model.feature_dimension = static_cast<uint32_t>(std::stoul(argv[5]));
        check(dvc_load_model(handle, &model));
        std::vector<float> output(audio.samples.size()); size_t written = 0;
        check(dvc_convert(handle, audio.samples.data(), audio.samples.size(), output.data(), output.size(), &written));
        audio.samples.swap(output); write(output_path, audio);
        std::cout << "Wrote " << written << " samples at " << audio.rate << " Hz\n";
        return 0;
    } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
