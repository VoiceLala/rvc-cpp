<p align="center">
  <a href="https://voicelala.com/">
    <img src="docs/assets/rvc-cpp-banner.png" alt="RVC.cpp — Native voice conversion by VoiceLala" width="100%">
  </a>
</p>

<h1 align="center">RVC.cpp</h1>
<p align="center"><strong>RVC-basierte Stimmkonvertierung in C++ für deine Anwendung.</strong></p>

<p align="center">
  <a href="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml"><img src="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml/badge.svg" alt="Windows CPU build"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-8b5cf6" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-17-3b82f6" alt="C++17">
  <img src="https://img.shields.io/badge/status-experimental-f59e0b" alt="Experimental">
  <a href="https://voicelala.com/"><img src="https://img.shields.io/badge/VoiceLala-visit%20website-06b6d4" alt="Visit VoiceLala"></a>
</p>

[English](README.md) · [简体中文](README.zh-CN.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Español](README.es.md) · [Português](README.pt.md)

---

RVC.cpp ist eine Bibliothek für lokale Stimmkonvertierung mit C++17 und ONNX Runtime. Sie verbindet einen Inhaltsencoder, ein F0-Modell und einen Sprachsynthesizer nach dem RVC-Modellvertrag über eine C-ABI. Für die Inferenz wird kein Python benötigt.

Dies ist eine unabhängige Implementierung, keine offizielle C++-Version des RVC Project und keine vollständige Portierung des Python-Projekts. Aus Kompatibilitätsgründen behalten C-API, CMake-Ziele und Beispielprogramme den Namen `dvc`. Die ausführliche technische Dokumentation ist derzeit überwiegend auf Chinesisch.

## Funktionen

- **C-ABI**: UTF-8-Modellpfade, eindeutige Fehlercodes und Integration per FFI.
- **Offline und Streaming**: ganze Audioclips oder feste Blöcke mit Kontext, SOLA-Ausrichtung und Überblendung.
- **Stimme anpassen**: Tonhöhe, Sprecher-ID, Rauschstärke und Zufallsstartwert.
- **Getrennte Instanzen**: eigene Modellsitzungen und Zustände je Kontext. CMake-Integration und WAV-Beispiel enthalten.

## Verarbeitung

Aus dem Eingangsaudio werden Inhaltsmerkmale und F0 extrahiert und an die Stimmsynthese übergeben. Streaming ergänzt den bisherigen Kontext, SOLA und Überblendung. Tensoranforderungen stehen im [Modellvertrag](docs/models.md).

## Kompatibilität

**0.1.0-dev ist experimentell.** Die geprüfte Plattform ist Windows x64 mit CPU. DirectML ist optional kompilierbar, die GPU-Ausführung ist noch nicht validiert. Tests mit synthetischen Modellen bestehen; echte Stimmqualität, dauerhafte Echtzeitleistung und GPU-Betrieb müssen noch geprüft werden. Siehe [Validierung](docs/validation.md).

Linux/macOS, CUDA, Training, direktes Laden von `.pth`, FAISS-`.index`-Suche und Audiogeräteverwaltung werden derzeit nicht unterstützt. HTTP-, WebSocket- und gRPC-Server sind nicht enthalten. Modelle müssen dem dokumentierten Ein-/Ausgabeformat entsprechen; beliebige RVC-Exporte sind nicht austauschbar.

## Schnellstart

Benötigt werden CMake 3.24+, ein C++17-Compiler und ein ONNX-Runtime-SDK mit `include/` und `lib/`. Die Befehle in einer x64-Entwicklerkonsole von Visual Studio ausführen.

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

Dies ist ein CPU-Build. Die Standardtests laden keine echten Stimmmodelle. Vollständige Tests, DirectML und Installation beschreibt die [Bauanleitung](docs/build.md). Drei kompatible Modelle müssen separat beschafft werden.

```powershell
./build/dvc_wav.exe voice.onnx content.onnx pitch.onnx 40000 768 input.wav output.wav
```

`40000` ist die Ausgaberate des Stimmmodells, `768` die Dimension der Inhaltsmerkmale. Beide müssen zum Modell passen. Das Beispiel liest Mono-WAV mit PCM16 / float32 bis 30 Sekunden und schreibt PCM16-WAV, ohne bestehende Dateien zu überschreiben. Standard: CPU und Sprecher 0.

## Integration

`dvc/dvc.h` einbinden, Konfigurationen mit den Standardfunktionen initialisieren, einen Kontext erstellen und Modelle laden. Danach `dvc_convert` oder `dvc_process` aufrufen und den Kontext freigeben. Fehler liefern Statuscodes und das threadlokale `dvc_last_error`. Aufrufe derselben Instanz serialisieren. `dvc_process` erwartet genau `config.block_size` Samples pro Aufruf. Die Inferenz reserviert Speicher und läuft synchron; sie gehört in einen Arbeitsthread statt in den Audiogeräte-Callback. [API](docs/api.md).

## Lizenz und Beiträge

Lizenz: [MIT](LICENSE). Drittanbieterkomponenten behalten ihre jeweiligen Lizenzen; siehe [Hinweise](THIRD_PARTY_NOTICES.md). Modellgewichte werden separat bezogen und unterliegen ihren eigenen Lizenzen. Beiträge: [CONTRIBUTING.md](CONTRIBUTING.md).

Danke an [RVC Project](https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI) und [ONNX Runtime](https://github.com/microsoft/onnxruntime).

## VoiceLala entdecken

Neue Stimmen und Soundeffekte für Spiele, Streams und Sprachchats. [VoiceLala besuchen →](https://voicelala.com/)

