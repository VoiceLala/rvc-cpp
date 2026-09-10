<p align="center">
  <a href="https://voicelala.com/">
    <img src="docs/assets/rvc-cpp-banner.png" alt="RVC.cpp — Native voice conversion by VoiceLala" width="100%">
  </a>
</p>

<h1 align="center">RVC.cpp</h1>
<p align="center"><strong>Conversão de voz no estilo RVC em C++ para seus aplicativos.</strong></p>

<p align="center">
  <a href="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml"><img src="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml/badge.svg" alt="Windows CPU build"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-8b5cf6" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-17-3b82f6" alt="C++17">
  <img src="https://img.shields.io/badge/status-experimental-f59e0b" alt="Experimental">
  <a href="https://voicelala.com/"><img src="https://img.shields.io/badge/VoiceLala-visit%20website-06b6d4" alt="Visit VoiceLala"></a>
</p>

[English](README.md) · [简体中文](README.zh-CN.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Español](README.es.md) · [Português](README.pt.md)

---

RVC.cpp é uma biblioteca de conversão de voz local baseada em C++17 e ONNX Runtime. Ela conecta um codificador de conteúdo, um modelo F0 e um sintetizador no estilo RVC por meio de uma ABI C. A inferência não requer Python.

Esta é uma implementação independente, não uma versão oficial em C++ do RVC Project nem uma adaptação completa do projeto Python. Os símbolos C, alvos CMake e executáveis mantêm o nome `dvc` por compatibilidade. A documentação técnica detalhada está atualmente principalmente em chinês.

## Recursos

- **ABI C**: caminhos UTF-8, códigos de erro claros e integração por FFI.
- **Offline e em fluxo**: clipes completos ou blocos fixos com contexto, alinhamento SOLA e crossfade.
- **Ajustes de voz**: tom, ID do falante, intensidade do ruído e semente aleatória.
- **Instâncias independentes**: cada contexto mantém suas sessões e seu estado. Inclui CMake e exemplo de conversão WAV.

## Fluxo de processamento

O conteúdo e a F0 são extraídos do áudio e enviados ao modelo de síntese. O modo em fluxo adiciona contexto anterior, SOLA e crossfade. Veja os tensores exigidos no [contrato dos modelos](docs/models.md).

## Compatibilidade

**0.1.0-dev é experimental.** A plataforma validada é Windows x64 com CPU. DirectML é uma opção de compilação, mas a execução em GPU ainda não foi validada. Os testes com modelos sintéticos passam; a qualidade de vozes reais, o desempenho contínuo em tempo real e a GPU ainda precisam de validação. Veja a [validação](docs/validation.md).

Linux/macOS, CUDA, treinamento, carregamento direto de `.pth`, busca FAISS `.index` e gerenciamento de dispositivos de áudio não são compatíveis atualmente. Não inclui servidores HTTP, WebSocket ou gRPC. Os modelos devem seguir as entradas e saídas documentadas; nem todos os modelos exportados do RVC são intercambiáveis.

## Início rápido

Você precisa de CMake 3.24+, um compilador C++17 e um SDK ONNX Runtime com `include/` e `lib/`. Execute os comandos em um terminal de desenvolvedor x64 do Visual Studio.

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

Esta compilação usa CPU. Os testes padrão não carregam modelos de vozes reais. Para testes completos, DirectML e instalação, veja o [guia de compilação](docs/build.md). Obtenha separadamente três modelos compatíveis.

```powershell
./build/dvc_wav.exe voice.onnx content.onnx pitch.onnx 40000 768 input.wav output.wav
```

`40000` é a taxa de amostragem de saída do modelo de voz e `768` a dimensão das características de conteúdo. Os valores precisam corresponder ao modelo. O exemplo aceita WAV mono PCM16 / float32 de até 30 segundos e grava PCM16 sem sobrescrever arquivos existentes. O padrão é CPU e falante 0.

## Integração

Inclua `dvc/dvc.h`, inicialize as configurações com as funções padrão, crie um contexto e carregue os modelos. Chame `dvc_convert` ou `dvc_process` e libere o contexto ao terminar. Os erros são retornados por códigos de status e por `dvc_last_error`, local à thread. Serialize chamadas à mesma instância. `dvc_process` exige exatamente `config.block_size` amostras por chamada. A inferência aloca memória e executa de forma síncrona; use uma thread de trabalho em vez do callback do dispositivo de áudio. [API](docs/api.md).

## Licença e contribuições

Licença [MIT](LICENSE). Os componentes de terceiros mantêm suas licenças; veja os [avisos](THIRD_PARTY_NOTICES.md). Os pesos dos modelos são obtidos separadamente e seguem suas próprias licenças. Contribuições: [CONTRIBUTING.md](CONTRIBUTING.md).

Agradecimentos ao [RVC Project](https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI) e ao [ONNX Runtime](https://github.com/microsoft/onnxruntime).

## Conheça o VoiceLala

Novas vozes e efeitos sonoros para jogos, transmissões e conversas. [Visite o VoiceLala →](https://voicelala.com/)
