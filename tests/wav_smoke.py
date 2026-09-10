"""End-to-end CLI checks using generated models and artificial PCM only."""
import pathlib
import struct
import subprocess
import sys
import tempfile
import wave


def main():
    exe = pathlib.Path(sys.argv[1]).resolve()
    models = pathlib.Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory(prefix="dvc-wav-", dir=exe.parent) as temporary:
        root = pathlib.Path(temporary)
        source, output = root / "input.wav", root / "output.wav"
        with wave.open(str(source), "wb") as audio:
            audio.setparams((1, 2, 16000, 1600, "NONE", "not compressed"))
            audio.writeframes(struct.pack("<1600h", *([1000] * 1600)))
        args = [str(exe), str(models / "voice.onnx"), str(models / "content.onnx"), str(models / "pitch.onnx"), "16000", "256", str(source), str(output)]
        result = subprocess.run(args, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        with wave.open(str(output), "rb") as audio:
            assert audio.getparams()[:4] == (1, 2, 16000, 1600)
            samples = struct.unpack("<1600h", audio.readframes(1600))
            assert max(samples) > 5000 and min(samples) < -5000
        before = output.read_bytes()
        assert subprocess.run(args, capture_output=True).returncode != 0
        assert output.read_bytes() == before, "existing output was modified"
        malformed = root / "malformed.wav"
        malformed.write_bytes(b"RIFF" + struct.pack("<I", 10000) + b"WAVE")
        args[-2:] = [str(malformed), str(root / "bad-output.wav")]
        assert subprocess.run(args, capture_output=True).returncode != 0
        assert not (root / "bad-output.wav").exists()
    print("WAV conversion, no-overwrite and malformed-input tests passed")


if __name__ == "__main__":
    main()
