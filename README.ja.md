<p align="center">
  <a href="https://voicelala.com/">
    <img src="docs/assets/rvc-cpp-banner.png" alt="RVC.cpp — Native voice conversion by VoiceLala" width="100%">
  </a>
</p>

<h1 align="center">RVC.cpp</h1>
<p align="center"><strong>C++ で RVC スタイルの音声変換をアプリへ。</strong></p>

<p align="center">
  <a href="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml"><img src="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml/badge.svg" alt="Windows CPU build"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-8b5cf6" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-17-3b82f6" alt="C++17">
  <img src="https://img.shields.io/badge/status-experimental-f59e0b" alt="Experimental">
  <a href="https://voicelala.com/"><img src="https://img.shields.io/badge/VoiceLala-visit%20website-06b6d4" alt="Visit VoiceLala"></a>
</p>

[English](README.md) · [简体中文](README.zh-CN.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Español](README.es.md) · [Português](README.pt.md)

---

RVC.cpp は C++17 と ONNX Runtime を利用するローカル音声変換ライブラリです。内容エンコーダー、F0 モデル、RVC スタイルの音声合成モデルを C ABI 経由で利用できます。推論時に Python は不要です。

独立した実装であり、RVC Project の公式 C++ 版でも、Python プロジェクトの全機能を移植したものでもありません。互換性のため、C API、CMake ターゲット、実行ファイルの `dvc` 名は維持しています。詳細な技術文書は現在主に中国語です。

## 主な機能

- **組み込みやすい C ABI**：UTF-8 のモデルパス、明確なエラーコード、FFI 対応。
- **オフライン・ストリーミング**：音声全体の変換と、履歴コンテキスト、SOLA、クロスフェードを使う固定長ブロック処理。
- **音声の調整**：ピッチ、話者 ID、ノイズ強度、乱数シード。
- **独立した状態**：各インスタンスがモデルセッションと推論状態を保持。CMake と WAV 変換例を同梱。

## 処理の流れ

入力音声から内容特徴と F0 を抽出し、音声合成モデルへ渡します。ストリーミングでは履歴、SOLA による位置合わせ、クロスフェードを追加します。テンソルの要件は [モデル仕様](docs/models.md) を参照してください。

## 対応範囲

**0.1.0-dev は実験版です。** Windows x64 CPU が検証対象です。DirectML はオプションのビルド経路ですが、GPU 実行は未検証です。合成モデルによるテストは通過していますが、実際の声の品質、継続的なリアルタイム性能、GPU 動作は検証待ちです。[検証記録](docs/validation.md)。

Linux/macOS、CUDA、学習、`.pth` の直接読み込み、FAISS `.index` 検索、音声デバイス管理には現在対応していません。HTTP、WebSocket、gRPC サーバーは含まれません。モデルは指定の入出力仕様に従う必要があり、任意の RVC エクスポートをそのまま使用できるわけではありません。

## クイックスタート

CMake 3.24 以上、C++17 コンパイラー、`include/` と `lib/` を含む ONNX Runtime SDK が必要です。Visual Studio の x64 開発者ターミナルで実行します。

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

これは CPU ビルドです。既定のテストは実際の音色モデルを読み込みません。詳細なテスト、DirectML、インストールは [ビルド手順](docs/build.md) を参照してください。仕様に合う 3 つのモデルを別途用意します。

```powershell
./build/dvc_wav.exe voice.onnx content.onnx pitch.onnx 40000 768 input.wav output.wav
```

`40000` は音声モデルの出力サンプルレート、`768` は内容特徴の次元です。モデルと一致させてください。入力は最大 30 秒のモノラル PCM16 / float32 WAV、出力は PCM16 WAV です。既存ファイルは上書きしません。既定値は CPU と話者 0 です。

## アプリへの組み込み

`dvc/dvc.h` をインクルードし、既定値関数で設定を初期化してコンテキストを作成し、モデルを読み込みます。`dvc_convert` または `dvc_process` を呼び出し、最後にコンテキストを破棄します。エラーはステータスコードとスレッドローカルの `dvc_last_error` で取得します。同じインスタンスへの呼び出しは直列化してください。`dvc_process` には毎回 `config.block_size` サンプルを渡します。推論はメモリを確保して同期実行されるため、音声コールバックではなくワーカースレッドで実行してください。[API](docs/api.md)。

## ライセンス・貢献

[MIT](LICENSE) ライセンスです。第三者コンポーネントにはそれぞれのライセンスが適用されます。[第三者に関する表記](THIRD_PARTY_NOTICES.md) を参照してください。モデルの重みは別途入手し、それぞれのライセンスに従ってください。貢献方法は [CONTRIBUTING.md](CONTRIBUTING.md) を参照してください。

[RVC Project](https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI) と [ONNX Runtime](https://github.com/microsoft/onnxruntime) に感謝します。

## VoiceLala を体験

ゲーム、配信、ボイスチャットに新しい声と効果音を。[VoiceLala を訪問 →](https://voicelala.com/)

