#!/usr/bin/env python3
"""
unpack_pak.py / pak_extract — decompress 4Dyuchi .pak archives.

Mode 1 (legacy / default):
  unpack_pak.py <pak_path> [<out_dir>]
  Falls back to "<stem>_unpacked" when out_dir omitted.
  Each entry is written as "{i:05d}_{real}.bin" (legacy convention used by
  many existing test fixtures; preserved so the 12 existing audit scripts
  that grep the legacy name pattern keep working).

Mode 2 (pak_extract / --named):
  unpack_pak.py <pak_path> --named --out <dir> [--dry-run] [--manifest]
                       [--filter <glob>] [--verify]
  Writes each entry by its real .pak-internal name (e.g. "Map/Map10.hfl"
  or "Map\\\\Map10.hfl" from the legacy "\\" separator).
  Produces manifest.json with per-entry sha256/size/offset.

Two header variants, detected from dwFileItemNum at offset 4.
Spec: docs/formats/pak.md.

  Worktree (offset 4 != 0, e.g. PlayDH/Map.pak count 4192):
    92-byte header, entries start at 92.  Advance by
    32 + nameLen + 1 + realSize.  Some totals are corrupt, so this
    path does not step by dwTotalSize and does not require the file
    to end on the last entry.

  Original (offset 4 == 0, e.g. client Map.pak count 11267):
    96-byte header.  The entry count is the uint32 at offset 92.
    Entries start at 96.  Each entry is 32 bytes (total, real,
    name_len, data_off, 16 zero bytes) + GBK name + NUL + data.
    total == 32 + name_len + 1 + real, and the next entry is
    cur + total.  Stop after that count.  EOF before the count, or
    bytes left after it, is an error.
"""

from __future__ import annotations

import argparse
import dataclasses
import hashlib
import json
import os
import re
import struct
import sys
import time
from pathlib import Path
from typing import BinaryIO, Iterable, Optional


@dataclasses.dataclass
class PakEntry:
    index: int
    name: str           # e.g. "Map\\Map10.hfl" or "Character/man.chx"
    real_size: int
    data_offset: int    # offset of data within the .pak (post header+name)
    entry_offset: int   # offset of the entry start within the .pak
    sha256: str = ""

    @property
    def canonical_name(self) -> str:
        # The legacy writer uses '\\' as the path separator inside the
        # .pak (e.g. "Map\\Map0.bmhm").  Convert to '/' so the unpacked
        # tree is portable across Windows / POSIX tools.
        return self.name.replace("\\", "/")


@dataclasses.dataclass
class PakStats:
    pak_path: str
    pak_size: int
    header_version: int
    file_item_num: int
    parsed: int = 0
    written: int = 0
    skipped_filter: int = 0
    skipped_truncated: int = 0
    skipped_hfl_bad: int = 0
    skipped_name_bad: int = 0
    elapsed_ms: int = 0
    entries: list[PakEntry] = dataclasses.field(default_factory=list)


# ---------------------------------------------------------------------------
# Core unpack
# ---------------------------------------------------------------------------

def parse_pak(pak_path: str) -> tuple[bytes, int, int, int]:
    """Read the 92-byte header.  Returns (raw_header, version, n_items, flag)."""
    with open(pak_path, "rb") as f:
        pak_header = f.read(92)
    if len(pak_header) < 92:
        raise ValueError(f"pak header too short: {pak_path}")
    version, n_items, flag = struct.unpack("<III", pak_header[:12])
    if version != 1:
        raise ValueError(f"unknown pak version {version:#x} in {pak_path}")
    return pak_header, version, n_items, flag


def _original_declared_count(pak_path: str) -> int:
    """Entry count for an original-style pak (dwFileItemNum at offset 4 is 0).

    The count is the uint32 at offset 92.  Entries themselves start at 96.
    See docs/formats/pak.md.
    """
    pak_size = os.path.getsize(pak_path)
    if pak_size < 96:
        raise ValueError(
            "original-style pak (dwFileItemNum at offset 4 is 0) needs a "
            f"96-byte header with the entry count at offset 92, but file "
            f"size is {pak_size}"
        )
    with open(pak_path, "rb") as f:
        f.seek(92)
        raw = f.read(4)
    if len(raw) < 4:
        raise ValueError(
            "original-style pak (dwFileItemNum at offset 4 is 0) needs the "
            f"entry count at offset 92, but only {len(raw)} byte(s) were read"
        )
    return struct.unpack("<I", raw)[0]


