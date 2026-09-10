#include "dvc/dvc.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
static void check(bool ok, const char* what) { if (!ok) throw std::runtime_error(std::string(what) + ": " + dvc_last_error()); }
int main(int argc, char** argv) {
    try {
        check(argc == 2, "fixture directory required");
        const auto root = std::filesystem::u8path(argv[1]);
        auto voice = (root / "voice.onnx").u8string(), content = (root / "content.onnx").u8string(), pitch = (root / "pitch.onnx").u8string();
        dvc_config cfg; dvc_default_config(&cfg);
        cfg.sample_rate = 16000; cfg.block_size = 1600; cfg.context_samples = 1600; cfg.crossfade_samples = 160; cfg.search_samples = 80;
        dvc_context* ptr = nullptr; check(dvc_create(&cfg, &ptr) == DVC_OK, "create");
        std::unique_ptr<dvc_context, decltype(&dvc_destroy)> c(ptr, dvc_destroy);
        dvc_model_config model; dvc_default_model_config(&model);
        model.voice_path = voice.c_str(); model.content_path = content.c_str(); model.pitch_path = pitch.c_str(); model.sample_rate = 16000; model.feature_dimension = 256;
        check(dvc_load_model(c.get(), &model) == DVC_OK, "load fixtures");
        std::vector<float> input(1600, .25f), output(1600), baseline(1600); size_t n = 0;
        check(dvc_convert(c.get(), input.data(), input.size(), output.data(), output.size(), &n) == DVC_OK && n == input.size(), "inference");
        check(std::all_of(output.begin(), output.end(), [](float x) { return std::isfinite(x); }), "finite output");
        check(std::any_of(output.begin(), output.end(), [](float x) { return std::abs(x) > .01f; }), "nonzero fixture");
        baseline = output;
        dvc_params shifted; dvc_default_params(&shifted); shifted.pitch_semitones = 12.f;
        check(dvc_set_params(c.get(), &shifted) == DVC_OK, "set pitch");
        check(dvc_convert(c.get(), input.data(), input.size(), output.data(), output.size(), &n) == DVC_OK, "pitch probe inference");
        for(size_t i = 0; i < output.size(); ++i) check(std::abs(output[i] - 2.f * baseline[i]) < 1e-5f, "pitch parameter reaches ONNX graph");
        dvc_default_params(&shifted); check(dvc_set_params(c.get(), &shifted) == DVC_OK, "restore pitch");
        check(dvc_convert(c.get(), input.data(), input.size(), input.data(), input.size(), &n) == DVC_OK && input == baseline, "in-place conversion");
        auto bad = model; bad.voice_path = content.c_str();
        check(dvc_load_model(c.get(), &bad) == DVC_MODEL_ERROR, "reject wrong contract");
        check(dvc_convert(c.get(), input.data(), input.size(), output.data(), output.size(), &n) == DVC_OK && output == baseline, "failed load preserves old model");
        bad.voice_path = "missing-dvc-test-model.onnx";
        check(dvc_load_model(c.get(), &bad) == DVC_MODEL_ERROR, "missing model");
        dvc_params params; dvc_default_params(&params); params.speaker_id = 1;
        check(dvc_set_params(c.get(), &params) == DVC_INVALID_ARGUMENT, "speaker bound");
        input[0] = std::numeric_limits<float>::quiet_NaN();
        check(dvc_process(c.get(), input.data(), input.size(), output.data(), output.size(), &n) == DVC_INVALID_ARGUMENT && n == 0 && output == baseline, "NaN rejection preserves output");
        input[0] = .25f;
        check(dvc_process(c.get(), input.data(), input.size() - 1, output.data(), output.size(), &n) == DVC_INVALID_ARGUMENT, "stream block contract");
        check(dvc_process(c.get(), input.data(), input.size(), output.data(), output.size(), &n) == DVC_OK, "stream start");
        baseline = output;
        for (int i = 0; i < 12; ++i) check(dvc_process(c.get(), input.data(), input.size(), output.data(), output.size(), &n) == DVC_OK && n == input.size(), "stream continuation");
        check(dvc_reset(c.get()) == DVC_OK, "reset");
        check(dvc_process(c.get(), input.data(), input.size(), output.data(), output.size(), &n) == DVC_OK && output == baseline, "reset reproducibility");
        // A second independent context can run on a second thread.
        std::exception_ptr failure;
        std::thread worker([&] {
            try {
                dvc_context* other = nullptr; check(dvc_create(&cfg, &other) == DVC_OK, "second create");
                std::unique_ptr<dvc_context, decltype(&dvc_destroy)> owned(other, dvc_destroy);
                check(dvc_load_model(other, &model) == DVC_OK, "second load");
                std::vector<float> result(1600); size_t count = 0;
                check(dvc_process(other, input.data(), input.size(), result.data(), result.size(), &count) == DVC_OK && result == baseline, "isolated model state");
            } catch (...) { failure = std::current_exception(); }
        });
        auto main_status = dvc_process(c.get(), input.data(), input.size(), output.data(), output.size(), &n);
        worker.join();
        check(main_status == DVC_OK, "main concurrent instance");
        if (failure) std::rethrow_exception(failure);
        std::cout << "ONNX contract, stream, reset, failure preservation and isolation tests passed\n";
        return 0;
    } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
