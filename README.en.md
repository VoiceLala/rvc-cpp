# DVC

**Bring AI voice conversion to your application.**

[VoiceLala](https://voicelala.com/) · [中文](README.md) · [Build](docs/build.md) · [Model contract](docs/models.md) · [API](docs/api.md) · [MIT](LICENSE)

Discover more ways to sound different with **[VoiceLala](https://voicelala.com/)**. Explore AI voice changing and soundboard effects for gaming, streaming and voice chat.

DVC is a C++17 / ONNX Runtime library for developers integrating local voice conversion into their applications. It connects a content encoder, an F0 model and an RVC-style ONNX synthesizer behind a C ABI. Python is not required at inference time.

## Features

- C API with UTF-8 model paths and explicit error reporting.
- Offline mono float32 conversion and fixed-block streaming with context, SOLA alignment and crossfade.
- Pitch shifting, speaker selection, noise scale and reproducible seed configuration.
- Independent model sessions and state for each context.
- CMake integration and a WAV conversion example.

## Compatibility

The current validation target is Windows x64 CPU. DirectML is an optional build path, not yet GPU-validated. Linux/macOS, CUDA, training, `.pth` loading, FAISS `.index` retrieval and audio-device management are not currently supported. Models must match the documented export protocol.

The library provides an in-process C API; no HTTP, WebSocket or gRPC server is included. **0.1.0-dev is experimental.** Synthetic graph tests pass; real-voice quality, sustained real-time performance and GPU execution remain to be validated. See the [validation notes](docs/validation.md).

## Build

Install CMake 3.24+, a C++17 compiler and an ONNX Runtime SDK containing `include/` and `lib/`. In an x64 Visual Studio developer terminal:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DONNXRUNTIME_ROOT=C:/sdk/onnxruntime
cmake --build build
Copy-Item C:/sdk/onnxruntime/lib/onnxruntime.dll build/
ctest --test-dir build --output-on-failure
```

With three compatible, separately obtained models:

```powershell
./build/dvc_wav.exe voice.onnx content.onnx pitch.onnx 40000 768 input.wav output.wav
```

The two numbers specify the voice model's output sample rate and content feature dimension. The example accepts mono PCM16/float32 WAV up to 30 seconds and writes PCM16 without overwriting an existing file. See [model details](docs/models.md); arbitrary RVC ONNX exports are not interchangeable.

Include `dvc/dvc.h`, initialize configurations using the default functions, create a context, load models, call `dvc_convert` or `dvc_process`, and destroy the context. Errors are returned as status codes with thread-local details from `dvc_last_error`. Serialize calls to the same context. Streaming inference belongs on a worker thread, not an audio-device callback.

## License and acknowledgments

DVC is licensed under [MIT](LICENSE). Third-party components retain their own licenses; see [third-party notices](THIRD_PARTY_NOTICES.md). Model weights are obtained separately and remain subject to their respective licenses.

Thanks to [RVC Project](https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI) and [ONNX Runtime](https://github.com/microsoft/onnxruntime). Contribution guidelines are in [CONTRIBUTING.md](CONTRIBUTING.md).

## Explore VoiceLala

Find a voice for your next game, stream or conversation.

**[Visit voicelala.com →](https://voicelala.com/)**