def _walk_original_entries(f: BinaryIO, pak_size: int, count: int) -> Iterable[PakEntry]:
    """Yield `count` original-style entries starting at offset 96.

    Advance by the entry's total.  Exactly `count` entries must consume
    the file: EOF before that is "EOF arrived early"; leftover bytes are
    "bytes remain after the declared count".
    """
    cur = 96
    for index in range(count):
        remaining = pak_size - cur
        if remaining <= 0:
            raise ValueError(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"declared count is {count} but no bytes remain "
                f"(file size {pak_size})"
            )
        f.seek(cur)
        hdr = f.read(32)
        if len(hdr) < 32:
            raise ValueError(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"entry header is 32 bytes but only {len(hdr)} byte(s) "
                f"remain before EOF (declared count {count})"
            )
        (total, real, name_len, _data_off, _f1, _f2, _f3, _f4) = (
            struct.unpack("<IIIIIIII", hdr))
        expected_total = 32 + name_len + 1 + real
        if total != expected_total:
            raise ValueError(
                f"pak entry {index} at offset 0x{cur:x}: total={total} "
                f"does not equal 32+name_len+1+real={expected_total} "
                f"(name_len={name_len} real_size={real})"
            )
        if cur + total > pak_size:
            raise ValueError(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"total={total} name_len={name_len} real_size={real} but "
                f"only {remaining} byte(s) remain before EOF "
                f"(declared count {count})"
            )
        f.seek(cur + 32)
        raw_name = f.read(name_len)
        nul = f.read(1)
        if len(raw_name) < name_len or len(nul) != 1:
            raise ValueError(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"GBK filename needs {name_len + 1} bytes but the read "
                f"fell short before EOF (declared count {count})"
            )
        if nul != b"\x00":
            raise ValueError(
                f"pak entry {index} at offset 0x{cur:x}: expected NUL after "
                f"the GBK filename, got 0x{nul[0]:02x}"
            )
        try:
            name = raw_name.decode("gbk")
        except UnicodeDecodeError as exc:
            raise ValueError(
                f"pak entry {index} at offset 0x{cur:x}: filename is not "
                f"valid GBK ({exc})"
            ) from exc
        data_offset = cur + 32 + name_len + 1
        f.seek(data_offset)
        data = f.read(real)
        if len(data) < real:
            raise ValueError(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"payload needs {real} bytes but only {len(data)} byte(s) "
                f"remain before EOF (declared count {count})"
            )
        yield PakEntry(
            index=index,
            name=name,
            real_size=real,
            data_offset=data_offset,
            entry_offset=cur,
        )
        cur += total
    if cur != pak_size:
        raise ValueError(
            f"{pak_size - cur} byte(s) remain after the declared count "
            f"{count} at offset 0x{cur:x} (file size {pak_size})"
        )


