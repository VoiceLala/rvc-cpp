# Changelog

Project renamed to **RVC.cpp** (`VoiceLala/rvc-cpp`). English is now the default README, with Chinese, Japanese, Korean, German, French, Spanish and Portuguese translations. Existing `dvc` API and build target names remain compatible.

## 0.1.0-dev — 2026-09-10

- Prepared standalone C++17 / ONNX Runtime library and C ABI.
- Added explicit content/F0/voice model loading, CPU and optional DirectML path.
- Added independent clip conversion and fixed-block history/SOLA/crossfade.
- Added isolated model/RNG state, argument checks and non-fallback error handling.
- Added WAV CLI, synthetic ONNX contract tests, C ABI and CMake consumer examples.
- Added source packaging, documentation and VoiceLala website links.
- Released source under MIT with third-party notices.
- Real-voice acceptance and GPU validation remain pending.
