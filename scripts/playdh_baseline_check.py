#!/usr/bin/env python3
"""PlayDH baseline check and decoded-map absence check.

Any PR that changes reference/manifests/playdh-current.sha256.json must
state in its body that the change was approved by 战略专家.

The check hashes working-tree bytes. It prints paths, counts, and sha256
only. Decoded .map bytes are not stored in this script; the registered
hashes are read from derivation.localDecoder.decodedMaps.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

LFS_MARKER = b"version https://git-lfs.github.com/spec/v1"
DECODED_MAPS_FIELD = "derivation.localDecoder.decodedMaps"
REPO_ROOT = Path(__file__).resolve().parents[1]


def _sha256(path: Path) -> tuple[str | None, str | None]:
    """Return (digest, error). error is LFS_POINTER or SYMLINK."""
    if path.is_symlink():
        return None, "SYMLINK"
    hasher = hashlib.sha256()
    with path.open("rb") as handle:
        first = handle.read(1024 * 1024)
        if first.startswith(LFS_MARKER):
            return None, "LFS_POINTER"
        hasher.update(first)
        while True:
            block = handle.read(1024 * 1024)
            if not block:
                break
            hasher.update(block)
    return hasher.hexdigest(), None


def _load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _manifest_entries(document: dict) -> tuple[dict[str, tuple[str, int]], list[str]]:
    lines: list[str] = []
    files = document.get("files")
    if not isinstance(files, list) or not files:
        lines.append("PLAYDH_BASELINE BAD_MANIFEST entries=0")
        return {}, lines
    header = document.get("fileCount")
    if header != len(files):
        lines.append(
            "PLAYDH_BASELINE COUNT header=%s entries=%s" % (header, len(files))
        )
    expected: dict[str, tuple[str, int]] = {}
    for index, entry in enumerate(files):
        if not isinstance(entry, dict):
            lines.append("PLAYDH_BASELINE BAD_ENTRY index=%s" % index)
            continue
        rel = entry.get("path")
        digest = entry.get("sha256")
        size = entry.get("bytes")
        if (
            not isinstance(rel, str)
            or not rel
            or rel.startswith(("/", "\\"))
            or ".." in rel.split("/")
        ):
            lines.append("PLAYDH_BASELINE BAD_ENTRY index=%s" % index)
            continue
        rel = rel.replace("\\", "/")
        if not isinstance(digest, str) or len(digest) != 64:
            lines.append("PLAYDH_BASELINE BAD_ENTRY index=%s" % index)
            continue
        try:
            size_int = int(size)
        except (TypeError, ValueError):
            lines.append("PLAYDH_BASELINE BAD_ENTRY index=%s" % index)
            continue
        if rel in expected:
            lines.append("PLAYDH_BASELINE DUPLICATE %s" % rel)
            continue
        expected[rel] = (digest.lower(), size_int)
    return expected, lines


def check_playdh(root: Path, manifest_path: Path) -> tuple[int, list[str]]:
    document = _load_json(manifest_path)
    expected, lines = _manifest_entries(document)
    if not expected:
        return 1, lines
    actual: dict[str, Path] = {}
    if not root.is_dir():
        lines.append("PLAYDH_BASELINE MISSING_ROOT %s" % root.as_posix())
        return 1, lines
    for directory, dirnames, filenames in os.walk(root, followlinks=False):
        dirnames[:] = [name for name in dirnames if not (Path(directory) / name).is_symlink()]
        for name in filenames:
            path = Path(directory) / name
            rel = path.relative_to(root).as_posix()
            actual[rel] = path
    failed = any(line.startswith("PLAYDH_BASELINE COUNT") or line.startswith("PLAYDH_BASELINE BAD") or line.startswith("PLAYDH_BASELINE DUPLICATE") for line in lines)
    for rel in sorted(set(actual) - set(expected)):
        digest, error = _sha256(actual[rel])
        if error == "LFS_POINTER":
            lines.append("PLAYDH_BASELINE LFS_POINTER %s" % rel)
        elif error == "SYMLINK":
            lines.append("PLAYDH_BASELINE SYMLINK %s" % rel)
        else:
            lines.append("PLAYDH_BASELINE EXTRA %s actual=%s" % (rel, digest))
        failed = True
    for rel in sorted(set(expected) - set(actual)):
        lines.append("PLAYDH_BASELINE MISSING %s expected=%s" % (rel, expected[rel][0]))
        failed = True
    for rel in sorted(set(actual) & set(expected)):
        digest, error = _sha256(actual[rel])
        want, want_size = expected[rel]
        if error == "LFS_POINTER":
            lines.append("PLAYDH_BASELINE LFS_POINTER %s" % rel)
            failed = True
            continue
        if error == "SYMLINK":
            lines.append("PLAYDH_BASELINE SYMLINK %s" % rel)
            failed = True
            continue
        size = actual[rel].stat().st_size
        if digest != want or size != want_size:
            lines.append(
                "PLAYDH_BASELINE MISMATCH %s expected=%s actual=%s" % (rel, want, digest)
            )
            failed = True
    if not failed:
        lines.append("PLAYDH_BASELINE OK files=%s" % len(expected))
        return 0, lines
    return 1, lines


def decoded_map_hashes(document: dict) -> tuple[list[str], str | None]:
    node: object = document
    for part in ("derivation", "localDecoder", "decodedMaps"):
        if not isinstance(node, dict) or part not in node:
            return [], "field-missing"
        node = node[part]
    if not isinstance(node, list):
        return [], "field-missing"
    if len(node) == 0:
        return [], "registered=0"
    hashes: list[str] = []
    for index, entry in enumerate(node):
        digest = entry.get("sha256") if isinstance(entry, dict) else None
        if not isinstance(digest, str) or len(digest) != 64 or any(ch not in "0123456789abcdef" for ch in digest.lower()):
            return [], "bad-sha256 index=%s" % index
        hashes.append(digest.lower())
    return hashes, None


def list_tracked(repo: Path) -> list[str]:
    proc = subprocess.run(
        ["git", "-c", "safe.directory=%s" % repo, "ls-files", "-z"],
        cwd=repo,
        check=False,
        capture_output=True,
    )
    if proc.returncode != 0:
        raise RuntimeError("git ls-files failed exit=%s" % proc.returncode)
    rels: list[str] = []
    for raw in proc.stdout.split(b"\0"):
        if not raw:
            continue
        rels.append(raw.decode("utf-8", "surrogateescape").replace("\\", "/"))
    return rels


def check_decoded(repo: Path, references_path: Path, tracked: list[str]) -> tuple[int, list[str]]:
    document = _load_json(references_path)
    hashes, error = decoded_map_hashes(document)
    if error:
        return 1, ["DECODED_MAP_HITS %s" % error]
    wanted = set(hashes)
    lines: list[str] = []
    failed = False
    for rel in tracked:
        path = repo / rel
        if not path.exists():
            lines.append("TRACKED_MISSING %s" % rel)
            failed = True
            continue
        digest, pointer = _sha256(path)
        if pointer:
            lines.append("LFS_POINTER %s" % rel)
            failed = True
            continue
        if digest in wanted:
            lines.append("DECODED_MAP_HIT %s %s" % (rel, digest))
            failed = True
    if not failed:
        lines.append("DECODED_MAP_HITS 0 of %s registered" % len(hashes))
        return 0, lines
    return 1, lines


def _write_manifest(path: Path, files: list[tuple[str, bytes]]) -> None:
    entries = []
    total = 0
    for rel, data in files:
        entries.append(
            {
                "path": rel,
                "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
            }
        )
        total += len(data)
    path.write_text(
        json.dumps(
            {
                "schemaVersion": 1,
                "profileId": "fixture",
                "fileCount": len(entries),
                "byteCount": total,
                "files": entries,
            },
            indent=2,
        ),
        encoding="utf-8",
    )


def _write_references(path: Path, hashes: list[str]) -> None:
    path.write_text(
        json.dumps(
            {
                "derivation": {
                    "localDecoder": {
                        "decodedMaps": [{"name": "synthetic-%s.map" % index, "sha256": digest} for index, digest in enumerate(hashes)]
                    }
                }
            }
        ),
        encoding="utf-8",
    )


def _expect(name: str, code: int, lines: list[str], expect_fail: bool, required: str) -> bool:
    ok = (code != 0) if expect_fail else (code == 0)
    if required not in lines:
        ok = False
    if ok:
        print("PLAYDH_BASELINE SELFTEST OK %s" % name)
        return True
    print("PLAYDH_BASELINE SELFTEST FAIL %s" % name)
    for line in lines:
        print(line)
    return False


def self_test() -> int:
    base = os.environ.get("RUNNER_TEMP") or None
    failed = False
    with tempfile.TemporaryDirectory(prefix="playdh-baseline-", dir=base) as temporary:
        root = Path(temporary)
        playdh = root / "playdh"
        playdh.mkdir()
        file_a = b"synthetic-baseline-a"
        file_b = b"synthetic-baseline-b"
        (playdh / "a.bin").write_bytes(file_a)
        (playdh / "b.bin").write_bytes(file_b)
        manifest = root / "manifest.json"
        _write_manifest(manifest, [("a.bin", file_a), ("b.bin", file_b)])
        absent = hashlib.sha256(b"synthetic-decoded-map-not-in-tree").hexdigest()
        references = root / "references.json"
        _write_references(references, [absent])
        code, lines = check_playdh(playdh, manifest)
        decoded_code, decoded_lines = check_decoded(root, references, ["playdh/a.bin", "playdh/b.bin"])
        green = lines + decoded_lines
        if not _expect(
            "green-ok",
            code or decoded_code,
            green,
            False,
            "PLAYDH_BASELINE OK files=2",
        ) or "DECODED_MAP_HITS 0 of 1 registered" not in green:
            if "PLAYDH_BASELINE SELFTEST FAIL green-ok" not in green:
                print("PLAYDH_BASELINE SELFTEST FAIL green-ok")
            failed = True

        extra = playdh / "extra.bin"
        extra.write_bytes(b"synthetic-baseline-extra")
        code, lines = check_playdh(playdh, manifest)
        extra_hash = hashlib.sha256(b"synthetic-baseline-extra").hexdigest()
        if not _expect(
            "extra",
            code,
            lines,
            True,
            "PLAYDH_BASELINE EXTRA extra.bin actual=%s" % extra_hash,
        ):
            failed = True
        extra.unlink()

        (playdh / "b.bin").unlink()
        code, lines = check_playdh(playdh, manifest)
        missing_hash = hashlib.sha256(file_b).hexdigest()
        if not _expect(
            "missing",
            code,
            lines,
            True,
            "PLAYDH_BASELINE MISSING b.bin expected=%s" % missing_hash,
        ):
            failed = True
        (playdh / "b.bin").write_bytes(file_b)

        changed = bytearray(file_a)
        changed[0] ^= 0x01
        (playdh / "a.bin").write_bytes(changed)
        code, lines = check_playdh(playdh, manifest)
        changed_hash = hashlib.sha256(changed).hexdigest()
        original_hash = hashlib.sha256(file_a).hexdigest()
        if not _expect(
            "changed-byte",
            code,
            lines,
            True,
            "PLAYDH_BASELINE MISMATCH a.bin expected=%s actual=%s" % (original_hash, changed_hash),
        ):
            failed = True
        (playdh / "a.bin").write_bytes(file_a)

        hit_name = "hit.bin"
        hit_bytes = b"synthetic-decoded-map-present"
        hit_hash = hashlib.sha256(hit_bytes).hexdigest()
        (root / hit_name).write_bytes(hit_bytes)
        _write_references(references, [hit_hash])
        code, lines = check_decoded(root, references, ["playdh/a.bin", "playdh/b.bin", hit_name])
        if not _expect("decoded-hit", code, lines, True, "DECODED_MAP_HIT %s %s" % (hit_name, hit_hash)):
            failed = True

        _write_references(references, [])
        code, lines = check_decoded(root, references, ["playdh/a.bin"])
        if not _expect("empty-registered", code, lines, True, "DECODED_MAP_HITS registered=0"):
            failed = True

        pointer = playdh / "pointer.bin"
        pointer.write_bytes(LFS_MARKER + b"\noid sha256:" + b"0" * 64 + b"\nsize 1\n")
        _write_manifest(manifest, [("pointer.bin", pointer.read_bytes()), ("a.bin", file_a)])
        # Manifest hash is of the pointer bytes, so content would match.
        # The pointer check must still fail before accepting that hash.
        (playdh / "b.bin").unlink(missing_ok=True)
        code, lines = check_playdh(playdh, manifest)
        if not _expect("lfs-pointer", code, lines, True, "PLAYDH_BASELINE LFS_POINTER pointer.bin"):
            failed = True
    if failed:
        return 1
    print("PLAYDH_BASELINE SELFTEST OK")
    return 0


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--manifest", type=Path, default=REPO_ROOT / "reference/manifests/playdh-current.sha256.json")
    parser.add_argument("--playdh", type=Path, default=REPO_ROOT / "modern/data/PlayDH")
    parser.add_argument("--references", type=Path, default=REPO_ROOT / "reference/manifests/original-resource-references.json")
    parser.add_argument("--repo", type=Path, default=REPO_ROOT)
    args = parser.parse_args(argv)
    if args.self_test:
        return self_test()
    status, lines = check_playdh(args.playdh, args.manifest)
    for line in lines:
        print(line)
    try:
        tracked = list_tracked(args.repo)
    except RuntimeError as exc:
        print("DECODED_MAP_HITS %s" % exc)
        return 1
    decoded_status, decoded_lines = check_decoded(args.repo, args.references, tracked)
    for line in decoded_lines:
        print(line)
    return 1 if status or decoded_status else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