def walk_entries(pak_path: str, expected: int) -> Iterable[PakEntry]:
    """Yield PakEntry in order.

    `expected` is dwFileItemNum at offset 4, not a guess.  Zero selects
    the original 96-byte header (count at offset 92, entries at 96).
    A non-zero value keeps the worktree walk below, unchanged: start at
    92, stop at the declared count, and resync 4 bytes on a junk header.
    """
    pak_size = os.path.getsize(pak_path)
    if expected == 0:
        count = _original_declared_count(pak_path)
        with open(pak_path, "rb") as f:
            yield from _walk_original_entries(f, pak_size, count)
        return
    with open(pak_path, "rb") as f:
        cur = 92
        i = 0
        consecutive_junk = 0
        while i < expected and cur < pak_size - 33:
            f.seek(cur)
            hdr = f.read(32)
            if len(hdr) < 32:
                return
            (_total, real, name_len, _data_off, _f1, _f2, _f3, _f4) = (
                struct.unpack("<IIIIIIII", hdr))
            if name_len > 4096 or real > 0xFFFFFFFF:
                # Header at this offset is junk (probably the tail of the
                # last real entry's data misinterpreted as a header).
                # Skip 4 bytes and try the next 32-byte window.  If we
                # see three consecutive junk headers (12 bytes of dead
                # space), the walk is over.
                cur += 4
                consecutive_junk += 1
                if consecutive_junk >= 8:
                    return
                continue
            consecutive_junk = 0
            f.seek(cur + 32)
            raw_name = f.read(name_len + 1)
            if len(raw_name) < name_len + 1:
                return
            f.seek(cur + 32 + name_len + 1)
            data = f.read(real)
            if len(data) < real:
                return
            name = raw_name.split(b"\x00", 1)[0].decode("latin-1", errors="replace")
            yield PakEntry(
                index=i,
                name=name,
                real_size=real,
                data_offset=cur + 32 + name_len + 1,
                entry_offset=cur,
            )
            # Advance by real_file_size only — matches modern
            # modern/src/pack_file.cpp::open_buffer() line 117-126
            # which uses `layout_advance = real_file_size` (the legacy
            # 4DyuchiFileStorage reader does NOT 4-byte-align each
            # entry).  Adding alignment padding here would walk the
            # cursor off the end of the file on real .pak inputs.
            cur = cur + 32 + (name_len + 1) + real
            i += 1


# ---------------------------------------------------------------------------
# HFL header sniff
# ---------------------------------------------------------------------------

# 4Dyuchi HFL header (HFL_DESC, fixed portion): 32 bytes.  The first DWORD
# is the literal 'HFL\\0' marker in some exports and a numeric version in
# others; we accept either as long as the next 12 bytes look like a valid
# desc block (left/top/right/bottom floats in a plausible range).  This is
# the same heuristic the C++ parse_hfl() uses as a fast-fail.
HFL_PLAUSIBLE_FACE_SIZE_MIN = 100.0
HFL_PLAUSIBLE_FACE_SIZE_MAX = 100000.0


def hfl_header_ok(prefix: bytes) -> bool:
    """Sniff the first 32 bytes of an HFL stream.

    Returns True iff the bytes decode to a plausible HFL_DESC fixed prefix.
    Returns False on malformed input.

    Layout sniffed (32 bytes total):
      u32 version
      f32 left, top, right, bottom
      f32 face_size
      u32 faces_per_object_axis
      u32 object_count_x
    (object_count_z, detail_level_count, blend_enabled live in the
    second 32-byte block that we don't sniff here — the C++ parse_hfl()
    uses the same two-block split.)
    """
    if len(prefix) < 32:
        return False
    try:
        (_ver, left, top, right, bottom, face_size,
         _fpoa, _ocx) = struct.unpack("<IfffffII", prefix[:32])
    except struct.error:
        return False
    if not (HFL_PLAUSIBLE_FACE_SIZE_MIN <= face_size <= HFL_PLAUSIBLE_FACE_SIZE_MAX):
        return False
    if not all(-1e6 < v < 1e6 for v in (left, top, right, bottom)):
        return False
    return True


# ---------------------------------------------------------------------------
# Mode 1 — legacy (preserve {i:05d}_{real}.bin naming)
# ---------------------------------------------------------------------------

def _unpack_legacy_original(pak_path: str, out_dir: str) -> int:
    """Legacy writer for the original 96-byte header (count at offset 92)."""
    pak_size = os.path.getsize(pak_path)
    count = _original_declared_count(pak_path)
    n_written = 0
    with open(pak_path, "rb") as f:
        for entry in _walk_original_entries(f, pak_size, count):
            f.seek(entry.data_offset)
            data = f.read(entry.real_size)
            if len(data) != entry.real_size:
                raise ValueError(
                    f"EOF arrived early at pak entry {entry.index} offset "
                    f"0x{entry.entry_offset:x}: payload needs {entry.real_size} "
                    f"bytes but only {len(data)} byte(s) remain before EOF "
                    f"(declared count {count})"
                )
            safe = f"{entry.index:05d}_{entry.real_size}.bin"
            target = os.path.join(out_dir, safe)
            with open(target, "wb") as out:
                out.write(data)
            n_written += 1
            if n_written <= 3 or n_written % 500 == 0:
                print(
                    f"  [{n_written}/{count}] {safe}  "
                    f"({entry.real_size} bytes, name={entry.name[:40]!r})"
                )
    print(f"wrote {n_written} entries")
    return 0


