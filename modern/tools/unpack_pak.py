#!/usr/bin/env python3
"""
unpack_pak.py / pak_extract — decompress 4Dyuchi .pak archives.

Mode 1 (legacy / default):
  unpack_pak.py <pak_path> [<out_dir>] [--dry-run]
  When out_dir is omitted, files go under the system temp directory
  (moxiang-unpack/<stem>).  If that location is inside the pak directory
  or the original root, Windows uses %LOCALAPPDATA%\\moxiang-unpack\\<stem>
  (or the working directory when LOCALAPPDATA is unset) and POSIX uses
  ~/.cache/moxiang-unpack/<stem> and then the working directory.  A
  candidate that still resolves inside either read-only tree is skipped.
  Each entry is written as "{i:05d}_{real}.bin" (legacy convention used by
  many existing test fixtures; preserved so the 12 existing audit scripts
  that grep the legacy name pattern keep working).

Mode 2 (pak_extract / --named):
  unpack_pak.py <pak_path> --named --out <dir> [--dry-run] [--manifest]
                       [--filter <glob>] [--verify]
  Writes each entry by its real .pak-internal name (e.g. "Map/Map10.hfl"
  or "Map\\\\Map10.hfl" from the legacy "\\" separator).
  Produces manifest.json with per-entry sha256/size/offset.

--dry-run applies to both modes and both header sizes.  It enumerates
and validates entries, and creates no directories or files.

--list is a read-only listing.  It prints the header kind, the uint32 at
offset 4, the declared count, the parsed count, and one line per entry
(start, data offset, name_len, real, total).  It does not resolve an
output directory and it does not create anything.  A format error exits
nonzero and prints parsed vs declared; it does not print a partial list.

Two resolved locations are read-only (symlinks followed, ".." folded,
case-insensitive on Windows).  The tool exits non-zero and writes
nothing, including under --dry-run, when the output is either of them:

  * the original resource root, or anything under it.  On Windows the
    root defaults to D:\\MX.  --original-root overrides MXH_ORIGINAL_ROOT,
    which overrides that default.  A prefix sibling such as D:\\MX2 is
    not inside D:\\MX.
  * the pak's own directory, or anything under it.  This still applies
    when the pak lives outside the original root.

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
    name_len, data_off, 16 zero bytes) + name bytes + NUL + data.
    total must equal 32 + name_len + 1 + real.  The next entry is
    cur + total.  data_off is this entry's own start, not the data
    position.  Data starts at entry_start + 32 + name_len + 1.
    Names are raw bytes (original samples are GBK).  GBK is used
    only for the output filename, with a replacement fallback.
    Truncated header, a count that cannot fit (minimum 33 bytes per
    entry), total mismatch, entry or data past EOF, a missing NUL,
    or bytes left after the last entry are errors.

Either header: the process validates the whole archive before it
creates the output directory.  On error it exits nonzero, reports
parsed vs declared, and leaves no partial extract.
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
import tempfile
import time
from pathlib import Path
from typing import BinaryIO, Iterable, Optional


class PakOutputError(ValueError):
    """The output path would write into a read-only resource directory."""


# Windows original-client tree.  Non-Windows has no default root.
ORIGINAL_ROOT_ENV = "MXH_ORIGINAL_ROOT"
DEFAULT_ORIGINAL_ROOT_WINDOWS = r"D:\MX"


@dataclasses.dataclass
class PakEntry:
    index: int
    name: str           # display name; 96-byte names are GBK with fallback
    real_size: int
    data_offset: int    # offset of data within the .pak (post header+name)
    entry_offset: int   # offset of the entry start within the .pak
    name_bytes: bytes = b""  # on-disk name, no NUL; match on these
    name_len: int = 0
    total_size: int = 0      # on-disk total field
    sha256: str = ""

    @property
    def canonical_name(self) -> str:
        # The legacy writer uses '\\' as the path separator inside the
        # .pak (e.g. "Map\\Map0.bmhm").  Convert to '/' so the unpacked
        # tree is portable across Windows / POSIX tools.
        return self.name.replace("\\", "/")

    @property
    def match_key(self) -> bytes:
        """Raw name bytes with '\\' folded to '/'.  Not the GBK decode."""
        return self.name_bytes.replace(b"\\", b"/")


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

def _split_path(path: str, pathmod) -> tuple[str, bool, list[str]]:
    """Drive, absolute flag, and components.  ``..`` is already folded."""
    normalized = pathmod.normpath(path)
    drive, tail = pathmod.splitdrive(normalized)
    sep = pathmod.sep
    alt = pathmod.altsep
    if alt:
        tail = tail.replace(alt, sep)
    is_abs = tail.startswith(sep)
    parts = [part for part in tail.split(sep)
             if part and part != pathmod.curdir]
    return drive, is_abs, parts


def _same_or_under(parent: str, child: str, *, case_insensitive: bool,
                   pathmod=os.path) -> bool:
    """True when child is parent or a directory inside parent.

    `parent` and `child` are already-resolved path strings.  Comparison
    is case-sensitive unless `case_insensitive` is set.  ntpath.commonpath
    always lowercases, so this does not use it: case_insensitive=False
    must not treat Client and client as the same directory.
    Production passes case_insensitive=True on Windows, so D:\\MX and
    d:\\mx still match there.  A prefix sibling does not match:
    D:\\MX2 is not under D:\\MX.
    `pathmod` is os.path in production; tests pass ntpath to check the
    Windows spelling on other hosts.
    """
    def fold(text: str) -> str:
        return text.casefold() if case_insensitive else text

    parent_drive, parent_abs, parent_parts = _split_path(parent, pathmod)
    child_drive, child_abs, child_parts = _split_path(child, pathmod)
    if parent_abs != child_abs or fold(parent_drive) != fold(child_drive):
        return False
    if len(child_parts) < len(parent_parts):
        return False
    return [fold(part) for part in child_parts[:len(parent_parts)]] == [
        fold(part) for part in parent_parts]


def resolve_original_root(cli_value: Optional[str] = None, *,
                          environ: Optional[dict[str, str]] = None,
                          windows: Optional[bool] = None) -> Optional[str]:
    """Flag, then MXH_ORIGINAL_ROOT, then D:\\MX on Windows."""
    if windows is None:
        windows = os.name == "nt"
    if cli_value is not None and cli_value.strip():
        return cli_value.strip()
    env_map = os.environ if environ is None else environ
    env_value = env_map.get(ORIGINAL_ROOT_ENV, "")
    if env_value.strip():
        return env_value.strip()
    if windows:
        return DEFAULT_ORIGINAL_ROOT_WINDOWS
    return None


def output_is_inside_pak_dir(pak_path: str, out_dir: str, *,
                             case_insensitive: Optional[bool] = None) -> bool:
    """True when resolved out_dir is the pak directory or a child of it."""
    if case_insensitive is None:
        case_insensitive = os.name == "nt"
    pak_dir = Path(pak_path).resolve().parent
    out = Path(out_dir).resolve()
    return _same_or_under(str(pak_dir), str(out),
                          case_insensitive=case_insensitive)


def output_is_inside_original_root(out_dir: str,
                                   original_root: Optional[str], *,
                                   case_insensitive: Optional[bool] = None,
                                   ) -> bool:
    """True when resolved out_dir is the original root or a child of it."""
    if original_root is None or not original_root.strip():
        return False
    if case_insensitive is None:
        case_insensitive = os.name == "nt"
    root = Path(original_root).resolve()
    out = Path(out_dir).resolve()
    return _same_or_under(str(root), str(out),
                          case_insensitive=case_insensitive)


def refuse_output_inside_pak_dir(pak_path: str, out_dir: str) -> None:
    """Raise PakOutputError before any directory or file is created."""
    if not output_is_inside_pak_dir(pak_path, out_dir):
        return
    pak_dir = Path(pak_path).resolve().parent
    raise PakOutputError(
        f"refusing to write {out_dir}: output is the pak directory or a "
        f"subdirectory of it ({pak_dir}). Original resource directories "
        f"are read-only. Pass --out outside that directory; the default "
        f"is under the system temp directory."
    )


def refuse_readonly_output(pak_path: str, out_dir: str,
                           original_root: Optional[str] = None) -> None:
    """Pak directory first, then the original resource root."""
    refuse_output_inside_pak_dir(pak_path, out_dir)
    if not output_is_inside_original_root(out_dir, original_root):
        return
    root = Path(original_root).resolve()
    raise PakOutputError(
        f"refusing to write {out_dir}: output is inside the original "
        f"root ({root}). Original resource directories are read-only. "
        f"Pass --out outside that directory; the default is under the "
        f"system temp directory."
    )


def _output_is_readonly(pak_path: str, out_dir: str,
                        original_root: Optional[str]) -> bool:
    return (output_is_inside_pak_dir(pak_path, out_dir)
            or output_is_inside_original_root(out_dir, original_root))


def _default_output_candidates(stem: str, *,
                               windows: Optional[bool] = None,
                               environ: Optional[dict[str, str]] = None,
                               temp_dir: Optional[str] = None,
                               cwd: Optional[str] = None,
                               home: Optional[str] = None) -> list[Path]:
    """Default output dirs, first choice first.

    System temp is always first.  On Windows the next choice is
    %LOCALAPPDATA%\\moxiang-unpack\\<stem>, or the working directory when
    LOCALAPPDATA is unset.  On POSIX the next choice is
    ~/.cache/moxiang-unpack/<stem>, then the working directory.
    """
    if windows is None:
        windows = os.name == "nt"
    env = os.environ if environ is None else environ
    if temp_dir is None:
        temp_dir = tempfile.gettempdir()
    if cwd is None:
        cwd = os.getcwd()
    candidates = [Path(temp_dir) / "moxiang-unpack" / stem]
    if windows:
        local = env.get("LOCALAPPDATA", "").strip()
        if local:
            candidates.append(Path(local) / "moxiang-unpack" / stem)
        else:
            candidates.append(Path(cwd) / "moxiang-unpack" / stem)
        return candidates
    if home is None:
        try:
            home = str(Path.home())
        except RuntimeError:
            home = ""
    if home:
        candidates.append(Path(home) / ".cache" / "moxiang-unpack" / stem)
    candidates.append(Path(cwd) / "moxiang-unpack" / stem)
    return candidates


def default_unpack_dir(pak_path: str,
                       original_root: Optional[str] = None) -> Path:
    """First default directory outside the pak dir and the original root.

    original_root None resolves the flag, MXH_ORIGINAL_ROOT, then the
    Windows default.  A candidate inside either read-only tree is skipped,
    so temp / LOCALAPPDATA / ~/.cache / cwd are not used when they would
    land in the root or the pak directory.
    """
    if original_root is None:
        original_root = resolve_original_root()
    stem = Path(pak_path).stem or "pak"
    for candidate in _default_output_candidates(stem):
        if not _output_is_readonly(pak_path, str(candidate), original_root):
            return candidate
    raise PakOutputError(
        f"refusing to choose a default output directory for {pak_path}: "
        f"every default location would fall inside the pak directory or "
        f"the original root. Original resource directories are read-only. "
        f"Pass --out explicitly, outside that directory."
    )


def parse_pak(pak_path: str) -> tuple[bytes, int, int, int]:
    """Read the 92-byte header.  Returns (raw_header, version, n_items, flag)."""
    with open(pak_path, "rb") as f:
        pak_header = f.read(92)
    if len(pak_header) < 92:
        raise _pak_error(
            f"truncated pak header: {pak_path} has {len(pak_header)} bytes, "
            f"need 92",
            parsed=0,
            declared=0,
            kind="92-byte",
        )
    version, n_items, flag = struct.unpack("<III", pak_header[:12])
    if version != 1:
        raise ValueError(f"unknown pak version {version:#x} in {pak_path}")
    return pak_header, version, n_items, flag


# Smallest original entry is a 32-byte header, an empty name, and one NUL.
_MIN_ENTRY_BYTES = 33
_MAX_ENTRY_COUNT = 1_000_000


def _pak_error(message: str, *, parsed: int, declared: int, kind: str) -> ValueError:
    """Format error that always names parsed vs declared."""
    return ValueError(
        f"{message} (parsed {parsed} of declared {declared}, {kind} header)"
    )


def _decode_entry_name(raw: bytes, *, original: bool) -> str:
    """Display name.  Original bytes stay on PakEntry.name_bytes.

    96-byte names are GBK only here, and only for output.  A sequence GBK
    cannot decode becomes U+FFFD and does not fail the archive.  92-byte
    names stay latin-1, which keeps every byte.
    """
    if not original:
        return raw.decode("latin-1", errors="replace")
    try:
        return raw.decode("gbk")
    except UnicodeDecodeError:
        return raw.decode("gbk", errors="replace")


def _output_relative(entry: PakEntry) -> str:
    """Path under the output directory, or a per-index fallback."""
    rel = entry.canonical_name.replace("\\", "/").lstrip("/")
    parts = [part for part in rel.split("/") if part not in ("", ".")]
    if not parts or any(part == ".." for part in parts):
        return f"unnamed_{entry.index:05d}.bin"
    return "/".join(parts)


def _original_declared_count(pak_path: str) -> int:
    """Entry count for an original-style pak (dwFileItemNum at offset 4 is 0).

    The count is the uint32 at offset 92.  Entries themselves start at 96.
    See docs/formats/pak.md.
    """
    pak_size = os.path.getsize(pak_path)
    if pak_size < 96:
        raise _pak_error(
            "truncated pak header: original-style pak (dwFileItemNum at "
            f"offset 4 is 0) needs a 96-byte header with the entry count at "
            f"offset 92, but file size is {pak_size}",
            parsed=0,
            declared=0,
            kind="96-byte",
        )
    with open(pak_path, "rb") as f:
        f.seek(92)
        raw = f.read(4)
    if len(raw) < 4:
        raise _pak_error(
            "truncated pak header: original-style pak (dwFileItemNum at "
            "offset 4 is 0) needs the entry count at offset 92, but only "
            f"{len(raw)} byte(s) were read",
            parsed=0,
            declared=0,
            kind="96-byte",
        )
    return struct.unpack("<I", raw)[0]


def _check_original_count(pak_size: int, count: int) -> None:
    """Reject a 96-byte count before any entry is yielded."""
    if count == 0:
        raise _pak_error(
            "corrupt pak header: entry count at offset 92 is 0",
            parsed=0,
            declared=0,
            kind="96-byte",
        )
    if count > _MAX_ENTRY_COUNT:
        raise _pak_error(
            f"entry count {count} is larger than the file allows",
            parsed=0,
            declared=count,
            kind="96-byte",
        )
    body = pak_size - 96
    if body // _MIN_ENTRY_BYTES < count:
        raise _pak_error(
            f"entry count {count} is larger than the file allows "
            f"({body} bytes remain after the 96-byte header)",
            parsed=0,
            declared=count,
            kind="96-byte",
        )


def _walk_original_entries(f: BinaryIO, pak_size: int, count: int) -> Iterable[PakEntry]:
    """Yield `count` original-style entries starting at offset 96.

    Advance by the entry's total.  Exactly `count` entries must consume
    the file: EOF before that is "EOF arrived early"; leftover bytes are
    "bytes remain after the declared count".  data_off is not the data
    position.  Name bytes are kept raw; GBK is display-only.
    """
    _check_original_count(pak_size, count)
    cur = 96
    for index in range(count):
        remaining = pak_size - cur

        def fail(message: str) -> ValueError:
            return _pak_error(
                message, parsed=index, declared=count, kind="96-byte")

        if remaining <= 0:
            raise fail(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"declared count is {count} but no bytes remain "
                f"(file size {pak_size})"
            )
        f.seek(cur)
        hdr = f.read(32)
        if len(hdr) < 32:
            raise fail(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"entry header is 32 bytes but only {len(hdr)} byte(s) "
                f"remain before EOF (declared count {count})"
            )
        (total, real, name_len, _data_off, _f1, _f2, _f3, _f4) = (
            struct.unpack("<IIIIIIII", hdr))
        # data_off is the entry start in original samples.  It is not
        # where the payload lives, so it is not used as a seek target.
        expected_total = 32 + name_len + 1 + real
        if total != expected_total or total < _MIN_ENTRY_BYTES:
            raise fail(
                f"pak entry {index} at offset 0x{cur:x}: total={total} "
                f"does not equal 32+name_len+1+real={expected_total} "
                f"(name_len={name_len} real_size={real})"
            )
        if cur + total > pak_size:
            raise fail(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"total={total} name_len={name_len} real_size={real} but "
                f"only {remaining} byte(s) remain before EOF "
                f"(declared count {count})"
            )
        f.seek(cur + 32)
        raw_name = f.read(name_len)
        nul = f.read(1)
        if len(raw_name) < name_len or len(nul) != 1:
            raise fail(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"filename needs {name_len + 1} bytes but the read "
                f"fell short before EOF (declared count {count})"
            )
        if nul != b"\x00":
            raise fail(
                f"pak entry {index} at offset 0x{cur:x}: expected NUL after "
                f"the filename, got 0x{nul[0]:02x}"
            )
        name = _decode_entry_name(raw_name, original=True)
        data_offset = cur + 32 + name_len + 1
        f.seek(data_offset)
        data = f.read(real)
        if len(data) < real:
            raise fail(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"payload needs {real} bytes but only {len(data)} byte(s) "
                f"remain before EOF (declared count {count})"
            )
        yield PakEntry(
            index=index,
            name=name,
            name_bytes=raw_name,
            name_len=name_len,
            total_size=total,
            real_size=real,
            data_offset=data_offset,
            entry_offset=cur,
        )
        cur += total
    if cur != pak_size:
        raise _pak_error(
            f"{pak_size - cur} byte(s) remain after the declared count "
            f"{count} at offset 0x{cur:x} (file size {pak_size})",
            parsed=count,
            declared=count,
            kind="96-byte",
        )


def _walk_worktree_entries(f: BinaryIO, pak_size: int,
                           expected: int) -> Iterable[PakEntry]:
    """92-byte header.  Start at 92 and advance by layout, not by total.

    A junk header still resyncs by 4 bytes.  Stopping before `expected`
    entries is an error.  Bytes after the declared count are allowed.
    A count that cannot fit, at 33 bytes per entry, fails before the walk.
    """
    body = pak_size - 92
    if expected > _MAX_ENTRY_COUNT or body // _MIN_ENTRY_BYTES < expected:
        raise _pak_error(
            f"entry count {expected} is larger than the file allows "
            f"({body} bytes remain after the 92-byte header)",
            parsed=0,
            declared=expected,
            kind="92-byte",
        )
    cur = 92
    index = 0
    consecutive_junk = 0
    while index < expected:
        def fail(message: str) -> ValueError:
            return _pak_error(
                message, parsed=index, declared=expected, kind="92-byte")

        if cur > pak_size or pak_size - cur < 32:
            raise fail(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"entry header is 32 bytes but the file ends first"
            )
        f.seek(cur)
        hdr = f.read(32)
        if len(hdr) < 32:
            raise fail(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"entry header is 32 bytes but only {len(hdr)} byte(s) remain"
            )
        (total, real, name_len, _data_off, _f1, _f2, _f3, _f4) = (
            struct.unpack("<IIIIIIII", hdr))
        if name_len > 4096:
            # Header at this offset is junk (probably the tail of the
            # last real entry's data misinterpreted as a header).
            # Skip 4 bytes and try the next 32-byte window.
            cur += 4
            consecutive_junk += 1
            if consecutive_junk >= 8:
                raise fail(
                    f"walk stopped at pak entry {index} offset 0x{cur:x} "
                    f"after {consecutive_junk} junk headers"
                )
            continue
        consecutive_junk = 0
        name_end = cur + 32 + name_len + 1
        data_end = name_end + real
        if name_end > pak_size or data_end > pak_size:
            raise fail(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"name_len={name_len} real_size={real} but only "
                f"{pak_size - cur} byte(s) remain before EOF"
            )
        f.seek(cur + 32)
        raw_name = f.read(name_len + 1)
        if len(raw_name) < name_len + 1:
            raise fail(
                f"EOF arrived early at pak entry {index} offset 0x{cur:x}: "
                f"filename needs {name_len + 1} bytes"
            )
        name_bytes = raw_name.split(b"\x00", 1)[0]
        name = _decode_entry_name(name_bytes, original=False)
        yield PakEntry(
            index=index,
            name=name,
            name_bytes=name_bytes,
            name_len=name_len,
            total_size=total,
            real_size=real,
            data_offset=cur + 32 + name_len + 1,
            entry_offset=cur,
        )
        # Advance by the layout, not by total.  Some worktree totals are
        # corrupt; stepping cur + total walks off the file.  There is no
        # 4-byte alignment between entries.
        cur = cur + 32 + (name_len + 1) + real
        index += 1


def walk_entries(pak_path: str, expected: int) -> Iterable[PakEntry]:
    """Yield PakEntry in order.

    `expected` is dwFileItemNum at offset 4, not a guess.  Zero selects
    the original 96-byte header (count at offset 92, entries at 96).
    A non-zero value keeps the worktree walk: start at 92, advance by
    32 + name_len + 1 + real, stop at the declared count.
    """
    pak_size = os.path.getsize(pak_path)
    if expected == 0:
        count = _original_declared_count(pak_path)
        with open(pak_path, "rb") as f:
            yield from _walk_original_entries(f, pak_size, count)
        return
    with open(pak_path, "rb") as f:
        yield from _walk_worktree_entries(f, pak_size, expected)


def read_entries(pak_path: str) -> tuple[str, int, int, list[PakEntry]]:
    """Validate the whole pak.  Returns (kind, offset4, declared, entries).

    Raises before any caller writes a file.  On success parsed == declared.
    """
    _header, _version, offset4, _flag = parse_pak(pak_path)
    entries = list(walk_entries(pak_path, offset4))
    if offset4 == 0:
        kind = "96-byte"
        declared = _original_declared_count(pak_path)
    else:
        kind = "92-byte"
        declared = offset4
    if len(entries) != declared:
        raise _pak_error(
            "walk ended before the declared count",
            parsed=len(entries),
            declared=declared,
            kind=kind,
        )
    return kind, offset4, declared, entries


def format_entry_line(entry: PakEntry) -> str:
    return (
        f"entry {entry.index} start=0x{entry.entry_offset:X} "
        f"data=0x{entry.data_offset:X} name_len={entry.name_len} "
        f"real={entry.real_size} total={entry.total_size} name={entry.name}"
    )


def format_pak_list(kind: str, offset4: int, declared: int,
                    entries: list[PakEntry]) -> str:
    lines = [
        f"header: {kind}",
        f"offset4: {offset4}",
        f"declared: {declared}",
        f"parsed: {len(entries)}",
    ]
    lines.extend(format_entry_line(entry) for entry in entries)
    lines.append(
        f"list: parsed {len(entries)} of declared {declared} "
        f"(no files written)"
    )
    return "\n".join(lines)


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

def _read_payload(f: BinaryIO, entry: PakEntry, *,
                  declared: int, kind: str) -> bytes:
    f.seek(entry.data_offset)
    data = f.read(entry.real_size)
    if len(data) != entry.real_size:
        raise _pak_error(
            f"EOF arrived early at pak entry {entry.index} offset "
            f"0x{entry.entry_offset:x}: payload needs {entry.real_size} "
            f"bytes but only {len(data)} byte(s) remain before EOF",
            parsed=entry.index,
            declared=declared,
            kind=kind,
        )
    return data


def _rollback(paths: list[Path]) -> None:
    for path in reversed(paths):
        try:
            if path.is_file() or path.is_symlink():
                path.unlink()
        except OSError:
            pass


def unpack_legacy(pak_path: str, out_dir: str, *, dry_run: bool = False,
                  original_root: Optional[str] = None) -> int:
    """The original mode that writes entries as {i:05d}_{real}.bin.

    The archive is validated before out_dir is created.  dry_run creates
    neither out_dir nor files.  original_root None resolves the
    CLI/env/Windows default.
    """
    if original_root is None:
        original_root = resolve_original_root()
    refuse_readonly_output(pak_path, out_dir, original_root)
    _header, version, n_items, flag = parse_pak(pak_path)
    kind, offset4, declared, entries = read_entries(pak_path)
    print(f"version={version} n_items={n_items} flag={flag}")
    print(f"header: {kind}")
    print(f"offset4: {offset4}")
    print(f"declared: {declared}")
    print(f"parsed: {len(entries)}")
    if not dry_run:
        os.makedirs(out_dir, exist_ok=True)
    written: list[Path] = []
    try:
        with open(pak_path, "rb") as f:
            for entry in entries:
                data = _read_payload(f, entry, declared=declared, kind=kind)
                safe = f"{entry.index:05d}_{entry.real_size}.bin"
                if not dry_run:
                    target = Path(out_dir) / safe
                    with open(target, "wb") as out:
                        out.write(data)
                    written.append(target)
                if entry.index < 3 or (entry.index + 1) % 500 == 0:
                    tail = " (dry-run)" if dry_run else ""
                    print(
                        f"  [{entry.index + 1}/{declared}] {safe}  "
                        f"({entry.real_size} bytes, "
                        f"name={entry.name[:40]!r}){tail}"
                    )
    except Exception:
        _rollback(written)
        raise
    if dry_run:
        print(f"dry-run: {len(entries)} entries (no files written)")
    else:
        print(f"wrote {len(entries)} entries")
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
                filter_pattern: Optional[str] = None,
                original_root: Optional[str] = None) -> PakStats:
    """Named-entry mode.  Returns a stats summary."""
    if original_root is None:
        original_root = resolve_original_root()
    refuse_readonly_output(pak_path, out_dir, original_root)
    t0 = time.monotonic()
    pak_size = os.path.getsize(pak_path)
    _header, version, _n_items, _flag = parse_pak(pak_path)
    # Validate every entry before creating out_dir.  A short read used to
    # increment skipped_truncated and keep going; that wrote a partial tree.
    kind, _offset4, entry_count, entries = read_entries(pak_path)
    accept = make_filter(filter_pattern)
    stats = PakStats(
        pak_path=str(Path(pak_path).resolve()),
        pak_size=pak_size,
        header_version=version,
        file_item_num=entry_count,
    )
    print(f"header: {kind}")
    print(f"declared: {entry_count}")
    print(f"parsed: {len(entries)}")

    if not dry_run:
        os.makedirs(out_dir, exist_ok=True)

    seen_raw: set[bytes] = set()
    seen_paths: set[str] = set()
    written_paths: list[Path] = []
    try:
        with open(pak_path, "rb") as f:
            for entry in entries:
                stats.parsed += 1
                if not accept(entry):
                    stats.skipped_filter += 1
                    continue
                key = entry.match_key
                if not key or key in seen_raw:
                    stats.skipped_name_bad += 1
                    continue

                data = _read_payload(
                    f, entry, declared=entry_count, kind=kind)
                rel = _output_relative(entry)
                if rel in seen_paths:
                    rel = f"unnamed_{entry.index:05d}.bin"
                if entry.canonical_name.lower().endswith(".hfl"):
                    if not hfl_header_ok(data[:32]):
                        stats.skipped_hfl_bad += 1
                        if verify:
                            print(f"  [skip-hfl-bad] {entry.canonical_name} "
                                  f"(first 32 bytes do not match HFL_DESC)")
                        continue

                entry.sha256 = hashlib.sha256(data).hexdigest()
                stats.entries.append(entry)
                seen_raw.add(key)
                seen_paths.add(rel)

                if not dry_run:
                    target = Path(out_dir) / rel
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with open(target, "wb") as out:
                        out.write(data)
                    written_paths.append(target)
                stats.written += 1
                if stats.written <= 3 or stats.written % 1000 == 0:
                    tail = " (dry-run)" if dry_run else ""
                    print(f"  [{stats.written}/{entry_count}] "
                          f"{rel}  ({entry.real_size} bytes){tail}")
    except Exception:
        _rollback(written_paths)
        raise

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
                    help="enumerate and validate only; create no directories "
                         "or files (legacy and --named, both header sizes)")
    ap.add_argument("--list", action="store_true",
                    help="print header kind, offset 4, declared count, "
                         "parsed count, and per-entry offsets; write nothing")
    ap.add_argument("--manifest", action="store_true",
                    help="write manifest.json (only with --named)")
    ap.add_argument("--verify", action="store_true",
                    help="print each skipped entry's reason")
    ap.add_argument("--filter", default=None,
                    help="only write entries whose name matches this regex")
    ap.add_argument(
        "--original-root",
        default=None,
        help="read-only original resource root. Output inside it is "
             "refused even with --dry-run "
             f"(default: {ORIGINAL_ROOT_ENV}, or "
             f"{DEFAULT_ORIGINAL_ROOT_WINDOWS} on Windows). "
             "A prefix sibling such as D:\\MX2 is allowed.",
    )
    args = ap.parse_args()
    if args.out_flag is not None:
        args.out = args.out_flag

    pak_path = args.pak
    if args.list:
        try:
            kind, offset4, declared, entries = read_entries(pak_path)
        except (ValueError, OSError) as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
        print(format_pak_list(kind, offset4, declared, entries))
        return 0

    try:
        original_root = resolve_original_root(args.original_root)
        out_dir = args.out or str(default_unpack_dir(pak_path, original_root))
        refuse_readonly_output(pak_path, out_dir, original_root)
    except PakOutputError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(f"unpacking {pak_path} -> {out_dir}")
    print(f"  size = {os.path.getsize(pak_path)} bytes")

    try:
        if args.named:
            stats = pak_extract(
                pak_path, out_dir,
                dry_run=args.dry_run,
                write_manifest=args.manifest,
                verify=args.verify,
                filter_pattern=args.filter,
                original_root=original_root,
            )
            return 0 if stats.written > 0 else 2
        return unpack_legacy(pak_path, out_dir, dry_run=args.dry_run,
                             original_root=original_root)
    except (ValueError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
