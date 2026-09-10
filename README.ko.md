<p align="center">
  <a href="https://voicelala.com/">
    <img src="docs/assets/rvc-cpp-banner.png" alt="RVC.cpp — Native voice conversion by VoiceLala" width="100%">
  </a>
</p>

<h1 align="center">RVC.cpp</h1>
<p align="center"><strong>C++로 RVC 스타일 음성 변환을 앱에 통합하세요.</strong></p>

<p align="center">
  <a href="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml"><img src="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml/badge.svg" alt="Windows CPU build"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-8b5cf6" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-17-3b82f6" alt="C++17">
  <img src="https://img.shields.io/badge/status-experimental-f59e0b" alt="Experimental">
  <a href="https://voicelala.com/"><img src="https://img.shields.io/badge/VoiceLala-visit%20website-06b6d4" alt="Visit VoiceLala"></a>
</p>

[English](README.md) · [简体中文](README.zh-CN.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Español](README.es.md) · [Português](README.pt.md)

---

RVC.cpp는 C++17과 ONNX Runtime 기반의 로컬 음성 변환 라이브러리입니다. 콘텐츠 인코더, F0 모델, RVC 스타일 음성 합성 모델을 C ABI로 연결합니다. 추론 실행에는 Python이 필요하지 않습니다.

독립 구현이며 RVC Project의 공식 C++ 버전이나 Python 프로젝트의 모든 기능을 이식한 버전이 아닙니다. 호환성을 위해 C API, CMake 타깃, 예제 실행 파일의 `dvc` 이름을 유지합니다. 상세 기술 문서는 현재 주로 중국어로 제공됩니다.

## 주요 기능

- **C ABI 통합**: UTF-8 모델 경로, 명확한 오류 코드, FFI 지원.
- **오프라인 및 스트리밍**: 전체 오디오 변환과 이전 컨텍스트, SOLA 정렬, 크로스페이드를 사용하는 고정 블록 처리.
- **음성 조절**: 음높이, 화자 ID, 노이즈 강도, 난수 시드 설정.
- **독립 인스턴스**: 각 컨텍스트가 모델 세션과 추론 상태를 관리합니다. CMake 및 WAV 변환 예제를 제공합니다.

## 처리 흐름

입력 오디오에서 콘텐츠 특징과 F0를 추출해 음성 합성 모델에 전달합니다. 스트리밍에는 이전 컨텍스트, SOLA 정렬, 크로스페이드가 추가됩니다. 텐서 요구 사항은 [모델 규약](docs/models.md)을 참고하세요.

## 지원 범위

**0.1.0-dev는 실험 버전입니다.** 검증 기준은 Windows x64 CPU입니다. DirectML은 선택적 빌드 경로이며 GPU 실행은 아직 검증하지 않았습니다. 합성 모델 테스트는 통과했지만 실제 음색 품질, 지속적인 실시간 성능, GPU 동작은 추가 검증이 필요합니다. [검증 기록](docs/validation.md).

Linux/macOS, CUDA, 학습, `.pth` 직접 로딩, FAISS `.index` 검색, 오디오 장치 관리는 현재 지원하지 않습니다. HTTP, WebSocket, gRPC 서버는 포함하지 않습니다. 모델은 문서의 입출력 규약을 따라야 하며 모든 RVC 내보내기 형식과 호환되지는 않습니다.

## 빠른 시작

CMake 3.24 이상, C++17 컴파일러, `include/`와 `lib/`가 포함된 ONNX Runtime SDK가 필요합니다. Visual Studio x64 개발자 터미널에서 실행하세요.

```sh
git clone https://github.com/VoiceLala/rvc-cpp.git
cd rvc-cpp
```

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DONNXRUNTIME_ROOT=C:/sdk/onnxruntime
cmake --build build
Copy-Item C:/sdk/onnxruntime/lib/onnxruntime.dll build/
ctest --test-dir build --output-on-failure
```

CPU 빌드입니다. 기본 테스트는 실제 음색 모델을 로드하지 않습니다. 전체 테스트, DirectML, 설치 방법은 [빌드 안내](docs/build.md)를 참고하세요. 규약에 맞는 모델 3개를 별도로 준비합니다.

```powershell
./build/dvc_wav.exe voice.onnx content.onnx pitch.onnx 40000 768 input.wav output.wav
```

`40000`은 음성 모델 출력 샘플 레이트이고 `768`은 콘텐츠 특징 차원입니다. 모델과 일치해야 합니다. 최대 30초의 모노 PCM16 / float32 WAV를 입력받아 PCM16 WAV로 출력합니다. 기존 출력 파일은 덮어쓰지 않습니다. 기본값은 CPU와 화자 0입니다.

## 앱에 통합하기

`dvc/dvc.h`를 포함하고 기본값 함수로 설정을 초기화한 뒤 컨텍스트를 만들고 모델을 로드하세요. `dvc_convert` 또는 `dvc_process` 호출 후 컨텍스트를 해제합니다. 오류는 상태 코드와 스레드 로컬 `dvc_last_error`로 확인합니다. 같은 인스턴스의 호출은 직렬화해야 합니다. `dvc_process`에는 매번 정확히 `config.block_size`개의 샘플을 전달하세요. 추론은 메모리를 할당하고 동기 실행하므로 오디오 콜백이 아닌 작업 스레드에서 실행하세요. [API](docs/api.md).

## 라이선스 및 기여

[MIT](LICENSE) 라이선스입니다. 타사 구성 요소에는 각자의 라이선스가 적용됩니다. [타사 고지](THIRD_PARTY_NOTICES.md)를 확인하세요. 모델 가중치는 별도로 구하고 해당 라이선스를 따라야 합니다. 기여 방법은 [CONTRIBUTING.md](CONTRIBUTING.md)를 참고하세요.

[RVC Project](https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI)와 [ONNX Runtime](https://github.com/microsoft/onnxruntime)에 감사드립니다.

## VoiceLala 체험하기

게임, 방송, 음성 채팅에 새로운 목소리와 효과음을 더하세요. [VoiceLala 방문 →](https://voicelala.com/)