def unpack_legacy(pak_path: str, out_dir: str) -> int:
    """The original mode that writes entries as {i:05d}_{real}.bin."""
    os.makedirs(out_dir, exist_ok=True)
    _header, version, n_items, flag = parse_pak(pak_path)
    print(f"version={version} n_items={n_items} flag={flag}")
    if n_items == 0:
        return _unpack_legacy_original(pak_path, out_dir)
    cur = 92
    n_written = 0
    consecutive_junk = 0
    with open(pak_path, "rb") as f:
        i = 0
        while i < n_items:
            f.seek(cur)
            hdr = f.read(32)
            if len(hdr) < 32:
                print(f"  !! fsfile header truncated at i={i} cur=0x{cur:x}")
                break
            (_total, real, name_len, _data_off, _f1, _f2, _f3, _f4) = (
                struct.unpack("<IIIIIIII", hdr))
            if name_len > 4096 or real > 0xFFFFFFFF:
                cur += 4
                consecutive_junk += 1
                if consecutive_junk >= 8:
                    print(f"  !! walk aborted after {consecutive_junk} junk headers")
                    break
                continue
            consecutive_junk = 0
            name = f.read(name_len + 1).split(b"\x00", 1)[0].decode(
                "latin-1", errors="replace")
            data = f.read(real)
            safe = f"{i:05d}_{real}.bin"
            target = os.path.join(out_dir, safe)
            with open(target, "wb") as out:
                out.write(data)
            n_written += 1
            # Advance by real_file_size only — see walk_entries() note
            # about the 4DyuchiFileStorage no-alignment convention.
            cur = cur + 32 + (name_len + 1) + real
            if n_written <= 3 or n_written % 500 == 0:
                print(f"  [{n_written}/{n_items}] {safe}  ({real} bytes, name={name[:40]!r})")
            i += 1
    print(f"wrote {n_written} entries")
    return 0


# ---------------------------------------------------------------------------
# Mode 2 — pak_extract (named + manifest)
# ---------------------------------------------------------------------------

def make_filter(pattern: Optional[str]):
    if not pattern:
        return lambda e: True
    rx = re.compile(pattern)
    return lambda e: bool(rx.search(e.canonical_name))


