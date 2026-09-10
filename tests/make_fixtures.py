"""Generate tiny synthetic ONNX graphs without downloading models or packages.

Only protobuf fields required by ONNX ModelProto are encoded. These graphs emit
constants; they verify ABI and graph plumbing, NOT real RVC quality or timing.
Schema: https://github.com/onnx/onnx/blob/main/onnx/onnx.proto
"""
import math
import pathlib
import struct
import sys


def varint(n):
    data = bytearray()
    while n > 127:
        data.append((n & 127) | 128)
        n >>= 7
    data.append(n)
    return bytes(data)


def number(field, value):
    return varint(field << 3) + varint(value)


def blob(field, value):
    if isinstance(value, str):
        value = value.encode("utf-8")
    return varint((field << 3) | 2) + varint(len(value)) + value


def value_info(name, dtype, dimensions):
    shape = b"".join(blob(1, number(1, d) if isinstance(d, int) else blob(2, d)) for d in dimensions)
    return blob(1, name) + blob(2, blob(1, number(1, dtype) + blob(2, shape)))


def constant(name, dims, values):
    tensor = b"".join(number(1, d) for d in dims) + number(2, 1)
    tensor += blob(9, struct.pack("<" + "f" * len(values), *values))
    attribute = blob(1, "value") + blob(5, tensor) + number(20, 4)
    return blob(2, name) + blob(4, "Constant") + blob(5, attribute)


def model(path, inputs, outputs, pitch_probe=False):
    graph = blob(2, "dvc_synthetic_contract_test")
    for name, dims, values in outputs:
        if pitch_probe:
            graph += blob(1, constant("carrier", dims, [v / 220. for v in values]))
            axes = blob(1, "axes") + number(8, 1) + number(20, 7)
            mean = blob(1, "pitchf") + blob(2, "mean_pitch") + blob(4, "ReduceMean") + blob(5, axes)
            multiply = blob(1, "mean_pitch") + blob(1, "carrier") + blob(2, name) + blob(4, "Mul")
            graph += blob(1, mean) + blob(1, multiply)
        else:
            graph += blob(1, constant(name, dims, values))
        graph += blob(12, value_info(name, 1, dims))
    for name, dtype, dims in inputs:
        graph += blob(11, value_info(name, dtype, dims))
    path.write_bytes(number(1, 8) + blob(2, "dvc-tests") + blob(7, graph) + blob(8, number(2, 13)))


def main():
    root = pathlib.Path(sys.argv[1])
    root.mkdir(parents=True, exist_ok=True)
    model(root / "content.onnx", [("source", 1, [1, 1, "samples"])], [("embed", [1, 4, 256], [0.1] * 1024)])
    model(root / "pitch.onnx", [("waveform", 1, [1, "samples"]), ("threshold", 1, [1])], [("f0", [1, 8], [220.] * 8), ("uv", [1, 8], [1.] * 8)])
    model(root / "voice.onnx", [
        ("phone", 1, [1, "frames", 256]), ("phone_lengths", 7, [1]),
        ("pitch", 7, [1, "frames"]), ("pitchf", 1, [1, "frames"]),
        ("ds", 7, [1]), ("rnd", 1, [1, 192, "frames"]),
    ], [("audio", [1, 1, 6400], [0.2 * math.sin(i * 2 * math.pi * 220 / 16000) for i in range(6400)])], pitch_probe=True)
    print(f"Synthetic fixtures written to {root}")


if __name__ == "__main__":
    main()
