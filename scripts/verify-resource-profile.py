#!/usr/bin/env python3
"""Verify a resource profile manifest without modifying the resource tree."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import stat
import sys
from pathlib import Path

_SCRIPT_DIR = Path(__file__).resolve().parent
if str(_SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPT_DIR))
import playdh_external_only


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


def verify_exact_file_set(root: Path, entries: list[dict], external: set[str] | None = None) -> int:
    """Reject unlisted resources and directory links without traversing them."""
    expected = [entry.get("path") for entry in entries]
    if any(not isinstance(path, str) or not path for path in expected):
        raise ValueError("manifest contains an invalid resource path")
    if len(set(expected)) != len(expected):
        raise ValueError("manifest contains duplicate resource paths")
    actual: set[str] = set()
    def fail_walk(error: OSError) -> None:
        raise error
    for directory, dirs, files in os.walk(root, followlinks=False, onerror=fail_walk):
        for name in dirs + files:
            path = Path(directory) / name
            info = path.lstat()
            if path.is_symlink() or getattr(info, "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT:
                raise ValueError(f"linked resource path is not an exact baseline: {path.relative_to(root)}")
        for name in files:
            actual.add((Path(directory) / name).relative_to(root).as_posix())
    allowed_absent = external or set()
    present_external = sorted(allowed_absent & actual)
    if present_external:
        raise ValueError(
            "external-only path is present in the tree: " + ", ".join(present_external[:8])
        )
    unknown = sorted(name for name in allowed_absent if name not in set(expected))
    if unknown:
        raise ValueError(
            "external-only path is not in the manifest: " + ", ".join(unknown[:8])
        )
    wanted = set(expected) - allowed_absent
    extra = sorted(actual - set(expected))
    missing = sorted(wanted - actual)
    if extra or missing:
        raise ValueError(f"resource file set mismatch: extra={extra[:8]}, missing={missing[:8]}")
    return len(actual)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--profile-id", required=True)
    parser.add_argument("--full", action="store_true", help="hash every manifest entry")
    parser.add_argument("--exact", action="store_true", help="also reject unlisted files and links")
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
        external: set[str] = set()
        repo_mode = playdh_external_only.is_repo_playdh(root)
        if args.profile_id == "playdh-current" and repo_mode:
            manifest_paths = {
                entry.get("path")
                for entry in entries
                if isinstance(entry, dict) and isinstance(entry.get("path"), str)
            }
            external = set(playdh_external_only.playdh_names())
            for name in sorted(external):
                if name not in manifest_paths:
                    raise ValueError(f"external-only path is not in the manifest: {name}")
                status, _found = playdh_external_only.classify_relative(root, name)
                if status == "match":
                    raise ValueError(f"external-only path is present in the tree: {name}")
        if args.exact:
            verify_exact_file_set(root, entries, external)
        selected = set(args.path)
        checked = 0
        total_bytes = 0
        for entry in entries:
            relative = entry.get("path")
            if not isinstance(relative, str) or (selected and relative not in selected):
                continue
            if relative in external:
                continue
            if args.profile_id == "playdh-current" and not repo_mode:
                status, found = playdh_external_only.classify_relative(root, relative)
                if status == "case_mismatch":
                    raise ValueError(f"CASE_MISMATCH: {relative}")
                if status != "match" or found is None:
                    raise ValueError(f"missing resource: {relative}")
                file_path = found
            else:
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
                          "bytes": total_bytes, "fullHash": bool(args.full or selected),
                          "exactFileSet": bool(args.exact)},
                         ensure_ascii=False))
        return 0
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"RESOURCE_PROFILE_FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