def pak_extract(pak_path: str, out_dir: str, *,
                dry_run: bool = False,
                write_manifest: bool = True,
                verify: bool = False,
                filter_pattern: Optional[str] = None) -> PakStats:
    """Named-entry mode.  Returns a stats summary."""
    t0 = time.monotonic()
    pak_size = os.path.getsize(pak_path)
    _header, version, n_items, flag = parse_pak(pak_path)
    accept = make_filter(filter_pattern)
    # Offset 4 == 0 is the original header: the walk count lives at offset 92.
    # A non-zero offset-4 count is the worktree header and stays as-is.
    entry_count = _original_declared_count(pak_path) if n_items == 0 else n_items
    stats = PakStats(
        pak_path=str(Path(pak_path).resolve()),
        pak_size=pak_size,
        header_version=version,
        file_item_num=entry_count,
    )

    if not dry_run:
        os.makedirs(out_dir, exist_ok=True)

    seen_paths: set[str] = set()
    with open(pak_path, "rb") as f:
        for entry in walk_entries(pak_path, n_items):
            stats.parsed += 1
            if not accept(entry):
                stats.skipped_filter += 1
                continue
            if not entry.canonical_name or entry.canonical_name in seen_paths:
                stats.skipped_name_bad += 1
                continue

            f.seek(entry.data_offset)
            data = f.read(entry.real_size)
            if len(data) != entry.real_size:
                stats.skipped_truncated += 1
                continue

            if entry.canonical_name.lower().endswith(".hfl"):
                if not hfl_header_ok(data[:32]):
                    stats.skipped_hfl_bad += 1
                    if verify:
                        print(f"  [skip-hfl-bad] {entry.canonical_name} "
                              f"(first 32 bytes do not match HFL_DESC)")
                    continue

            entry.sha256 = hashlib.sha256(data).hexdigest()
            stats.entries.append(entry)
            seen_paths.add(entry.canonical_name)

            if not dry_run:
                target = Path(out_dir) / entry.canonical_name
                target.parent.mkdir(parents=True, exist_ok=True)
                with open(target, "wb") as out:
                    out.write(data)
            stats.written += 1
            if stats.written <= 3 or stats.written % 1000 == 0:
                tail = " (dry-run)" if dry_run else ""
                print(f"  [{stats.written}/{entry_count}] "
                      f"{entry.canonical_name}  ({entry.real_size} bytes){tail}")

    stats.elapsed_ms = int((time.monotonic() - t0) * 1000)

    if write_manifest and not dry_run:
        manifest = {
            "schemaVersion": 1,
            "tool": "pak_extract",
            "generatedAtUtc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "pak": {
                "path": stats.pak_path,
                "size": stats.pak_size,
                "version": stats.header_version,
                "fileItemNum": stats.file_item_num,
            },
            "stats": {
                "parsed": stats.parsed,
                "written": stats.written,
                "skipped_filter": stats.skipped_filter,
                "skipped_truncated": stats.skipped_truncated,
                "skipped_hfl_bad": stats.skipped_hfl_bad,
                "skipped_name_bad": stats.skipped_name_bad,
                "elapsedMs": stats.elapsed_ms,
            },
            "entries": [
                {
                    "index": e.index,
                    "name": e.canonical_name,
                    "bytes": e.real_size,
                    "sha256": e.sha256,
                    "offset": e.entry_offset,
                }
                for e in stats.entries
            ],
        }
        manifest_path = Path(out_dir) / "manifest.json"
        with open(manifest_path, "w", encoding="utf-8") as mf:
            json.dump(manifest, mf, ensure_ascii=False, indent=2)
        print(f"  manifest -> {manifest_path}")

    print(
        f"  parsed={stats.parsed}  written={stats.written}  "
        f"skipped(filter={stats.skipped_filter}, trunc={stats.skipped_truncated}, "
        f"hfl={stats.skipped_hfl_bad}, name={stats.skipped_name_bad})  "
        f"elapsed={stats.elapsed_ms}ms"
    )
    return stats


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(
        description="unpack 4Dyuchi .pak files (legacy + named/manifest mode).",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    ap.add_argument("pak", help="path to .pak file")
    ap.add_argument("out", nargs="?",
                    help="output directory (positional, legacy form; "
                         "equivalent to --out)")
    ap.add_argument("--out", dest="out_flag", default=None,
                    help="output directory (named flag form)")
    ap.add_argument("--named", action="store_true",
                    help="use pak_extract mode (named entries + manifest)")
    ap.add_argument("--dry-run", action="store_true",
                    help="walk but do not write")
    ap.add_argument("--manifest", action="store_true",
                    help="write manifest.json (only with --named)")
    ap.add_argument("--verify", action="store_true",
                    help="print each skipped entry's reason")
    ap.add_argument("--filter", default=None,
                    help="only write entries whose name matches this regex")
    args = ap.parse_args()
    if args.out_flag is not None:
        args.out = args.out_flag

    pak_path = args.pak
    out_dir = args.out or (os.path.splitext(pak_path)[0] + "_unpacked")

    print(f"unpacking {pak_path} -> {out_dir}")
    print(f"  size = {os.path.getsize(pak_path)} bytes")

    if args.named:
        stats = pak_extract(
            pak_path, out_dir,
            dry_run=args.dry_run,
            write_manifest=args.manifest,
            verify=args.verify,
            filter_pattern=args.filter,
        )
        return 0 if stats.written > 0 else 2
    return unpack_legacy(pak_path, out_dir)


if __name__ == "__main__":
    sys.exit(main())
