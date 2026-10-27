#!/usr/bin/env python3
"""
pak_writer.py - write an original-layout (96-byte header) .pak.

  pak_writer.py --manifest <json> --input-dir <dir> --out <pak>
                (--conflict-root <dir>... | --no-conflict-check)

Layout (docs/formats/pak.md, variant B), exactly what unpack_pak.py
and PackFile read:

  0   version u32 = 1
  4   dwFileItemNum u32 = 0 (as in the original client Map.pak)
  8   flag u32 = 0
  12  zero to 92
  92  entry count u32
  96  entries: total, real, name_len, entry_start (u32 each),
      16 zero bytes, name bytes, NUL, payload

Payload bytes are copied verbatim.  This tool has no key, decode table,
compression or encryption.  Output is reproducible: entries are sorted
by casefolded name (then raw name), and no timestamp or other varying
field is written, so the same inputs give the same sha256.

Fails (exit 1, nothing left at --out) when:
  * a manifest entry has no sha256, or the input file hash differs;
  * two manifest names share a basename (case-insensitive);
  * an entry basename (case-insensitive) already exists in any .pak at
    the top level of a --conflict-root (92 or 96 layout, read with
    unpack_pak.read_entries).  The runtime falls back to basename
    lookup, first hit in mount order, so a shared basename would be
    shadowed or shadow another pak.  --out itself is skipped;
  * no --conflict-root is given (pass --no-conflict-check to skip the
    collision check explicitly; it prints a WARN to stderr);
  * manifest "pakSha256" is set and the built pak hashes differently;
  * --out (resolved real path, so symlinks and junctions count) is in a
    git working tree, inside the repository that holds this tool, under
    D:\\MX (any case, either slash), or under a forbidden root given by
    --forbidden-root or MXH_PAK_WRITER_FORBIDDEN_ROOTS (os.pathsep list);
  * read-back with unpack_pak is not the 96-byte layout only, or any
    entry differs from its input byte for byte.

Manifest: JSON with "entries": [{"name", "sha256", ...}].  Other keys
(sourcePak, ticket) are carried for review and ignored here.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import unpack_pak  # noqa: E402

HEADER_SIZE = 96
FORBIDDEN_ENV = "MXH_PAK_WRITER_FORBIDDEN_ROOTS"
TOOL_REPO = Path(__file__).resolve().parents[2]
DESC_SIZE = 32


class PakWriteError(ValueError):
    pass


def _basename_key(name: str) -> str:
    return name.replace("\\", "/").rstrip("/").split("/")[-1].casefold()


def _sort_key(name: str) -> tuple[bytes, bytes]:
    raw = name.encode("ascii")
    return raw.lower(), raw


def build_pak_bytes(entries: list[tuple[str, bytes]]) -> bytes:
    """Serialize sorted (name, payload) pairs.  Names must be ASCII."""
    if not entries:
        raise PakWriteError("refusing to write an empty pak")
    ordered = sorted(entries, key=lambda item: _sort_key(item[0]))
    out = bytearray(struct.pack("<III", 1, 0, 0))
    out += b"\x00" * (92 - len(out))
    out += struct.pack("<I", len(ordered))
    for name, data in ordered:
        name_b = name.encode("ascii")
        start = len(out)
        total = DESC_SIZE + len(name_b) + 1 + len(data)
        out += struct.pack("<IIII", total, len(data), len(name_b), start)
        out += b"\x00" * 16
        out += name_b + b"\x00" + data
    return bytes(out)


def existing_basenames(roots: list[Path], skip: Path | None = None) -> dict[str, str]:
    """casefolded basename -> "pak:entry" for every top-level .pak under roots."""
    seen: dict[str, str] = {}
    skip_resolved = skip.resolve() if skip is not None else None
    for root in roots:
        if not root.is_dir():
            raise PakWriteError(f"conflict root is not a directory: {root}")
        for pak in sorted(root.iterdir()):
            if not pak.is_file() or pak.suffix.casefold() != ".pak":
                continue
            if skip_resolved is not None and pak.resolve() == skip_resolved:
                continue
            _kind, _off4, _declared, entries = unpack_pak.read_entries(str(pak))
            for entry in entries:
                seen.setdefault(_basename_key(entry.canonical_name),
                                f"{pak.name}:{entry.canonical_name}")
    return seen


def _fold(path: str) -> str:
    return path.replace("\\", "/").rstrip("/").casefold()


def is_under(path: str, root: str) -> bool:
    """Case-insensitive, slash-agnostic prefix test on whole components."""
    p, r = _fold(path), _fold(root)
    return p == r or p.startswith(r + "/")


def is_original_root(path: str) -> bool:
    return is_under(path, "D:/MX")


def forbidden_output_reason(out: Path, extra_roots: list[str] | None = None) -> str | None:
    """Why --out may not be written, or None.  Checks the resolved parent."""
    parent = Path(os.path.realpath(out.parent))
    if is_original_root(str(parent)) or is_original_root(str(out.parent)):
        return f"{out} is under the original root D:\\MX"
    roots = list(extra_roots or [])
    roots += [r for r in os.environ.get(FORBIDDEN_ENV, "").split(os.pathsep) if r]
    for root in roots:
        real = os.path.realpath(root)
        if is_under(str(parent), real) or is_under(str(parent), root):
            return f"{out} is under forbidden root {root}"
    if is_under(str(parent), str(TOOL_REPO)):
        return f"{out} is inside the repository {TOOL_REPO}"
    for candidate in (parent, *parent.parents):
        if (candidate / ".git").exists():
            return f"{out} is inside the git working tree {candidate}"
    return None


def load_inputs(manifest: dict, input_dir: Path) -> list[tuple[str, bytes]]:
    rows = manifest.get("entries")
    if not isinstance(rows, list) or not rows:
        raise PakWriteError("manifest has no entries")
    names: dict[str, str] = {}
    out: list[tuple[str, bytes]] = []
    for row in rows:
        name = str(row.get("name") or "")
        if not name or not name.isascii() or "\x00" in name:
            raise PakWriteError(f"bad entry name {name!r}")
        key = _basename_key(name)
        if key in names:
            raise PakWriteError(f"duplicate basename {name!r} / {names[key]!r}")
        names[key] = name
        expected = row.get("sha256")
        if not isinstance(expected, str) or len(expected) != 64:
            raise PakWriteError(f"{name}: manifest sha256 is missing or not filled")
        path = input_dir / name.replace("\\", "/")
        if not path.is_file():
            raise PakWriteError(f"{name}: input file not found: {path}")
        data = path.read_bytes()
        actual = hashlib.sha256(data).hexdigest()
        if actual != expected.lower():
            raise PakWriteError(f"{name}: sha256 mismatch: manifest {expected}, file {actual}")
        out.append((name, data))
    return out


def verify_pak(pak_path: Path, entries: list[tuple[str, bytes]]) -> None:
    kind, _off4, declared, read = unpack_pak.read_entries(str(pak_path))
    if kind != "96-byte" or declared != len(entries):
        raise PakWriteError(f"read-back: layout {kind} count {declared}, want 96-byte {len(entries)}")
    expected = {name.encode("ascii"): data for name, data in entries}
    with pak_path.open("rb") as f:
        for entry in read:
            want = expected.pop(entry.name_bytes, None)
            f.seek(entry.data_offset)
            got = f.read(entry.real_size)
            if want is None or hashlib.sha256(got).digest() != hashlib.sha256(want).digest():
                raise PakWriteError(f"read-back mismatch for {entry.name}")
    if expected:
        raise PakWriteError(f"read-back missing {len(expected)} entr(ies)")


def write_pak(manifest: dict, input_dir: Path, out: Path,
              conflict_roots: list[Path] | None = None,
              forbidden_roots: list[str] | None = None) -> str:
    """Write, verify and return the pak sha256.  Leaves nothing on failure."""
    reason = forbidden_output_reason(out, forbidden_roots)
    if reason:
        raise PakWriteError("refusing to write: " + reason)
    entries = load_inputs(manifest, input_dir)
    if conflict_roots:
        seen = existing_basenames(conflict_roots, skip=out)
        clashes = [f"{name} (already in {seen[_basename_key(name)]})"
                   for name, _ in entries if _basename_key(name) in seen]
        if clashes:
            raise PakWriteError("basename already in a mounted pak: " + "; ".join(clashes))
    blob = build_pak_bytes(entries)
    want_pak = manifest.get("pakSha256")
    if want_pak and hashlib.sha256(blob).hexdigest() != str(want_pak).lower():
        raise PakWriteError(f"built pak sha256 {hashlib.sha256(blob).hexdigest()} != manifest pakSha256 {want_pak}")
    out.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix=".pak_writer.", dir=str(out.parent))
    try:
        with os.fdopen(fd, "wb") as f:
            f.write(blob)
        verify_pak(Path(tmp), entries)
        os.replace(tmp, out)
    except BaseException:
        if os.path.exists(tmp):
            os.unlink(tmp)
        raise
    return hashlib.sha256(blob).hexdigest()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="write an original 96-byte-header .pak")
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--input-dir", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--conflict-root", action="append", type=Path, default=[],
                        help="directory whose top-level .pak entries must not share a basename (required unless --no-conflict-check)")
    parser.add_argument("--no-conflict-check", action="store_true",
                        help="explicitly skip the basename collision check (prints a WARN)")
    parser.add_argument("--forbidden-root", action="append", default=[],
                        help=f"never write under this directory (also {FORBIDDEN_ENV})")
    args = parser.parse_args(argv)
    if args.no_conflict_check:
        print("WARN: --no-conflict-check: basename collision check against mounted paks skipped",
              file=sys.stderr)
    elif not args.conflict_root:
        print("ERROR: at least one --conflict-root is required (or pass --no-conflict-check)",
              file=sys.stderr)
        return 1
    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        digest = write_pak(manifest, args.input_dir, args.out,
                           [] if args.no_conflict_check else args.conflict_root,
                           args.forbidden_root)
    except (OSError, ValueError, struct.error) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    print(f"{args.out} sha256={digest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
