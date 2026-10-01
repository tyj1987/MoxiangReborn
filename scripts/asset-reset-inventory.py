#!/usr/bin/env python3
import json
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
OUTPUT = REPO / "modern" / "out" / "unity-remaster" / "asset-reset-v2" / "inventory.json"
PROJECT_VERSION = REPO / "unity" / "MoxiangClient" / "ProjectSettings" / "ProjectVersion.txt"

ROOTS = {
    "legacy_art": REPO / "unity" / "MoxiangClient" / "Assets" / "Moxiang" / "Art",
    "art_v2": REPO / "unity" / "MoxiangClient" / "Assets" / "Moxiang" / "ArtV2",
    "artsource": REPO / "artsource",
}

SKIP_SUFFIXES = {".meta"}


def scan(root: Path):
    extensions = Counter()
    files = []
    total_bytes = 0
    if not root.exists():
        return {
            "exists": False,
            "file_count": 0,
            "bytes": 0,
            "extensions": {},
            "largest_files": [],
        }

    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() in SKIP_SUFFIXES:
            continue
        try:
            size = path.stat().st_size
        except OSError:
            continue
        total_bytes += size
        extensions[path.suffix.lower() or "<none>"] += 1
        files.append((size, path.relative_to(REPO).as_posix()))

    files.sort(reverse=True)
    return {
        "exists": True,
        "file_count": len(files),
        "bytes": total_bytes,
        "extensions": dict(sorted(extensions.items())),
        "largest_files": [
            {"path": path, "bytes": size}
            for size, path in files[:25]
        ],
    }


def unity_version():
    if not PROJECT_VERSION.exists():
        return None
    for line in PROJECT_VERSION.read_text(encoding="utf-8").splitlines():
        if line.startswith("m_EditorVersion:"):
            return line.split(":", 1)[1].strip()
    return None


def main():
    report = {
        "schema": "moxiang.asset-reset.inventory.v1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "unity_version": unity_version(),
        "roots": {name: scan(path) for name, path in ROOTS.items()},
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"MXH_ASSET_RESET_INVENTORY_OK {OUTPUT}")
    for name, data in report["roots"].items():
        print(f"{name}: files={data['file_count']} bytes={data['bytes']}")


if __name__ == "__main__":
    main()
