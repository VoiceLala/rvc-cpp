"""Package only the intended source tree, without Git history, SDKs or models."""
import argparse
import hashlib
import json
import pathlib
import zipfile

ROOT_FILES = {
    "CMakeLists.txt", "README.md", "README.en.md", "LICENSE", "LICENSE_STATUS.md",
    "THIRD_PARTY_NOTICES.md", "CONTRIBUTING.md", "CHANGELOG.md",
    ".gitignore", ".gitattributes", "dependencies.json",
}
DIRECTORIES = {"include", "src", "cmake", "examples", "tests", "docs", "licenses", "scripts", ".github"}
SUFFIXES = {".c", ".cpp", ".h", ".md", ".txt", ".py", ".cmake", ".in", ".yml", ".yaml", ".json"}


def collect(root):
    found = []
    for name in sorted(ROOT_FILES):
        path = root / name
        if not path.is_file() or path.is_symlink():
            raise RuntimeError(f"Missing or symlinked release file: {name}")
        found.append(path)
    for name in sorted(DIRECTORIES):
        directory = root / name
        if directory.is_symlink() or getattr(directory, "is_junction", lambda: False)():
            raise RuntimeError(f"Refusing linked source directory: {directory}")
        for path in sorted(directory.rglob("*")):
            if path.is_symlink() or getattr(path, "is_junction", lambda: False)():
                raise RuntimeError(f"Refusing filesystem link: {path}")
            if path.is_file() and path.suffix in SUFFIXES and "__pycache__" not in path.parts:
                if not path.resolve().is_relative_to(root):
                    raise RuntimeError(f"Path escapes source root: {path}")
                found.append(path)
    return sorted(found, key=lambda p: p.relative_to(root).as_posix())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Validate and list source files without creating an archive")
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    files = collect(root)
    payload = {}
    for path in files:
        data = path.read_bytes()
        data.decode("utf-8-sig")  # Reject unexpected binary payloads in source files.
        payload[path.relative_to(root).as_posix()] = data
    if "MIT License" not in payload["LICENSE"].decode():
        raise RuntimeError("Review the packaging policy if the project license changes")
    manifest = {name: {"sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data)} for name, data in payload.items()}
    if args.check:
        print("\n".join(payload))
        print(f"Checked {len(payload)} source/documentation files; build, SDK, models and Git history excluded")
        return
    destination = root / ".release"
    if destination.is_symlink() or getattr(destination, "is_junction", lambda: False)():
        raise RuntimeError("Refusing linked output directory")
    destination.mkdir(exist_ok=True)
    archive = destination / "dvc-0.1.0-dev-source.zip"
    payload["SOURCE_MANIFEST.json"] = (json.dumps(manifest, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as output:
        for name, data in sorted(payload.items()):
            entry = zipfile.ZipInfo("dvc/" + name, date_time=(1980, 1, 1, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            output.writestr(entry, data)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (destination / "SHA256SUMS").write_text(f"{digest}  {archive.name}\n", encoding="utf-8")
    print(f"Created {archive} ({archive.stat().st_size} bytes, {len(manifest)} files + manifest)")
    print(f"SHA256 {digest}")


if __name__ == "__main__":
    main()
