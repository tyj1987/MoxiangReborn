"""Compare this checkout's PAK LFS pointers with a read-only resource tree."""

import argparse
import hashlib
import json
import re
from pathlib import Path


CHECKOUT = Path(__file__).resolve().parents[1]
PAK_ROOT = Path("modern/data/PlayDH")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    args = parser.parse_args()
    result = []
    for pointer in sorted((CHECKOUT / PAK_ROOT).glob("*.pak")):
        contents = pointer.read_bytes()
        if not contents.startswith(b"version https://git-lfs.github.com/spec/v1\n"):
            continue
        text = contents.decode("ascii")
        oid = re.search(r"^oid sha256:([0-9a-f]{64})$", text, re.MULTILINE)
        size = re.search(r"^size ([0-9]+)$", text, re.MULTILINE)
        if not oid or not size:
            raise ValueError(f"Invalid LFS pointer: {pointer}")
        source = args.source_root / PAK_ROOT / pointer.name
        actual_size = source.stat().st_size if source.is_file() else None
        actual_sha = sha256(source) if actual_size == int(size.group(1)) else None
        result.append({
            "name": pointer.name,
            "expectedSize": int(size.group(1)),
            "actualSize": actual_size,
            "expectedSha256": oid.group(1),
            "actualSha256": actual_sha,
            "match": actual_sha == oid.group(1),
        })
    print(json.dumps(result, indent=2))
    return 0 if result and all(item["match"] for item in result) else 1


if __name__ == "__main__":
    raise SystemExit(main())
