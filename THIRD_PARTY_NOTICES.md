# Third-party notices

## ONNX Runtime

- Upstream: https://github.com/microsoft/onnxruntime
- Validation SDK: official Windows x64 1.20.1 package.
- License: MIT; retained in `licenses/ONNXRuntime-MIT.txt`.
- The SDK's bundled notices are retained in `licenses/ONNXRuntime-1.20.1-ThirdPartyNotices.txt` for that version.

ONNX Runtime is an external dependency and is not bundled in the source archive.
When distributing runtime binaries, include the notices matching that exact
runtime build and comply with its bundled licenses. DirectML distributions may
have additional runtime components with their own terms.

## RVC

- Upstream: https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI
- License: https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI/blob/main/LICENSE
- Copyright and MIT notice: `licenses/RVC-MIT.txt`.

DVC implements a limited RVC-style ONNX inference protocol. The Python WebUI,
training scripts, pretrained models and retrieval indexes are not bundled.

## Models and test assets

Model weights and recordings are obtained separately and are subject to their
respective licenses. Synthetic ONNX test graphs are generated from
`tests/make_fixtures.py`; they are not pretrained voice models.

DVC's own source is covered by the root `LICENSE`. References to third-party
projects do not imply endorsement.
