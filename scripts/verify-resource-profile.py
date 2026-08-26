#!/usr/bin/env python3
"""Verify a resource profile manifest without modifying the resource tree."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


def safe_path(root: Path, relative: str) -> Path:
    candidate = (root / relative).resolve()
    try:
        candidate.relative_to(root)
    except ValueError as exc:
        raise ValueError(f"manifest path escapes root: {relative}") from exc
    return candidate


def digest(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(block)
    return hasher.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--profile-id", required=True)
    parser.add_argument("--full", action="store_true", help="hash every manifest entry")
    parser.add_argument("--path", action="append", default=[], help="verify only this path (repeatable)")
    args = parser.parse_args()

    try:
        document = json.loads(args.manifest.read_text(encoding="utf-8"))
        if document.get("schemaVersion") != 1:
            raise ValueError("unsupported manifest schema")
        if document.get("profileId") != args.profile_id:
            raise ValueError("profile ID mismatch")
        root = (args.root or Path(document.get("root", ""))).resolve()
        if not root.is_dir():
            raise ValueError(f"resource root is not a directory: {root}")
        entries = document.get("files")
        if not isinstance(entries, list):
            raise ValueError("manifest files must be an array")
        selected = set(args.path)
        checked = 0
        total_bytes = 0
        for entry in entries:
            relative = entry.get("path")
            if not isinstance(relative, str) or (selected and relative not in selected):
                continue
            file_path = safe_path(root, relative)
            if not file_path.is_file():
                raise ValueError(f"missing resource: {relative}")
            expected_size = int(entry["bytes"])
            actual_size = file_path.stat().st_size
            if actual_size != expected_size:
                raise ValueError(f"size mismatch: {relative} ({actual_size} != {expected_size})")
            if args.full or selected:
                expected_hash = str(entry["sha256"]).lower()
                actual_hash = digest(file_path)
                if actual_hash != expected_hash:
                    raise ValueError(f"SHA-256 mismatch: {relative}")
            checked += 1
            total_bytes += actual_size
        if selected and checked != len(selected):
            missing = sorted(selected - {entry.get("path") for entry in entries})
            raise ValueError("requested paths are absent from manifest: " + ", ".join(missing))
        if not selected and not args.full:
            required = {"Map.pak", "Image/InterfaceScript/IDDlg.bin"}
            for relative in required:
                entry = next((item for item in entries if item.get("path") == relative), None)
                if entry is None:
                    raise ValueError(f"required resource absent from manifest: {relative}")
                file_path = safe_path(root, relative)
                if not file_path.is_file() or file_path.stat().st_size != int(entry["bytes"]):
                    raise ValueError(f"required resource invalid: {relative}")
            checked = len(required)
        print(json.dumps({"profileId": args.profile_id, "checked": checked,
                          "bytes": total_bytes, "fullHash": bool(args.full or selected)},
                         ensure_ascii=False))
        return 0
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"RESOURCE_PROFILE_FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
