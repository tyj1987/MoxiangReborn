#!/usr/bin/env python3
"""PlayDH baseline check.

Any PR that changes reference/manifests/playdh-current.sha256.json must
state in its body that the change was approved by 战略专家.

LFS pointer files are checked by the pointer oid and size. Their bytes are
not hashed and are not downloaded. derivation.localDecoder.decodedMaps
sha256 values are hashes of the original obfuscated .map files. The
registered count must be exactly 82. No tracked *.map file is allowed.
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

_SCRIPT_DIR = Path(__file__).resolve().parent
if str(_SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPT_DIR))
import playdh_external_only

LFS_MARKER = b"version https://git-lfs.github.com/spec/v1"
DECODED_MAPS_FIELD = "derivation.localDecoder.decodedMaps"
REPO_ROOT = Path(__file__).resolve().parents[1]


def _parse_lfs_pointer(payload: bytes) -> tuple[str, int] | None:
    if not payload.startswith(LFS_MARKER):
        return None
    text = payload.decode("utf-8", "replace")
    oid = None
    size = None
    for line in text.splitlines():
        if line.startswith("oid sha256:"):
            oid = line[len("oid sha256:") :].strip().lower()
        elif line.startswith("size "):
            try:
                size = int(line[5:].strip())
            except ValueError:
                return None
    if oid is None or size is None or len(oid) != 64 or any(ch not in "0123456789abcdef" for ch in oid):
        return None
    return oid, size


def _inspect(path: Path) -> tuple[str, str | None, int | None]:
    """Return (kind, digest_or_oid, size).

    kind is file, pointer, bad-pointer, or symlink. Pointer content is not hashed.
    """
    if path.is_symlink():
        return "symlink", None, None
    with path.open("rb") as handle:
        first = handle.read(4096)
        if first.startswith(LFS_MARKER):
            rest = handle.read(4096)
            parsed = _parse_lfs_pointer(first + rest)
            if parsed is None:
                return "bad-pointer", None, None
            oid, size = parsed
            return "pointer", oid, size
        hasher = hashlib.sha256(first)
        while True:
            block = handle.read(1024 * 1024)
            if not block:
                break
            hasher.update(block)
    return "file", hasher.hexdigest(), path.stat().st_size


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


def check_playdh(
    root: Path,
    manifest_path: Path,
    external_list: Path | None = None,
    *,
    use_repo_list: bool = False,
    require_production_counts: bool = False,
) -> tuple[int, list[str]]:
    document = _load_json(manifest_path)
    expected, lines = _manifest_entries(document)
    if not expected:
        return 1, lines
    listed: list[str] = []
    if use_repo_list or external_list is not None:
        try:
            listed = playdh_external_only.playdh_names(external_list if not use_repo_list else None)
        except (OSError, ValueError) as exc:
            lines.append("PLAYDH_BASELINE EXTERNAL_LIST %s" % exc)
            return 1, lines
    listed_set = set(listed)
    for rel in listed:
        if rel not in expected:
            lines.append("PLAYDH_BASELINE EXTERNAL_NOT_IN_MANIFEST %s" % rel)
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
    for rel in listed:
        if rel in actual:
            lines.append("PLAYDH_BASELINE EXTERNAL_PRESENT %s" % rel)
    tree_expected = {rel: value for rel, value in expected.items() if rel not in listed_set}
    failed = any(
        line.startswith("PLAYDH_BASELINE COUNT")
        or line.startswith("PLAYDH_BASELINE BAD")
        or line.startswith("PLAYDH_BASELINE DUPLICATE")
        or line.startswith("PLAYDH_BASELINE EXTERNAL_")
        for line in lines
    )
    for rel in sorted(set(actual) - set(tree_expected)):
        if rel in listed_set:
            continue
        kind, digest, _size = _inspect(actual[rel])
        if kind == "symlink":
            lines.append("PLAYDH_BASELINE SYMLINK %s" % rel)
        elif kind == "bad-pointer":
            lines.append("PLAYDH_BASELINE LFS_POINTER %s" % rel)
        else:
            lines.append("PLAYDH_BASELINE EXTRA %s actual=%s" % (rel, digest))
        failed = True
    for rel in sorted(set(tree_expected) - set(actual)):
        lines.append("PLAYDH_BASELINE MISSING %s expected=%s" % (rel, tree_expected[rel][0]))
        failed = True
    for rel in sorted(set(actual) & set(tree_expected)):
        kind, digest, size = _inspect(actual[rel])
        want, want_size = expected[rel]
        if kind == "symlink":
            lines.append("PLAYDH_BASELINE SYMLINK %s" % rel)
            failed = True
            continue
        if kind == "bad-pointer":
            lines.append("PLAYDH_BASELINE LFS_POINTER %s" % rel)
            failed = True
            continue
        if kind == "pointer":
            if digest != want:
                lines.append(
                    "PLAYDH_BASELINE LFS_OID_MISMATCH %s expected=%s actual=%s" % (rel, want, digest)
                )
                failed = True
            if size != want_size:
                lines.append(
                    "PLAYDH_BASELINE LFS_SIZE_MISMATCH %s expected=%s actual=%s" % (rel, want_size, size)
                )
                failed = True
            continue
        if digest != want or size != want_size:
            lines.append(
                "PLAYDH_BASELINE MISMATCH %s expected=%s actual=%s" % (rel, want, digest)
            )
            failed = True
    if not failed and require_production_counts:
        if (
            len(tree_expected) != EXPECTED_TREE_FILES
            or len(listed) != EXPECTED_EXTERNAL_ONLY
            or len(expected) != EXPECTED_MANIFEST_FILES
        ):
            lines.append(
                "PLAYDH_BASELINE COUNT files=%s external_only=%s manifest=%s"
                % (len(tree_expected), len(listed), len(expected))
            )
            failed = True
    if not failed:
        lines.append(
            "PLAYDH_BASELINE OK files=%s external_only=%s manifest=%s"
            % (len(tree_expected), len(listed), len(expected))
        )
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
    hashes: list[str] = []
    for index, entry in enumerate(node):
        digest = entry.get("sha256") if isinstance(entry, dict) else None
        if not isinstance(digest, str) or len(digest) != 64 or any(ch not in "0123456789abcdef" for ch in digest.lower()):
            return [], "bad-sha256 index=%s" % index
        hashes.append(digest.lower())
    return hashes, None


def check_tracked_maps(paths: list[str]) -> tuple[int, list[str]]:
    hits = sorted(path for path in paths if path.lower().endswith(".map"))
    if hits:
        return 1, ["TRACKED_MAP_FORBIDDEN %s" % path for path in hits]
    return 0, ["TRACKED_MAP_FILES 0"]


def list_tracked_maps(repo: Path) -> list[str]:
    """Case-insensitive `git ls-files '*.map'`."""
    proc = subprocess.run(
        ["git", "-c", "safe.directory=%s" % repo, "ls-files", "-z", "--", ":(icase)*.map"],
        cwd=repo,
        check=False,
        capture_output=True,
    )
    if proc.returncode != 0:
        raise RuntimeError("git ls-files failed exit=%s" % proc.returncode)
    found: list[str] = []
    for raw in proc.stdout.split(b"\0"):
        if not raw:
            continue
        rel = raw.decode("utf-8", "surrogateescape").replace("\\", "/")
        if rel.lower().endswith(".map"):
            found.append(rel)
    return sorted(set(found))


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


ORIGINAL_MAP_REGISTRY_COUNT = 82
# Tracked checkout count. The manifest count stays separate.
EXPECTED_TREE_FILES = 5277
EXPECTED_MANIFEST_FILES = 5280
EXPECTED_EXTERNAL_ONLY = 3


def check_decoded(
    repo: Path,
    references_path: Path,
    tracked: list[str],
    require_count: int | None = ORIGINAL_MAP_REGISTRY_COUNT,
) -> tuple[int, list[str]]:
    document = _load_json(references_path)
    hashes, error = decoded_map_hashes(document)
    if error:
        return 1, ["ORIGINAL_MAP_HITS %s" % error]
    if require_count is not None and len(hashes) != require_count:
        return 1, [
            "ORIGINAL_MAP_REGISTRY COUNT_MISMATCH expected=%s actual=%s" % (require_count, len(hashes))
        ]
    wanted = set(hashes)
    lines: list[str] = []
    failed = False
    for rel in tracked:
        path = repo / rel
        if not path.exists():
            lines.append("TRACKED_MISSING %s" % rel)
            failed = True
            continue
        kind, digest, _size = _inspect(path)
        if kind == "symlink":
            lines.append("ORIGINAL_MAP_HITS symlink %s" % rel)
            failed = True
            continue
        if kind == "bad-pointer":
            lines.append("PLAYDH_BASELINE LFS_POINTER %s" % rel)
            failed = True
            continue
        if digest in wanted:
            lines.append("ORIGINAL_MAP_HIT %s %s" % (rel, digest))
            failed = True
    if not failed:
        lines.append("ORIGINAL_MAP_REGISTRY OK count=%s" % len(hashes))
        lines.append("ORIGINAL_MAP_HITS 0 of %s registered" % len(hashes))
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
        decoded_code, decoded_lines = check_decoded(
            root, references, ["playdh/a.bin", "playdh/b.bin"], require_count=1
        )
        green = lines + decoded_lines
        if not _expect(
            "green-ok",
            code or decoded_code,
            green,
            False,
            "PLAYDH_BASELINE OK files=2 external_only=0 manifest=2",
        ) or "ORIGINAL_MAP_HITS 0 of 1 registered" not in green:
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
        code, lines = check_decoded(
            root, references, ["playdh/a.bin", "playdh/b.bin", hit_name], require_count=1
        )
        if not _expect("decoded-hit", code, lines, True, "ORIGINAL_MAP_HIT %s %s" % (hit_name, hit_hash)):
            failed = True

        _write_references(references, [])
        code, lines = check_decoded(root, references, ["playdh/a.bin"])
        if not _expect(
            "empty-registered",
            code,
            lines,
            True,
            "ORIGINAL_MAP_REGISTRY COUNT_MISMATCH expected=82 actual=0",
        ):
            failed = True

        registry81 = [
            hashlib.sha256(("synthetic-registry-%s" % index).encode("ascii")).hexdigest()
            for index in range(81)
        ]
        _write_references(references, registry81)
        code, lines = check_decoded(root, references, [])
        if not _expect(
            "count-81",
            code,
            lines,
            True,
            "ORIGINAL_MAP_REGISTRY COUNT_MISMATCH expected=82 actual=81",
        ):
            failed = True

        pointer = playdh / "pointer.bin"
        manifest_oid = hashlib.sha256(file_a).hexdigest()
        pointer_oid = "1" * 64
        pointer.write_bytes(
            LFS_MARKER + b"\noid sha256:" + pointer_oid.encode("ascii") + b"\nsize " + str(len(file_a)).encode("ascii") + b"\n"
        )
        _write_manifest(manifest, [("pointer.bin", file_a), ("a.bin", file_a)])
        (playdh / "b.bin").unlink(missing_ok=True)
        code, lines = check_playdh(playdh, manifest)
        if not _expect(
            "lfs-oid-mismatch",
            code,
            lines,
            True,
            "PLAYDH_BASELINE LFS_OID_MISMATCH pointer.bin expected=%s actual=%s" % (manifest_oid, pointer_oid),
        ):
            failed = True
        pointer.write_bytes(
            LFS_MARKER + b"\noid sha256:" + manifest_oid.encode("ascii") + b"\nsize " + str(len(file_a) + 7).encode("ascii") + b"\n"
        )
        code, lines = check_playdh(playdh, manifest)
        if not _expect(
            "lfs-size-mismatch",
            code,
            lines,
            True,
            "PLAYDH_BASELINE LFS_SIZE_MISMATCH pointer.bin expected=%s actual=%s" % (len(file_a), len(file_a) + 7),
        ):
            failed = True

        code, lines = check_tracked_maps(["x.map", "nested/X.MAP"])
        if not _expect("tracked-map", code, lines, True, "TRACKED_MAP_FORBIDDEN x.map"):
            failed = True
        if "TRACKED_MAP_FORBIDDEN nested/X.MAP" not in lines:
            print("PLAYDH_BASELINE SELFTEST FAIL tracked-map")
            failed = True

    with tempfile.TemporaryDirectory(prefix="playdh-external-", dir=base) as temporary:
        root = Path(temporary)
        playdh = root / "playdh"
        playdh.mkdir()
        keep = b"synthetic-keep"
        other = b"synthetic-other"
        listed_bytes = b"synthetic-listed"
        (playdh / "keep.bin").write_bytes(keep)
        (playdh / "other.bin").write_bytes(other)
        manifest = root / "manifest.json"
        listed_list = root / "external.txt"
        listed_list.write_text(
            "# synthetic\nmodern/data/PlayDH/listed.bin\n",
            encoding="utf-8",
        )
        _write_manifest(
            manifest,
            [("keep.bin", keep), ("other.bin", other), ("listed.bin", listed_bytes)],
        )
        code, lines = check_playdh(playdh, manifest, listed_list)
        if not _expect(
            "external-absent-ok",
            code,
            lines,
            False,
            "PLAYDH_BASELINE OK files=2 external_only=1 manifest=3",
        ):
            failed = True

        missing_list = root / "external-missing.txt"
        missing_list.write_text(
            "modern/data/PlayDH/not-in-manifest.bin\n",
            encoding="utf-8",
        )
        code, lines = check_playdh(playdh, manifest, missing_list)
        if not _expect(
            "external-not-in-manifest",
            code,
            lines,
            True,
            "PLAYDH_BASELINE EXTERNAL_NOT_IN_MANIFEST not-in-manifest.bin",
        ):
            failed = True

        (playdh / "listed.bin").write_bytes(listed_bytes)
        code, lines = check_playdh(playdh, manifest, listed_list)
        if not _expect(
            "external-present",
            code,
            lines,
            True,
            "PLAYDH_BASELINE EXTERNAL_PRESENT listed.bin",
        ):
            failed = True
        (playdh / "listed.bin").unlink()

        (playdh / "other.bin").unlink()
        code, lines = check_playdh(playdh, manifest, listed_list)
        other_hash = hashlib.sha256(other).hexdigest()
        listed_also_missing = any(
            line.startswith("PLAYDH_BASELINE MISSING listed.bin") for line in lines
        )
        if listed_also_missing or not _expect(
            "other-missing",
            code,
            lines,
            True,
            "PLAYDH_BASELINE MISSING other.bin expected=%s" % other_hash,
        ):
            if listed_also_missing:
                print("PLAYDH_BASELINE SELFTEST FAIL other-missing")
                for line in lines:
                    print(line)
            failed = True

    listed_payload = b"synthetic-listed-ok"
    listed_hash = hashlib.sha256(listed_payload).hexdigest()
    with tempfile.TemporaryDirectory(prefix="playdh-external-root-", dir=base) as temporary:
        root = Path(temporary)
        external_root = root / "external-root"
        external_root.mkdir()
        (external_root / "keep.bin").write_bytes(b"synthetic-keep")
        (external_root / "listed.bin").write_bytes(listed_payload)
        manifest = root / "manifest.json"
        listed_list = root / "external.txt"
        listed_list.write_text(
            "# synthetic\nmodern/data/PlayDH/listed.bin\n",
            encoding="utf-8",
        )
        _write_manifest(
            manifest,
            [("keep.bin", b"synthetic-keep"), ("listed.bin", listed_payload)],
        )
        code, lines = check_playdh(external_root, manifest, listed_list)
        coverage = "PLAYDH_BASELINE COVERAGE MATCH=2 MISSING=0 MISMATCH=0 CASE_MISMATCH=0"
        matched = "PLAYDH_BASELINE MATCH listed.bin %s" % listed_hash
        if code != 0 or coverage not in lines or matched not in lines:
            print("PLAYDH_BASELINE SELFTEST FAIL external-root-match")
            for line in lines:
                print(line)
            failed = True
        else:
            print("PLAYDH_BASELINE SELFTEST OK external-root-match")

        (external_root / "listed.bin").write_bytes(b"synthetic-listed-bad")
        code, lines = check_playdh(external_root, manifest, listed_list)
        if not _expect(
            "external-root-mismatch",
            code,
            lines,
            True,
            "PLAYDH_BASELINE MISMATCH listed.bin expected=%s" % listed_hash,
        ):
            failed = True

        (external_root / "listed.bin").unlink()
        code, lines = check_playdh(external_root, manifest, listed_list)
        if not _expect(
            "external-root-missing",
            code,
            lines,
            True,
            "PLAYDH_BASELINE MISSING listed.bin expected=%s" % listed_hash,
        ):
            failed = True

        (external_root / "Listed.bin").write_bytes(listed_payload)
        code, lines = check_playdh(external_root, manifest, listed_list)
        if not _expect(
            "external-root-case",
            code,
            lines,
            True,
            "PLAYDH_BASELINE CASE_MISMATCH listed.bin",
        ):
            failed = True
    if failed:
        return 1
    print("PLAYDH_BASELINE SELFTEST OK")
    return 0


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--manifest", type=Path, default=REPO_ROOT / "reference/manifests/playdh-current.sha256.json")
    parser.add_argument("--playdh", type=Path, default=REPO_ROOT / "modern/data/PlayDH")
    parser.add_argument("--references", type=Path, default=REPO_ROOT / "reference/manifests/original-resource-references.json")
    parser.add_argument("--repo", type=Path, default=REPO_ROOT)
    args = parser.parse_args(argv)
    if args.self_test:
        return self_test()
    status, lines = check_playdh(
        args.playdh, args.manifest, use_repo_list=True, require_production_counts=True
    )
    for line in lines:
        print(line)
    try:
        tracked = list_tracked(args.repo)
    except RuntimeError as exc:
        print("ORIGINAL_MAP_HITS %s" % exc)
        return 1
    decoded_status, decoded_lines = check_decoded(args.repo, args.references, tracked)
    for line in decoded_lines:
        print(line)
    try:
        map_paths = list_tracked_maps(args.repo)
    except RuntimeError as exc:
        print("TRACKED_MAP_FILES %s" % exc)
        return 1
    map_status, map_lines = check_tracked_maps(map_paths)
    for line in map_lines:
        print(line)
    return 1 if status or decoded_status or map_status else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
