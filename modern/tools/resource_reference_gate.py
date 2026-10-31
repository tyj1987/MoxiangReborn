#!/usr/bin/env python3
"""Original resource-reference gate, warn-only by default.

This extends the MxhResourceReference / resource-profile gate. It does not
invent required files from map numbers or filename patterns.

Where each step runs
---------------------
* ``derive`` runs on a machine that has the original client files (ROG).
  It reads original ``.map`` / ``.stm`` and, when a map actually names them,
  ``.hfl`` files. It writes only names into
  ``reference/manifests/original-resource-references.json``. It does not copy
  game bytes into the repo.
  ``originalExists`` looks only under ``--client-root`` (loose files keyed
  relative to that root, and every regular ``.pak`` under it). ``--source``
  and ``--pak`` still decide which maps are parsed, the same way as before.
  Each reference records ``originalHit``. After that index is built,
  ``derive`` calls ``locate`` on every ``KNOWN_NO_ORIGINAL`` name and on
  the same stem with ``.dds`` (the extension is replaced; ``locate`` does
  not rewrite it). Those names are not references, so the reference walk
  does not check them. A non-empty hit exits non-zero and writes no
  manifest. Every miss leaves the manifest unchanged. The index is
  ``--client-root``, or ``--source`` when ``--client-root`` is omitted.
  An explicit ``--client-root`` that does not exist or is not a directory
  is an argument error before any index is built. Omitting it still falls
  back to ``--source``.
  ``trim`` removes decoder filesystem paths and ``.py`` paths. It leaves
  ``derivation.localDecoder`` (version, ``sourcePakSha256``, ``decodedMaps``,
  ``keySourceCounts``).
* ``check`` runs in CI and on any checkout. It compares that manifest to the
  checkout. A derivation that has not been run is ``no conclusion``. A
  derivation whose status is ``partial`` is a partial conclusion. Neither
  is reported as a pass.

Categories
----------
* ``present`` — referenced, an original file exists, and the checkout has it.
* ``missing`` — referenced, an original file exists, and the checkout does not.
* ``referencedButNoOriginalFile`` — the original data names the file, but no
  original bytes exist. Informational, not a defect. Known members include
  Map17 ``tile_17_mou_a_01..09.tga``, ``test4`` and ``Monster_203.bin``.
* ``partial`` — ``derive`` recorded status partial. Warned, never a pass.
  Unparsed containers are still listed under no conclusion. A ``.map``
  whose bytes have the obfuscated layout (uint32 length, 20-byte per-file
  header, XOR body) is recorded with reason ``obfuscated-map``; nothing is
  decoded in the repo and no header byte is stored.
  ``derive --local-decoder <plugin.py outside the repo>`` (or
  ``MXH_LOCAL_MAP_DECODER``) loads a local decoder plugin; there is no
  default. Its conclusions carry ``derivedBy: "local-decoder"`` and
  ``derivation.localDecoder`` records version, --pak SHA-256 and decoded
  .map SHA-256s. Maps the plugin cannot decode get
  ``obfuscated-map-key-unavailable``. ``check`` never loads plugins; it
  reports local-decoder conclusions separately and, given ``--pak`` with a
  different SHA-256, turns them into not-run (stale).
  ``derivation.unparsedSummary`` only counts unparsed entries by reason.
  Header statistics exist only behind the local ``derive
  --local-header-stats`` flag, which prints to stdout and never touches the
  manifest; CI must not pass it.
* ``no conclusion`` — derivation has not been run, or a checkout file is
  only a Git LFS pointer. Never a pass.
* ``pakLfsPointer`` — reason ``pak-lfs-pointer``: a reference is not in the
  index and one or more .pak files under root are Git LFS pointers (content
  starts with ``version https://git-lfs.github.com/spec/v1``; size is not
  used). There is no reliable reference-to-pak mapping, so any unresolved
  reference is treated as possibly inside any pointer pak. No conclusion,
  own counter; neither missing nor present.
* ``pakIndexUnreadable`` — reason ``pak-index-unreadable``: no pointer pak,
  but a real .pak under root fits neither the 92- nor 96-byte layout, so its
  entries are unknown. Unresolved references are no conclusion, own counter,
  listing the pak(s). With pointer paks present too, rows stay in
  ``pakLfsPointer`` and list both pak sets.
* ``caseMismatch`` — found only after case folding (Windows matching is
  case-insensitive). Counted as present, listed separately, severity minor.
* ``extensionSubstituted`` — not found by name, but the same stem exists with
  another extension (``x.tga`` vs ``x.dds``). A fidelity gap: not found, not
  counted as missing, own counter and codequality entry (minor).
* ``collision`` — the supplemental pak (``zz_supplement.pak`` at the top of
  the root) shares an entry basename, case-insensitive, with another
  top-level .pak. Runtime lookup falls back to basename, first hit in mount
  order, so one copy would shadow the other. This fails without the strict
  switch.

Height fields 0, 4, 23, 24, 32, 33, 34, 94 and tile tables 0, 3, 4, 23, 24,
32, 33, 34, 94, 198 are not referenced by the original and are never reported
as missing.

``check`` exits 0 unless the single switch ``MXH_RESOURCE_REF_STRICT=1``
(or ``--strict``) is set. That switch is what later turns missing files and
an unresolved conclusion into a failing exit. ``derive`` exits non-zero,
and writes nothing, when the known-no-original guard hits.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
import re
import struct
import sys
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import unpack_pak  # noqa: E402

MANIFEST_SCHEMA = "moxiang.original-resource-references.v1"
REPORT_SCHEMA = "moxiang.resource-reference-check.v1"
STRICT_ENV = "MXH_RESOURCE_REF_STRICT"
LFS_PREFIX = b"version https://git-lfs.github.com/spec/v1"

NOT_REFERENCED_HFL = (0, 4, 23, 24, 32, 33, 34, 94)
NOT_REFERENCED_TTB = (0, 3, 4, 23, 24, 32, 33, 34, 94, 198)
KNOWN_NO_ORIGINAL = (
    "tile_17_mou_a_01.tga",
    "tile_17_mou_a_02.tga",
    "tile_17_mou_a_03.tga",
    "tile_17_mou_a_04.tga",
    "tile_17_mou_a_05.tga",
    "tile_17_mou_a_06.tga",
    "tile_17_mou_a_07.tga",
    "tile_17_mou_a_08.tga",
    "tile_17_mou_a_09.tga",
    "test4",
    "Monster_203.bin",
)

# Tokens printed by the original client EngineMap.cpp loader.
# A .map reference is one of these sections followed
# by the filename the section stores. The map's own filename is not a
# requirement.
MAP_SECTIONS = {
    "GX_METADATA",
    "BOX_MAX",
    "BOX_MIN",
    "GX_MAP",
    "STATIC_MODEL",
    "HEIGHT_FIELD",
    "GX_OBJECT",
    "GX_LIGHT",
    "GX_TRIGGER",
}
FILE_SECTIONS = {"STATIC_MODEL", "HEIGHT_FIELD"}
FILE_SECTION_ORDER = ("HEIGHT_FIELD", "STATIC_MODEL", "GX_OBJECT")
HFL_DESC_SIZE = 108
HFL_NAME_SIZE = 128
STM_MATERIAL_SIZE = 168
STM_NAME_SIZE = 128
_FILENAME_RE = re.compile(r"^[A-Za-z0-9_.\\/-]{1,240}\.[A-Za-z0-9]{1,8}$")
_NUMBERED_RESOURCE_RE = re.compile(r"^(\d+)\.(hfl|ttb)$", re.IGNORECASE)


def strict_enabled(cli_flag: bool = False) -> bool:
    """The one switch that turns this gate from warn-only into failing."""
    raw = os.environ.get(STRICT_ENV, "")
    return cli_flag or raw.strip().lower() in {"1", "true", "yes", "on"}


SUPPLEMENTAL_PAK_NAME = "zz_supplement.pak"


def _basename(name: str) -> str:
    return name.replace("\\", "/").rstrip("/").split("/")[-1]


def _casefold_name(name: str) -> str:
    return _basename(name).casefold()


def _ascii_fold(text: str) -> str:
    """Fold ASCII A-Z to a-z. Length-preserving, locale-independent.

    ``PackFile::find`` (``modern/src/pack_file.cpp`` eq_ci) compares with
    ``std::tolower`` per byte. ``pak_filename_less`` folds only A-Z. This
    matches both for bytes 0-127 and leaves every other code unit alone,
    so equality of folded strings is that ``eq_ci``.
    """
    return "".join(chr(ord(ch) + 32) if "A" <= ch <= "Z" else ch for ch in text)


def _display_rel(path: Path, source: Path | None) -> str | None:
    """Path relative to ``source`` with forward slashes, or the filename.

    Never an absolute path. ``None`` when ``source`` is set and ``path``
    is not inside it.
    """
    if source is not None:
        try:
            return path.resolve().relative_to(source.resolve()).as_posix()
        except (OSError, ValueError):
            return None
    return path.name


def is_lfs_pointer(path: Path) -> bool:
    try:
        with path.open("rb") as handle:
            return handle.read(len(LFS_PREFIX)) == LFS_PREFIX
    except OSError:
        return False


def excluded_resource(name: str) -> str | None:
    """Return 'hfl' or 'ttb' when the original does not reference this file."""
    match = _NUMBERED_RESOURCE_RE.fullmatch(_basename(name))
    if not match:
        return None
    number = int(match.group(1))
    kind = match.group(2).lower()
    if kind == "hfl" and number in NOT_REFERENCED_HFL:
        return "hfl"
    if kind == "ttb" and number in NOT_REFERENCED_TTB:
        return "ttb"
    return None


def _known_sets(names: list[str] | tuple[str, ...]) -> tuple[set[str], set[str]]:
    exact: set[str] = set()
    stems: set[str] = set()
    for name in names:
        folded = name.casefold()
        exact.add(folded)
        if "." not in folded:
            stems.add(folded)
    return exact, stems


def matches_known_no_original(name: str, known: list[str] | tuple[str, ...]) -> bool:
    exact, stems = _known_sets(known)
    base = _casefold_name(name)
    if base in exact:
        return True
    stem = base.rsplit(".", 1)[0] if "." in base else base
    return stem in stems


def _section_token(token: str) -> str | None:
    cleaned = token.lstrip("*").upper()
    if cleaned in MAP_SECTIONS:
        return cleaned
    return None


def _looks_like_filename(token: str, known: tuple[str, ...] = KNOWN_NO_ORIGINAL) -> bool:
    if not token or _section_token(token) or token.isdigit():
        return False
    if _FILENAME_RE.fullmatch(token):
        return True
    return matches_known_no_original(token, known)


def _decode_text(data: bytes) -> str | None:
    if not data or b"\x00" in data[:64]:
        # Binary maps still carry the ASCII section tokens; handled separately.
        printable = sum(32 <= byte < 127 or byte in (9, 10, 13) for byte in data)
        if printable < len(data) * 0.85:
            return None
    try:
        return data.decode("latin-1")
    except UnicodeError:
        return None


def parse_map_references(data: bytes) -> list[dict]:
    """Filenames stored in one original .map. Extensions are kept as written."""
    text = _decode_text(data)
    if text and ("HEIGHT_FIELD" in text.upper() or "STATIC_MODEL" in text.upper() or "GX_MAP" in text.upper()):
        parsed = _parse_map_text(text)
        if parsed:
            return parsed
    return _parse_map_binary(data)


def _parse_map_text(text: str) -> list[dict]:
    refs: list[dict] = []
    pending: str | None = None
    in_objects = False
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or line.startswith(("//", "#", ";")):
            continue
        tokens = line.split()
        if not tokens:
            continue
        section = _section_token(tokens[0])
        if section:
            in_objects = section == "GX_OBJECT"
            if section in FILE_SECTIONS:
                if len(tokens) > 1 and _looks_like_filename(tokens[1]):
                    refs.append(_ref(tokens[1], "map", section))
                    pending = None
                else:
                    pending = section
            else:
                pending = None
            continue
        if pending and _looks_like_filename(tokens[0]):
            refs.append(_ref(tokens[0], "map", pending))
            pending = None
            continue
        if in_objects and _looks_like_filename(tokens[0]):
            refs.append(_ref(tokens[0], "map", "GX_OBJECT"))
    return _dedupe(refs)


def _parse_map_binary(data: bytes) -> list[dict]:
    """Read a C string stored immediately after a section token."""
    refs: list[dict] = []
    upper = data.upper()
    for section in FILE_SECTION_ORDER:
        marker = section.encode("ascii")
        start = 0
        while True:
            index = upper.find(marker, start)
            if index < 0:
                break
            before = data[index - 1] if index else 0
            after_at = index + len(marker)
            after = data[after_at] if after_at < len(data) else 0
            bounded = (index == 0 or not (48 <= before <= 57 or 65 <= before <= 90 or 97 <= before <= 122))
            if bounded and after in (0, 9, 10, 13, 32):
                cursor = after_at
                while cursor < len(data) and data[cursor] in (0, 9, 10, 13, 32):
                    cursor += 1
                end = cursor
                while end < len(data) and 32 <= data[end] < 127 and data[end] not in (10, 13):
                    end += 1
                token = data[cursor:end].decode("latin-1", "replace").strip()
                if section == "GX_OBJECT":
                    # The object block stores one filename per C string.
                    block = data[cursor:cursor + 4096]
                    for piece in block.split(b"\x00"):
                        name = piece.decode("latin-1", "replace").strip()
                        if _section_token(name):
                            break
                        if _looks_like_filename(name):
                            refs.append(_ref(name, "map", section))
                        elif name and not _looks_like_filename(name):
                            if any(ch < " " or ch > "~" for ch in name):
                                break
                elif _looks_like_filename(token):
                    refs.append(_ref(token, "map", section))
            start = index + len(marker)
    return _dedupe(refs)


def parse_hfl_used_textures(data: bytes) -> list[str]:
    """Used HFL palette names, original extension unchanged.

    Numeric palette tokens are untextured entries, matching parse_hfl and
    audit_map_dependencies. Unused palette rows are not requirements.
    """
    if len(data) < 4 + HFL_DESC_SIZE:
        raise ValueError("truncated HFL header")
    version = struct.unpack_from("<I", data, 0)[0]
    if version == 0 or version > 0x10:
        raise ValueError(f"unsupported HFL version {version}")
    desc = 4
    texture_count = struct.unpack_from("<I", data, desc + 44)[0]
    height_x, height_z = struct.unpack_from("<II", data, desc + 52)
    tile_x, tile_z = struct.unpack_from("<II", data, desc + 96)
    if texture_count > 65536 or height_x == 0 or height_z == 0:
        raise ValueError("implausible HFL dimensions")
    cursor = 4 + HFL_DESC_SIZE + height_x * height_z * 4
    if cursor + 4 > len(data):
        raise ValueError("truncated HFL heights")
    stored = struct.unpack_from("<I", data, cursor)[0]
    if stored != texture_count:
        raise ValueError("invalid HFL texture table")
    cursor += 4
    names: list[str] = []
    for _ in range(texture_count):
        if cursor + 2 + HFL_NAME_SIZE > len(data):
            raise ValueError("truncated HFL texture table")
        raw = data[cursor + 2:cursor + 2 + HFL_NAME_SIZE]
        names.append(raw.split(b"\x00", 1)[0].decode("latin-1", "replace"))
        cursor += 2 + HFL_NAME_SIZE
    if cursor + 8 > len(data):
        raise ValueError("truncated HFL tile table")
    stored_x, stored_z = struct.unpack_from("<II", data, cursor)
    if stored_x != tile_x or stored_z != tile_z:
        raise ValueError("invalid HFL tile table dimensions")
    cursor += 8
    count = tile_x * tile_z
    if cursor + count * 2 > len(data):
        raise ValueError("truncated HFL tile table")
    used: set[int] = set()
    for index in range(count):
        slot = struct.unpack_from("<H", data, cursor + index * 2)[0] & 0x3FFF
        if slot < len(names):
            used.add(slot)
    found: list[str] = []
    for index in sorted(used):
        name = names[index].strip()
        if name and not name.isdigit():
            found.append(name)
    return found


def parse_stm_textures(data: bytes) -> list[str]:
    """STM material texture names, original extension unchanged."""
    if len(data) < 8:
        raise ValueError("truncated STM header")
    version, material_count = struct.unpack_from("<II", data, 0)
    if version != 1 or material_count > 65536:
        raise ValueError("unsupported STM material table")
    cursor = 8
    names: list[str] = []
    for _ in range(material_count):
        if cursor + STM_MATERIAL_SIZE > len(data):
            raise ValueError("truncated STM material table")
        raw = data[cursor + 36:cursor + 36 + STM_NAME_SIZE]
        name = raw.split(b"\x00", 1)[0].decode("latin-1", "replace").strip()
        if name:
            names.append(name)
        cursor += STM_MATERIAL_SIZE
    return names


def _ref(name: str, container: str, section: str) -> dict:
    return {
        "name": name.replace("\\", "/"),
        "containerKind": container,
        "section": section,
    }


def _dedupe(refs: list[dict]) -> list[dict]:
    seen: set[str] = set()
    unique: list[dict] = []
    for ref in refs:
        key = ref["name"].casefold()
        if key in seen:
            continue
        seen.add(key)
        unique.append(ref)
    return unique


def _iter_files(root: Path):
    if not root.is_dir():
        return
    for directory, dirnames, filenames in os.walk(root, followlinks=False):
        dirnames[:] = [name for name in dirnames if name not in {".git"}]
        for filename in filenames:
            yield Path(directory) / filename


def _read_pak(pak_path: Path) -> tuple[str, int, list] | None:
    """Return ``(kind, offset-4 field, entries)`` for the one valid layout.

    ``unpack_pak.read_entries`` walks both layouts and keeps the single walk
    that ends at EOF.  92-byte PlayDH: count is the uint32 at offset 4,
    entries start at 92.  96-byte original: count is the uint32 at offset
    92, entries start at 96.  Original Map.pak stores 0 at offset 4 and the
    real count (11267) at offset 92.  Both layouts, or neither, prints one
    ERROR line naming the pak and the caller skips it.  The count passed to
    the walker is the layout's own field, never an inflated cap.
    ``kind`` is ``92-byte`` or ``96-byte``.
    """
    try:
        kind, offset4, _declared, entries = unpack_pak.read_entries(str(pak_path))
    except (OSError, ValueError, struct.error) as exc:
        detail = str(exc)
        if str(pak_path) not in detail:
            detail = f"{pak_path}: {detail}"
        print(f"ERROR: {detail}", file=sys.stderr)
        return None
    return str(kind), int(offset4), entries


def _read_pak_entries(pak_path: Path) -> tuple[int, list] | None:
    """Offset-4 field and entries. See ``_read_pak``."""
    read = _read_pak(pak_path)
    if read is None:
        return None
    _kind, offset4, entries = read
    return offset4, entries


def _pak_bytes(pak_path: Path, entry) -> bytes:
    with pak_path.open("rb") as handle:
        handle.seek(entry.data_offset)
        data = handle.read(entry.real_size)
    if len(data) != entry.real_size:
        raise ValueError(f"truncated pak entry {entry.name}")
    return data


def _suffix(name: str) -> str:
    base = _basename(name)
    return base.rsplit(".", 1)[-1].casefold() if "." in base else ""


class _LooseFile:
    __slots__ = ("rel", "path")

    def __init__(self, rel: str, path: Path) -> None:
        self.rel = rel
        self.path = path


class _MountedPak:
    __slots__ = ("rel", "path", "tier", "by_name")

    def __init__(self, rel: str, path: Path, tier: int, by_name: dict) -> None:
        self.rel = rel
        self.path = path
        self.tier = tier
        self.by_name = by_name


class _Hit:
    __slots__ = ("label", "path", "entry")

    def __init__(self, label: str, path: Path, entry) -> None:
        self.label = label
        self.path = path
        self.entry = entry


class _BlobIndex:
    """Existence lookup. This index does not choose which maps are parsed."""

    def __init__(self) -> None:
        self.loose: list[_LooseFile] = []
        self.loose_by_fold: dict[str, _LooseFile] = {}
        self.packs: list[_MountedPak] = []

    def locate(self, name: str) -> _Hit | None:
        """``FilesystemFileStorage::locate`` (filesystem_file_storage.cpp).

        Loose file under the source root first (case-insensitive relative
        path; not a basename search). Then mounted paks in mount order.
        Inside a pak, ``PackFile::find`` eq_ci on the full name, and only
        when the query itself has a directory, eq_ci of the query basename
        against an entry's full name. Zero-length entries do not hit
        (``has_payload``). No extension is rewritten.
        """
        if not name:
            return None
        loose = self.loose_by_fold.get(_ascii_fold(name.replace("\\", "/")))
        if loose is not None:
            return _Hit("loose:" + loose.rel, loose.path, None)
        query = _ascii_fold(name.replace("/", "\\"))
        base = query.rsplit("\\", 1)[-1]
        for pack in self.packs:
            entry = pack.by_name.get(query)
            if (entry is None or entry.real_size <= 0) and base != query:
                entry = pack.by_name.get(base)
            if entry is not None and entry.real_size > 0:
                return _Hit("pak:" + pack.rel, pack.path, entry)
        return None


def _entry_match_name(entry) -> str:
    """On-disk entry name, byte-preserving, with ``/`` folded to ``\\``.

    ``PackFile`` stores the raw name bytes and ``find`` compares those.
    ``PakEntry.name`` for a 96-byte pak is a GBK display decode, so the
    match uses ``name_bytes`` (latin-1, one code unit per byte).
    """
    raw = getattr(entry, "name_bytes", b"") or b""
    text = raw.decode("latin-1") if raw else (entry.name or "")
    return text.replace("/", "\\")


def _pak_tier_rank(kind: str) -> int:
    """Default ``pak_tier_order``: PlayDH 92-byte, then original 96-byte.

    ``FilesystemFileStorageOptions`` in ``FilesystemFileStorage.hpp``.
    """
    if kind == "92-byte":
        return 0
    if kind == "96-byte":
        return 1
    return 2


def _mount_sort_key(rel: str, tier: int) -> tuple:
    """Tier, then ``pak_filename_less`` on the relative path.

    The client sorts ``filename().u8string()`` because it only mounts the
    top of its root (``directory_iterator`` in ``Initialize``). Existence
    mounts every pak under ``--client-root``, so the same comparison runs
    on the path relative to that root.
    """
    return (tier, _ascii_fold(rel), rel)


def _can_read_file(path: Path) -> bool:
    try:
        with path.open("rb"):
            return True
    except OSError:
        return False


class _ParseIndex:
    """Basename index used only to decide which maps are parsed and to load them.

    This is the pre-TYJ-165 rule: loose files under ``--source`` (pak files
    skipped) plus the single ``--pak``. Other archives never contribute
    ``.map`` names. Keys are ``basename.casefold()``.
    """

    def __init__(self) -> None:
        self.by_base: dict[str, list[tuple[str, bytes | None, Path | None]]] = {}

    def add(self, name: str, data: bytes | None, path: Path | None) -> None:
        self.by_base.setdefault(_casefold_name(name), []).append((name, data, path))

    def find(self, name: str):
        return self.by_base.get(_casefold_name(name), [])


def _build_parse_index(source: Path | None, pak: Path | None) -> tuple[_ParseIndex, dict]:
    """Map-discovery index. Unchanged by the client-root existence walk."""
    index = _ParseIndex()
    pak_info = {"path": str(pak) if pak else "", "headerEntries": None, "entriesReadToEof": None}
    if source and source.is_dir():
        for path in _iter_files(source):
            if path.suffix.casefold() == ".pak":
                continue
            if is_lfs_pointer(path):
                continue
            try:
                data = path.read_bytes()
            except OSError:
                continue
            index.add(path.name, data, path)
    if pak:
        if is_lfs_pointer(pak):
            raise ValueError(f"refusing to derive references from a Git LFS pointer: {pak}")
        read = _read_pak_entries(pak)
        if read is not None:
            header_count, entries = read
            pak_info["headerEntries"] = header_count
            pak_info["entriesReadToEof"] = len(entries)
            for entry in entries:
                index.add(entry.canonical_name, None, None)
                index.add(entry.name, None, None)
            pak_info["entries"] = entries
            pak_info["pakPath"] = pak
    return index, pak_info


def _load_named(index: _ParseIndex, pak_info: dict, name: str) -> bytes | None:
    """Load map, hfl, or stm bytes the way derive did before the existence index."""
    for _stored, data, path in index.find(name):
        if data is not None:
            return data
        if path is not None and path.is_file() and not is_lfs_pointer(path):
            return path.read_bytes()
    pak = pak_info.get("pakPath")
    if not pak:
        return None
    wanted = _casefold_name(name)
    for entry in pak_info.get("entries", []):
        if _casefold_name(entry.canonical_name) == wanted or _casefold_name(entry.name) == wanted:
            return _pak_bytes(pak, entry)
    return None


def _build_exists_index(client_root: Path | None) -> _BlobIndex:
    """Paks and loose files under ``client_root`` only.

    Loose keys are the path relative to ``client_root`` (``Resource/Map/x.hfl``,
    not ``client/Resource/Map/x.hfl``). A file under ``server/``, ``src/``, or
    ``database/`` is not a hit. ``--pak`` is included only when it is inside
    this root, because the walk finds it there.
    """
    index = _BlobIndex()
    if client_root is None or not client_root.is_dir():
        return index
    loose_paths: list[Path] = []
    pak_paths: list[Path] = []
    for path in _iter_files(client_root):
        if not path.is_file():
            continue
        if path.suffix.casefold() == ".pak":
            pak_paths.append(path)
        else:
            loose_paths.append(path)
    pak_paths.sort(key=lambda path: _mount_sort_key(_display_rel(path, client_root) or path.name, 0))
    mounted: list[_MountedPak] = []
    for path in pak_paths:
        if is_lfs_pointer(path):
            continue
        rel = _display_rel(path, client_root)
        if not rel or rel == ".":
            continue
        read = _read_pak(path)
        if read is None:
            continue
        kind, _offset4, entries = read
        by_name: dict = {}
        for entry in entries:
            # First entry in file order wins, including a zero-length one.
            # find() returns that first eq_ci match; has_payload then rejects
            # it and does not walk further entries of the same name.
            match = _entry_match_name(entry)
            by_name.setdefault(_ascii_fold(match), entry)
        mounted.append(_MountedPak(rel, path, _pak_tier_rank(kind), by_name))
    mounted.sort(key=lambda item: _mount_sort_key(item.rel, item.tier))
    index.packs = mounted
    loose_files: list[_LooseFile] = []
    for path in loose_paths:
        if is_lfs_pointer(path) or not _can_read_file(path):
            continue
        rel = _display_rel(path, client_root)
        if not rel:
            continue
        loose_files.append(_LooseFile(rel, path))
    loose_files.sort(key=lambda item: (_ascii_fold(item.rel), item.rel))
    for item in loose_files:
        index.loose_by_fold.setdefault(_ascii_fold(item.rel), item)
    index.loose = loose_files
    return index


def _original_hit(index: _BlobIndex, name: str) -> str:
    """``loose:<rel>`` or ``pak:<rel>``, or ``""`` when ``locate`` misses.

    Relative to ``--client-root``, forward slashes, no absolute path. The v1
    reference object had no field for the satisfying path; ``originalHit``
    is that record and is documented on ``derive``. ``check`` does not read
    it. ``originalExists`` is true exactly when this string is non-empty.
    """
    hit = index.locate(name)
    return "" if hit is None else hit.label


OBFUSCATED_MAP_REASON = "obfuscated-map"
NO_MARKERS_REASON = "map did not contain HEIGHT_FIELD, STATIC_MODEL or GX_OBJECT"
OBFUSCATED_MAP_PREFIX = 4
OBFUSCATED_MAP_HEADER = 20


def is_obfuscated_map(data: bytes) -> bool:
    """True when ``data`` has the obfuscated .map layout.

    Layout (see docs/formats/original-map-obfuscation.md): uint32 LE length
    equal to the whole entry length, a 20-byte per-file header, then an
    XOR-obfuscated body. Shape check only; the body is never decoded and no
    header byte is returned. Conservative: the length field must equal
    len(data), the body must be non-empty, the data must not start with
    ``GX_``, and no plaintext section marker may appear.
    """
    head = OBFUSCATED_MAP_PREFIX + OBFUSCATED_MAP_HEADER
    if len(data) <= head:
        return False
    (declared,) = struct.unpack_from("<I", data, 0)
    if declared != len(data):
        return False
    if data.startswith(b"GX_"):
        return False
    upper = data.upper()
    # No printable-ratio test: some real obfuscated bodies are >85% printable
    # after XOR. Plaintext maps start with GX_ and carry section markers.
    return not any(marker in upper for marker in (b"HEIGHT_FIELD", b"STATIC_MODEL", b"GX_OBJECT", b"GX_MAP"))


OBFUSCATED_KEY_UNAVAILABLE_REASON = "obfuscated-map-key-unavailable"
DECODED_NO_MARKERS_REASON = "local-decoder output did not contain HEIGHT_FIELD, STATIC_MODEL or GX_OBJECT"
LOCAL_DECODER_ENV = "MXH_LOCAL_MAP_DECODER"
LOCAL_DECODER_TAG = "local-decoder"
REPO_ROOT = TOOLS_DIR.parents[1]


class LocalDecoderError(ValueError):
    pass


class ClientRootError(ValueError):
    """``--client-root`` was passed and is missing or not a directory."""

    def __init__(self, path: Path) -> None:
        self.path = Path(path)
        if not self.path.exists():
            detail = "does not exist"
        else:
            detail = "is not a directory"
        super().__init__(f"--client-root {detail}: {self.path}")


class KnownNoOriginalFound(Exception):
    """A name recorded as having no original file is in the existence index.

    ``hits`` is ``(queried name, locate label)``. The label is ``loose:`` or
    ``pak:`` plus a path relative to the existence root. No absolute path.
    """

    def __init__(self, hits: list[tuple[str, str]]) -> None:
        self.hits = list(hits)
        listing = "\n".join(f"{name} -> {label}" for name, label in self.hits)
        super().__init__(
            "ERROR: known-no-original name is present; manifest not written\n" + listing)


def _dds_stem_name(name: str) -> str:
    """Same stem with a ``.dds`` extension.

    Replaces the final extension. A name with no extension gains ``.dds``.
    Directory separators are normalized to ``/`` and otherwise kept.
    """
    normalized = name.replace("\\", "/")
    slash = normalized.rfind("/")
    base = normalized[slash + 1:]
    prefix = normalized[:slash + 1]
    dot = base.rfind(".")
    renamed = base + ".dds" if dot <= 0 else base[:dot] + ".dds"
    return prefix + renamed


def _guard_known_no_original(index: _BlobIndex, names: tuple[str, ...]) -> None:
    """Fail when a recorded-absent name, or its ``.dds`` stem, is present.

    ``names`` are not in ``references``, so the reference walk never calls
    ``locate`` for them. The renderer maps ``.tga`` to ``.dds``; this lookup
    does not, so both names are queried. A miss changes nothing.
    """
    hits: list[tuple[str, str]] = []
    seen: set[str] = set()
    for name in names:
        for query in (name, _dds_stem_name(name)):
            key = _ascii_fold(query)
            if key in seen:
                continue
            seen.add(key)
            hit = index.locate(query)
            if hit is not None and hit.label:
                hits.append((query, hit.label))
    if hits:
        raise KnownNoOriginalFound(hits)


def _file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def resolve_local_decoder_path(cli_path: Path | None, environ=None) -> Path | None:
    """Only an explicit --local-decoder or MXH_LOCAL_MAP_DECODER selects a plugin.

    There is no default search path. ``check`` never calls this.
    """
    if cli_path is not None:
        return Path(cli_path)
    env = os.environ if environ is None else environ
    value = env.get(LOCAL_DECODER_ENV, "").strip()
    return Path(value) if value else None


def load_local_decoder(path: Path, repo_root: Path = REPO_ROOT):
    """Load a decoder plugin that lives outside the repository tree.

    Contract: ``DECODER_VERSION: str`` and
    ``decode_map(name: str, data: bytes) -> bytes | None``. Optional:
    ``key_source(name: str) -> str | None``, called after a successful
    decode_map, records how that map's key was obtained (for example a table
    lookup versus statistical recovery) as ``keySource`` so that later work can
    re-check one group only. The repository ships no decoder; a path inside
    ``repo_root`` is refused.
    """
    import importlib.util

    resolved = Path(path).expanduser().resolve()
    root = Path(repo_root).resolve()
    if resolved == root or root in resolved.parents:
        raise LocalDecoderError(f"refusing to load a local decoder inside the repository: {resolved}")
    if resolved.suffix != ".py" or not resolved.is_file():
        raise LocalDecoderError(f"local decoder must be an existing .py file: {resolved}")
    spec = importlib.util.spec_from_file_location("mxh_local_map_decoder", resolved)
    if spec is None or spec.loader is None:
        raise LocalDecoderError(f"cannot load local decoder: {resolved}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    version = getattr(module, "DECODER_VERSION", None)
    if not isinstance(version, str) or not version:
        raise LocalDecoderError("local decoder must define DECODER_VERSION: str")
    if not callable(getattr(module, "decode_map", None)):
        raise LocalDecoderError("local decoder must define decode_map(name, data)")
    key_source = getattr(module, "key_source", None)
    if key_source is not None and not callable(key_source):
        raise LocalDecoderError("local decoder key_source must be callable(name)")
    return module


_KEY_SOURCE_RE = re.compile(r"^[a-z0-9][a-z0-9-]{0,31}$")


def _local_key_source(local_decoder, map_name: str) -> str | None:
    hook = getattr(local_decoder, "key_source", None)
    if hook is None:
        return None
    value = hook(map_name)
    if value is None:
        return None
    if not isinstance(value, str) or not _KEY_SOURCE_RE.match(value):
        raise LocalDecoderError("key_source must return a short lowercase label or None")
    return value


def _unparsed_summary(unparsed: list[dict]) -> dict:
    by_reason: dict[str, int] = {}
    for item in unparsed:
        reason = str(item.get("reason", ""))
        by_reason[reason] = by_reason.get(reason, 0) + 1
    return {"total": len(unparsed), "byReason": dict(sorted(by_reason.items()))}


def derive(source: Path | None, pak: Path | None = None,
           known_no_original: tuple[str, ...] = KNOWN_NO_ORIGINAL,
           local_header_stats: dict[int, int] | None = None,
           local_decoder=None,
           client_root: Path | None = None) -> dict:
    """Build a name-only manifest from original .map / .stm / .hfl bytes.

    ``local_header_stats``, when a dict is passed (local CLI flag only), is
    filled in memory with per-file header byte 2 -> count for obfuscated
    maps. It is never written into the returned manifest.

    ``local_decoder`` is a module loaded by ``load_local_decoder``. Its
    plaintext output is parsed like a plaintext map; only names, the decoder
    version, the --pak SHA-256 and each decoded original .map SHA-256 are
    recorded. References from decoded maps carry ``derivedBy:
    "local-decoder"``. ``trim_manifest`` removes decoder filesystem paths
    and ``.py`` paths. It keeps ``derivation.localDecoder``.

    Map names come only from loose files under ``--source`` and from
    ``--pak``, keyed by casefolded basename. Other paks do not add maps.

    ``originalHit`` is relative to ``client_root`` (or ``source`` when
    ``client_root`` is omitted). ``loose:<path>`` and ``pak:<path>`` use
    forward slashes. ``""`` is a miss. The referenced extension is not
    rewritten.

    After the existence index is built, every ``known_no_original`` name
    and the same stem with ``.dds`` is passed to ``locate``. A non-empty
    hit raises ``KnownNoOriginalFound`` before a manifest exists. Every
    miss leaves the returned document identical to a derive that does not
    perform this check. An explicit ``client_root`` that does not exist or
    is not a directory raises ``ClientRootError`` before either index is
    built. Omitted ``client_root`` still falls back to ``source``.
    """
    if client_root is not None and not client_root.is_dir():
        raise ClientRootError(client_root)
    if local_decoder is not None and pak is None:
        raise LocalDecoderError(
            "--local-decoder requires --pak: local-decoder conclusions must record the "
            "source pak SHA-256 so check --pak can verify them")
    parse_index, pak_info = _build_parse_index(source, pak)
    exists_index = _build_exists_index(source if client_root is None else client_root)
    _guard_known_no_original(exists_index, known_no_original)
    index = exists_index
    references: list[dict] = []
    excluded: list[dict] = []
    unparsed: list[dict] = []
    seen_maps = 0
    consumed: set[str] = set()
    decoded_maps: list[dict] = []

    for map_name in sorted(name for name in parse_index.by_base if name.endswith(".map")):
        data = _load_named(parse_index, pak_info, map_name)
        if data is None:
            unparsed.append({"name": map_name, "reason": "map bytes unavailable"})
            continue
        seen_maps += 1
        derived_by: str | None = None
        if local_decoder is not None and is_obfuscated_map(data):
            plain = local_decoder.decode_map(map_name, data)
            if plain is None:
                unparsed.append({"name": map_name, "reason": OBFUSCATED_KEY_UNAVAILABLE_REASON})
                continue
            if not isinstance(plain, (bytes, bytearray)):
                raise LocalDecoderError("decode_map must return bytes or None")
            decoded_entry = {"name": map_name, "sha256": hashlib.sha256(data).hexdigest()}
            key_source = _local_key_source(local_decoder, map_name)
            if key_source is not None:
                decoded_entry["keySource"] = key_source
            decoded_maps.append(decoded_entry)
            data = bytes(plain)
            del plain
            derived_by = LOCAL_DECODER_TAG
            try:
                refs = parse_map_references(data)
            except ValueError:
                refs = []
            if not refs:
                unparsed.append({"name": map_name, "reason": DECODED_NO_MARKERS_REASON})
                continue
        else:
            try:
                refs = parse_map_references(data)
            except ValueError as exc:
                unparsed.append({"name": map_name, "reason": str(exc)})
                continue
            if not refs and b"GX_MAP" not in data.upper() and b"HEIGHT_FIELD" not in data.upper():
                if is_obfuscated_map(data):
                    unparsed.append({"name": map_name, "reason": OBFUSCATED_MAP_REASON})
                    if local_header_stats is not None:
                        value = data[OBFUSCATED_MAP_PREFIX + 2]
                        local_header_stats[value] = local_header_stats.get(value, 0) + 1
                else:
                    unparsed.append({"name": map_name, "reason": NO_MARKERS_REASON})
                continue
        del data
        _absorb(references, excluded, refs, map_name, index, derived_by)
        for ref in refs:
            suffix = _suffix(ref["name"])
            if suffix not in {"hfl", "stm"}:
                continue
            key = _casefold_name(ref["name"])
            if key in consumed or excluded_resource(ref["name"]):
                continue
            consumed.add(key)
            payload = _load_named(parse_index, pak_info, ref["name"])
            if payload is None:
                continue
            try:
                if suffix == "hfl":
                    textures = parse_hfl_used_textures(payload)
                    section = "hfl-used-texture"
                else:
                    textures = parse_stm_textures(payload)
                    section = "stm-material"
            except ValueError as exc:
                unparsed.append({"name": ref["name"], "reason": str(exc)})
                continue
            nested = [_ref(texture, suffix, section) for texture in textures]
            _absorb(references, excluded, nested, ref["name"], index, derived_by)

    if seen_maps == 0:
        status = "not-run"
    elif unparsed:
        status = "partial"
    else:
        status = "complete"
    public_pak = {key: value for key, value in pak_info.items() if key not in {"entries", "pakPath"}}
    document = {
        "schema": MANIFEST_SCHEMA,
        "derivation": {
            "status": status,
            "runsOn": "ROG, the machine that has the original client files",
            "checkedBy": "CI check command against this manifest; it does not read original .map/.stm bytes",
            "parsedMapCount": seen_maps,
            "pak": public_pak,
            "unparsed": unparsed,
            "unparsedSummary": _unparsed_summary(unparsed),
            "note": (
                "Required names come only from filenames stored in original .map, "
                ".stm and used .hfl palette rows. Map numbers are not turned into "
                "N.hfl or N.ttb requirements."
            ),
        },
        "references": references,
        "excludedNotReferenced": excluded,
        "notReferencedByOriginal": {
            "hfl": list(NOT_REFERENCED_HFL),
            "ttb": list(NOT_REFERENCED_TTB),
        },
        "knownReferencedNoOriginalFile": list(known_no_original),
    }
    if local_decoder is not None:
        document["derivation"]["localDecoder"] = {
            "version": str(local_decoder.DECODER_VERSION),
            "sourcePakSha256": _file_sha256(pak) if pak else None,
            "decodedMaps": decoded_maps,
        }
        key_sources: dict[str, int] = {}
        for entry in decoded_maps:
            if "keySource" in entry:
                key_sources[entry["keySource"]] = key_sources.get(entry["keySource"], 0) + 1
        if key_sources:
            document["derivation"]["localDecoder"]["keySourceCounts"] = dict(sorted(key_sources.items()))
    return document


def _absorb(references: list[dict], excluded: list[dict], refs: list[dict],
            via: str, index: _BlobIndex, derived_by: str | None = None) -> None:
    have = {_casefold_name(item["name"]) for item in references}
    have_excluded = {_casefold_name(item["name"]) for item in excluded}
    for ref in refs:
        key = _casefold_name(ref["name"])
        kind = excluded_resource(ref["name"])
        hit = _original_hit(index, ref["name"])
        row = {
            "name": ref["name"],
            "via": via,
            "section": ref["section"],
            "originalExists": hit != "",
            "originalHit": hit,
        }
        if derived_by:
            row["derivedBy"] = derived_by
        if kind:
            if key not in have_excluded:
                row["kind"] = kind
                excluded.append(row)
                have_excluded.add(key)
            continue
        if key in have:
            continue
        references.append(row)
        have.add(key)


class _CheckoutIndex:
    """Names the checkout has, keyed casefolded, original case kept.

    ``names`` maps a casefolded basename to every original-case spelling seen
    (loose files and real .pak entries). ``stems`` maps a casefolded name
    without its last extension to the original-case names carrying it.
    ``pointers`` holds casefolded basenames of Git LFS pointer files and
    ``pointer_paks`` the original-case names of .pak files that are pointers.
    ``unreadable_paks`` holds real .pak files whose entry table could not be
    read (``_read_pak_entries`` returned None); their entries are unknown.
    A file is a pointer iff its content starts with ``LFS_PREFIX``; size is
    never used.
    """

    def __init__(self) -> None:
        self.names: dict[str, set[str]] = {}
        self.stems: dict[str, set[str]] = {}
        self.pointers: set[str] = set()
        self.pointer_paks: list[str] = []
        self.unreadable_paks: list[str] = []

    def add(self, name: str) -> None:
        base = _basename(name)
        if not base:
            return
        self.names.setdefault(base.casefold(), set()).add(base)
        self.stems.setdefault(_stem_key(base), set()).add(base)

    def add_pointer(self, name: str) -> None:
        base = _basename(name)
        self.pointers.add(base.casefold())
        if base.casefold().endswith(".pak"):
            self.pointer_paks.append(base)

    @classmethod
    def from_names(cls, names, pointer_names=(), pointer_paks=(),
                   unreadable_paks=()) -> "_CheckoutIndex":
        """Build an index without a filesystem (tests and offline harnesses)."""
        index = cls()
        index.unreadable_paks.extend(_basename(n) for n in unreadable_paks)
        for name in names:
            index.add(name)
        for name in list(pointer_names) + list(pointer_paks):
            index.add_pointer(name)
        return index


def _stem_key(name: str) -> str:
    base = _basename(name).casefold()
    return base.rsplit(".", 1)[0] if "." in base else base


def _checkout_index(root: Path) -> _CheckoutIndex:
    """Index loose files and real .pak entries under ``root``.

    LFS pointer files (content starts with ``LFS_PREFIX``) are recorded as
    pointers; a pointer .pak has no readable entries, so it is recorded in
    ``pointer_paks`` and its entries stay unknown.
    """
    index = _CheckoutIndex()
    if not root.is_dir():
        return index
    for path in _iter_files(root):
        if is_lfs_pointer(path):
            index.add_pointer(path.name)
            continue
        index.add(path.name)
        if path.suffix.casefold() == ".pak":
            read = _read_pak_entries(path)
            if read is None:
                # Entries unknown: record it so refs that could be inside it
                # become no conclusion (pak-index-unreadable), not missing.
                index.unreadable_paks.append(path.name)
                continue
            _header_count, entries = read
            for entry in entries:
                index.add(entry.canonical_name)
    return index


def supplemental_pak_collisions(root: Path) -> list[dict]:
    """Basenames the supplemental pak shares with another mounted pak.

    Only top-level .pak files are mounted.  Runtime lookup tries the full
    name, then the basename, first hit in mount order, so a shared
    basename (case-insensitive) means one copy silently shadows the other.
    """
    if not root.is_dir():
        return []
    paks = sorted(p for p in root.iterdir() if p.is_file() and p.suffix.casefold() == ".pak")
    supplemental = [p for p in paks if p.name.casefold() == SUPPLEMENTAL_PAK_NAME]
    if not supplemental or is_lfs_pointer(supplemental[0]):
        return []
    read = _read_pak_entries(supplemental[0])
    if read is None:
        return [{"name": "", "pak": supplemental[0].name, "other": "", "reason": "supplemental pak is unreadable"}]
    mine = {_casefold_name(e.canonical_name): e.canonical_name for e in read[1]}
    rows: list[dict] = []
    for other in paks:
        if other == supplemental[0] or is_lfs_pointer(other):
            continue
        other_read = _read_pak_entries(other)
        if other_read is None:
            rows.append({"name": "", "pak": supplemental[0].name, "other": other.name,
                         "reason": "other pak unreadable"})
            continue
        for entry in other_read[1]:
            key = _casefold_name(entry.canonical_name)
            if key in mine:
                rows.append({"name": mine[key], "pak": supplemental[0].name,
                             "other": f"{other.name}:{entry.canonical_name}",
                             "reason": "supplemental pak shares a basename with another mounted pak"})
    return rows


def check(manifest: dict, root: Path, strict: bool = False, pak: Path | None = None) -> dict:
    """Compare a checkout to the manifest. Never loads a decoder plugin.

    ``pak`` is only hashed, to verify ``derivation.localDecoder``.
    """
    return check_index(manifest, _checkout_index(root), strict=strict, pak=pak,
                       collisions=supplemental_pak_collisions(root))


def check_index(manifest: dict, index: _CheckoutIndex, strict: bool = False,
                pak: Path | None = None, collisions: list[dict] | None = None) -> dict:
    """``check`` against a prebuilt checkout index.

    Resolution order for a reference whose original exists:

    1. exact-case basename in the index -> ``present``;
    2. same name after case folding only -> ``present`` and also listed in
       ``caseMismatch`` (severity minor; Windows matching is case-insensitive);
    3. the basename itself is an LFS pointer file -> ``noConclusion``;
    4. one or more .pak files under root are LFS pointers -> ``pakLfsPointer``
       (reason ``pak-lfs-pointer``). ``originalHit`` records which original
       archive satisfied the name at derive time. It is not a checkout
       path, so there is still no reliable reference-to-checkout-pak
       mapping (the same name family is spread across Map/Character/Effect/
       monster). ANY unresolved reference is treated as possibly inside ANY
       pointer pak.
       No conclusion: neither missing nor present. If unreadable paks also
       exist, the row stays in ``pakLfsPointer`` and lists both sets
       (``paks`` and ``unreadablePaks``) so neither is hidden;
    4b. otherwise, one or more real .pak files under root could not be read
       (neither 92- nor 96-byte layout) -> ``pakIndexUnreadable`` (reason
       ``pak-index-unreadable``, listing the paks). Same "could be inside
       any" rule, same no-conclusion treatment;
       a pak already reported as unreadable by the supplemental-collision
       check (``collisions``) is counted there only: it is left out of
       ``pakIndexUnreadable``/``unreadablePaks`` and refs that could be inside
       it are plain ``noConclusion`` pointing at ``pakBasenameCollisions``;
    5. same stem with a different extension (``x.tga`` vs ``x.dds``) ->
       ``extensionSubstituted``: a fidelity gap, not found, not missing;
    6. otherwise -> ``missing``.

    Steps 4/4b precede step 5 because with pointer paks the exact name may still
    be inside one, so an extension substitute is not yet a conclusion.
    """
    if manifest.get("schema") != MANIFEST_SCHEMA:
        raise ValueError("unsupported resource-reference manifest schema")
    derivation = manifest.get("derivation") or {}
    status = derivation.get("status", "not-run")
    known = list(manifest.get("knownReferencedNoOriginalFile") or KNOWN_NO_ORIGINAL)
    references = list(manifest.get("references") or [])
    pointer_names = index.pointers
    collisions = list(collisions or [])
    # One unreadable pak, one bucket: a pak the supplemental-collision check
    # already reports as unreadable (other pak unreadable / supplemental pak
    # is unreadable) belongs to pakBasenameCollisions only. The reference
    # check excludes it from pakIndexUnreadable / unreadablePaks; references
    # that could be inside it are plain no conclusion pointing at that row.
    claimed = {
        (row["other"] if row.get("reason") == "other pak unreadable" else row["pak"]).casefold()
        for row in collisions
        if row.get("reason") in ("other pak unreadable", "supplemental pak is unreadable")
    }
    bucket_unreadable = sorted((p for p in index.unreadable_paks if p.casefold() not in claimed),
                               key=str.casefold)
    collision_unreadable = sorted((p for p in index.unreadable_paks if p.casefold() in claimed),
                                  key=str.casefold)
    local_info = derivation.get("localDecoder")
    local_state: str | None = None
    if isinstance(local_info, dict):
        expected = local_info.get("sourcePakSha256")
        if pak is None:
            local_state = "unverified"
        elif expected and Path(pak).is_file() and _file_sha256(Path(pak)) == expected:
            local_state = "verified"
        else:
            local_state = "stale"
    local_rows: list[dict] = []

    present: list[dict] = []
    missing: list[dict] = []
    no_original: list[dict] = []
    no_conclusion: list[dict] = []
    case_mismatch: list[dict] = []
    pak_lfs_pointer: list[dict] = []
    pak_index_unreadable: list[dict] = []
    extension_substituted: list[dict] = []
    seen: set[str] = set()

    def add_no_original(name: str, reason: str, via: str = "") -> None:
        key = _casefold_name(name)
        if key in seen or excluded_resource(name):
            return
        seen.add(key)
        no_original.append({
            "name": name,
            "via": via,
            "reason": reason,
            "defect": False,
        })

    if status == "not-run":
        no_conclusion.append({
            "name": "",
            "reason": (
                "original .map/.stm derivation has not been completed on the machine "
                "that has the original files; no conclusion, not a pass"
            ),
        })
    elif status not in {"complete", "partial"}:
        no_conclusion.append({
            "name": "",
            "reason": (
                f"original .map/.stm derivation status {status!r} is not complete; "
                "no conclusion, not a pass"
            ),
        })
    for item in derivation.get("unparsed") or []:
        no_conclusion.append({
            "name": item.get("name", ""),
            "reason": "unparsed original container: " + str(item.get("reason", "")),
        })

    for ref in references:
        name = str(ref.get("name", ""))
        if not name or excluded_resource(name):
            continue
        key = _casefold_name(name)
        if key in seen:
            continue
        if local_state is not None and ref.get("derivedBy") == LOCAL_DECODER_TAG:
            local_rows.append({"name": name, "via": ref.get("via", ""), "state": local_state})
            if local_state == "stale":
                seen.add(key)
                no_conclusion.append({
                    "name": name,
                    "reason": (
                        "local-decoder conclusion is not-run (stale): manifest sourcePakSha256 "
                        "does not match the given pak; no conclusion, not a pass"
                    ),
                })
                continue
        if matches_known_no_original(name, known) or ref.get("originalExists") is False:
            add_no_original(
                name,
                "referenced by the original, but no original file exists",
                str(ref.get("via", "")),
            )
            continue
        if ref.get("originalExists") is not True:
            seen.add(key)
            no_conclusion.append({
                "name": name,
                "reason": "reference has no originalExists result; no conclusion, not a pass",
            })
            continue
        spellings = index.names.get(key)
        if spellings:
            seen.add(key)
            present.append({"name": name, "via": ref.get("via", "")})
            if _basename(name) not in spellings:
                case_mismatch.append({
                    "name": name,
                    "via": ref.get("via", ""),
                    "checkout": sorted(spellings),
                    "reason": "case-mismatch",
                    "severity": "minor",
                })
            continue
        if key in pointer_names:
            seen.add(key)
            no_conclusion.append({
                "name": name,
                "reason": "checkout only has a Git LFS pointer; no conclusion, not a pass",
            })
            continue
        seen.add(key)
        if index.pointer_paks:
            pak_lfs_pointer.append({
                "name": name,
                "via": ref.get("via", ""),
                "reason": "pak-lfs-pointer",
                "paks": sorted(index.pointer_paks, key=str.casefold),
                "unreadablePaks": bucket_unreadable,
            })
            continue
        if bucket_unreadable:
            pak_index_unreadable.append({
                "name": name,
                "via": ref.get("via", ""),
                "reason": "pak-index-unreadable",
                "paks": bucket_unreadable,
            })
            continue
        if collision_unreadable:
            no_conclusion.append({
                "name": name,
                "reason": "could be inside " + ", ".join(collision_unreadable)
                          + " (unreadable; counted under pakBasenameCollisions); no conclusion, not a pass",
            })
            continue
        substitutes = sorted(index.stems.get(_stem_key(name), ()))
        if substitutes:
            extension_substituted.append({
                "name": name,
                "via": ref.get("via", ""),
                "checkout": substitutes,
                "reason": "extension-substituted",
            })
            continue
        missing.append({
            "name": name,
            "via": ref.get("via", ""),
            "reason": "referenced by the original and the original file exists, but the checkout does not have it",
        })

    for name in known:
        add_no_original(name, "known referenced-by-original name; no original file exists")

    if collisions:
        conclusion = "collision"
    elif status == "partial":
        conclusion = "partial"
    elif no_conclusion or pak_lfs_pointer or pak_index_unreadable:
        conclusion = "no conclusion"
    elif missing or extension_substituted:
        conclusion = "warnings"
    else:
        conclusion = "clean"
    report = {
        "schema": REPORT_SCHEMA,
        "conclusion": conclusion,
        "strict": strict,
        "counts": {
            "present": len(present),
            "missing": len(missing),
            "referencedButNoOriginalFile": len(no_original),
            "noConclusion": len(no_conclusion),
            "pakLfsPointer": len(pak_lfs_pointer),
            "pakIndexUnreadable": len(pak_index_unreadable),
            "caseMismatch": len(case_mismatch),
            "extensionSubstituted": len(extension_substituted),
        },
        "present": present,
        "missing": missing,
        "referencedButNoOriginalFile": no_original,
        "noConclusion": no_conclusion,
        "pakLfsPointer": pak_lfs_pointer,
        "pakIndexUnreadable": pak_index_unreadable,
        "caseMismatch": case_mismatch,
        "extensionSubstituted": extension_substituted,
        "excludedNotReferenced": {
            "hfl": [f"{number}.hfl" for number in NOT_REFERENCED_HFL],
            "ttb": [f"{number}.ttb" for number in NOT_REFERENCED_TTB],
        },
        "where": {
            "derive": (
                "ROG, --client-root D:/MX/client, --pak Map.pak; "
                "write under D:/build-mxr/tyj165/"
            ),
            "check": "CI and local checkouts, against the checked-in manifest only",
        },
    }
    if collisions:
        report["counts"]["pakBasenameCollisions"] = len(collisions)
        report["pakBasenameCollisions"] = collisions
    if local_state is not None:
        report["counts"]["localDecoder"] = len(local_rows)
        report["localDecoder"] = {
            "state": local_state,
            "version": local_info.get("version"),
            "references": local_rows,
            "note": {
                "verified": "local-decoder conclusions match the given pak",
                "unverified": "local-decoder conclusions, unverified by CI (no pak given); not a failure",
                "stale": "pak SHA-256 mismatch; local-decoder conclusions are not-run (stale)",
            }[local_state],
        }
    # A basename collision fails even without --strict: it changes which
    # bytes the runtime loads.
    report["exitCode"] = 1 if collisions else (0 if not strict or conclusion == "clean" else 1)
    return report


def format_report(report: dict) -> str:
    lines = [
        f"conclusion: {report['conclusion']}",
        "skipped or unresolved results are no conclusion, not a pass",
        (
            f"counts: present={report['counts']['present']} "
            f"missing={report['counts']['missing']} "
            f"referenced-but-no-original-file={report['counts']['referencedButNoOriginalFile']} "
            f"no-conclusion={report['counts']['noConclusion']} "
            f"pak-lfs-pointer={report['counts'].get('pakLfsPointer', 0)} "
            f"pak-index-unreadable={report['counts'].get('pakIndexUnreadable', 0)} "
            f"case-mismatch={report['counts'].get('caseMismatch', 0)} "
            f"extension-substituted={report['counts'].get('extensionSubstituted', 0)}"
        ),
        "derive runs on ROG; CI only checks the checked-in manifest",
    ]
    if "localDecoder" in report:
        info = report["localDecoder"]
        lines.append(f"local-decoder={report['counts']['localDecoder']} state={info['state']} ({info['note']})")
    lines += [
        f"strict switch {STRICT_ENV}: {'on' if report['strict'] else 'off'}",
    ]
    for row in report.get("pakBasenameCollisions", []):
        lines.append(f"collision: {row['name']} in {row['pak']} vs {row['other']} ({row['reason']})")
    for row in report["missing"]:
        lines.append(f"missing: {row['name']} ({row['reason']})")
    for row in report["referencedButNoOriginalFile"]:
        lines.append(
            f"referenced-by-original-but-no-original-file-exists: {row['name']} "
            "(informational, not a defect)"
        )
    for row in report["noConclusion"]:
        label = row["name"] or "(gate)"
        lines.append(f"no conclusion: {label}: {row['reason']}")
    for row in report.get("pakLfsPointer", []):
        lines.append(f"no conclusion (pak-lfs-pointer): {row['name']}")
    for row in report.get("pakIndexUnreadable", []):
        lines.append(f"no conclusion (pak-index-unreadable): {row['name']}")
    for row in report.get("extensionSubstituted", []):
        lines.append(
            f"extension-substituted: {row['name']} -> {', '.join(row['checkout'])} "
            "(fidelity gap, not found)"
        )
    for row in report.get("caseMismatch", []):
        lines.append(f"case-mismatch: {row['name']} -> {', '.join(row['checkout'])} (minor, found)")
    for row in report["present"]:
        lines.append(f"present: {row['name']}")
    lines.append(
        "not referenced by the original, not reported as missing: "
        + " ".join(report["excludedNotReferenced"]["hfl"] + report["excludedNotReferenced"]["ttb"])
    )
    return "\n".join(lines) + "\n"


def warning_lines(report: dict) -> list[str]:
    return [text for text, _severity in _warnings(report)]


def _warnings(report: dict) -> list[tuple[str, str]]:
    lines: list[tuple[str, str]] = []
    if report["conclusion"] == "no conclusion":
        lines.append(("resource reference check has no conclusion; this is not a pass", "info"))
    elif report["conclusion"] == "partial":
        lines.append(("resource reference check is partial; this is not a pass", "info"))
    for row in report.get("pakBasenameCollisions", []):
        lines.append((f"supplemental pak basename collision {row['name']} with {row['other']}", "info"))
    for row in report["missing"]:
        lines.append((f"missing referenced resource {row['name']}", "major"))
    for row in report["noConclusion"]:
        if row["name"]:
            lines.append((f"no conclusion for {row['name']}", "info"))
    pointer_rows = report.get("pakLfsPointer", [])
    if pointer_rows:
        paks = ", ".join(pointer_rows[0].get("paks", []))
        lines.append((
            f"no conclusion (pak-lfs-pointer) for {len(pointer_rows)} reference(s): "
            f"pak(s) {paks} are Git LFS pointers; not missing, not a pass",
            "info",
        ))
        unreadable = pointer_rows[0].get("unreadablePaks") or []
        if unreadable:
            lines.append((
                f"pak index unreadable for {', '.join(unreadable)} as well; "
                "those references are also covered by the pak-lfs-pointer no conclusion",
                "info",
            ))
    unreadable_rows = report.get("pakIndexUnreadable", [])
    if unreadable_rows:
        lines.append((
            f"no conclusion (pak-index-unreadable) for {len(unreadable_rows)} reference(s): "
            f"pak index of {', '.join(unreadable_rows[0]['paks'])} could not be read; "
            "not missing, not a pass",
            "info",
        ))
    for row in report.get("extensionSubstituted", []):
        lines.append((
            f"extension-substituted referenced resource {row['name']} "
            f"(checkout has {', '.join(row['checkout'])})",
            "minor",
        ))
    for row in report.get("caseMismatch", []):
        lines.append((
            f"case-mismatch referenced resource {row['name']} "
            f"(checkout has {', '.join(row['checkout'])})",
            "minor",
        ))
    return lines


def codequality_report(report: dict) -> list[dict]:
    issues = []
    for text, severity in _warnings(report):
        fingerprint = hashlib.sha256(text.encode("utf-8")).hexdigest()
        issues.append({
            "description": text,
            "check_name": "MxhResourceReference",
            "fingerprint": fingerprint,
            "severity": severity,
            "location": {
                "path": "reference/manifests/original-resource-references.json",
                "lines": {"begin": 1},
            },
        })
    return issues


def emit_warnings(report: dict) -> None:
    for text in warning_lines(report):
        print(f"WARNING: {text}", file=sys.stderr)
        # GitHub Actions annotation. Harmless in GitLab logs; GitLab's MR
        # warning surface is the codequality artifact written by --codequality.
        print(f"::warning::{text}")


TILE17_NAMES = tuple(f"tile_17_mou_a_{index:02d}.tga" for index in range(1, 10))
_DECODER_PATH_KEYS = {"decoderpath", "localdecoderpath", "pluginpath"}


def _is_py_path(value: str) -> bool:
    return value.replace("\\", "/").strip().lower().endswith(".py")


def _is_absolute_fs_path(value: str) -> bool:
    """Drive, UNC, or rooted path. Resource names such as ``a/b.tga`` are not."""
    text = value.strip()
    if len(text) >= 3 and text[0].isalpha() and text[1] == ":" and text[2] in "\\/":
        return True
    if text.startswith("\\\\") or text.startswith("//"):
        return True
    return text.startswith("/")


def _is_decoder_path(value: str) -> bool:
    """A decoder filesystem path, including any ``.py`` path."""
    if _is_py_path(value):
        return True
    text = value.replace("\\", "/").lower()
    return _is_absolute_fs_path(value) and ("decoder" in text or "plugin" in text)


def _strip_decoder_paths(node, *, keep_pak_path: bool = False) -> None:
    """Drop decoder paths. ``derivation.localDecoder`` and ``pak.path`` stay."""
    if isinstance(node, dict):
        for key in list(node.keys()):
            if key == "localDecoder" and isinstance(node[key], dict):
                _strip_local_decoder(node[key])
                continue
            if key == "pak" and isinstance(node[key], dict):
                _strip_decoder_paths(node[key], keep_pak_path=True)
                continue
            if keep_pak_path and key == "path":
                continue
            if key.casefold() in _DECODER_PATH_KEYS:
                node.pop(key, None)
                continue
            value = node[key]
            if isinstance(value, str) and _is_decoder_path(value):
                node.pop(key, None)
                continue
            _strip_decoder_paths(value)
    elif isinstance(node, list):
        kept = []
        for item in node:
            if isinstance(item, str) and _is_decoder_path(item):
                continue
            _strip_decoder_paths(item)
            kept.append(item)
        node[:] = kept


def _strip_local_decoder(node: dict) -> None:
    """Keep version, sourcePakSha256, decodedMaps, and keySourceCounts."""
    for key in list(node.keys()):
        if key in {"version", "sourcePakSha256", "keySourceCounts"}:
            continue
        if key == "decodedMaps" and isinstance(node[key], list):
            for row in node[key]:
                if not isinstance(row, dict):
                    continue
                for field in list(row.keys()):
                    if field in {"name", "sha256", "keySource"}:
                        continue
                    value = row[field]
                    if field.casefold() in _DECODER_PATH_KEYS or (
                        isinstance(value, str) and _is_decoder_path(value)
                    ):
                        row.pop(field, None)
                    else:
                        _strip_decoder_paths(value)
            continue
        value = node[key]
        if key.casefold() in _DECODER_PATH_KEYS or (
            isinstance(value, str) and _is_decoder_path(value)
        ):
            node.pop(key, None)
        else:
            _strip_decoder_paths(value)


def trim_manifest(document: dict) -> dict:
    """Copy with decoder filesystem paths and ``.py`` paths removed.

    ``derivation.localDecoder`` stays, including ``version``,
    ``sourcePakSha256``, ``decodedMaps`` (name and sha256), and
    ``keySourceCounts``. ``check --pak`` reads that node. ``pak.path``
    stays. Two derives of the same inputs trim to the same bytes.
    """
    trimmed = copy.deepcopy(document)
    _strip_decoder_paths(trimmed)
    return trimmed


def _decoded_map_names(document: dict) -> list[str]:
    info = (document.get("derivation") or {}).get("localDecoder") or {}
    names: list[str] = []
    for row in info.get("decodedMaps") or []:
        if isinstance(row, dict) and isinstance(row.get("name"), str) and row["name"]:
            names.append(row["name"])
    return names


def _strip_hit_fields(document: dict) -> dict:
    out = copy.deepcopy(document)
    for key in ("references", "excludedNotReferenced"):
        rows = out.get(key)
        if not isinstance(rows, list):
            continue
        for row in rows:
            if isinstance(row, dict):
                row.pop("originalExists", None)
                row.pop("originalHit", None)
    return out


def _diff_paths(old, new, path: str, diffs: list[str]) -> None:
    if isinstance(old, dict) and isinstance(new, dict):
        for key in sorted(set(old) | set(new)):
            child = f"{path}.{key}" if path else str(key)
            if key not in old or key not in new:
                diffs.append(child)
            else:
                _diff_paths(old[key], new[key], child, diffs)
        return
    if isinstance(old, list) and isinstance(new, list):
        if len(old) != len(new):
            diffs.append(f"{path}.length")
        for index in range(min(len(old), len(new))):
            _diff_paths(old[index], new[index], f"{path}[{index}]", diffs)
        return
    if old != new:
        diffs.append(path or "$")


def _count_lines(prefix: str, counts: dict[str, int]) -> list[str]:
    return [f"{prefix} {key} {counts[key]}" for key in sorted(counts)]


def compare_manifest_report(old: dict, new: dict) -> str:
    """Names and counts only. No pak bytes, map text, or hashes.

    ``originalExists`` and ``originalHit`` are the fields a client-root
    re-derive is allowed to change. Every other field is counted.
    """
    diffs: list[str] = []
    _diff_paths(_strip_hit_fields(old), _strip_hit_fields(new), "", diffs)
    lines = [f"OTHER_FIELD_DIFFS {len(diffs)}"]
    lines.extend(f"OTHER_FIELD {path}" for path in diffs)
    old_count = (old.get("derivation") or {}).get("parsedMapCount")
    new_count = (new.get("derivation") or {}).get("parsedMapCount")
    lines.append(f"PARSED_MAP_COUNT old={old_count} new={new_count}")
    old_maps = set(_decoded_map_names(old))
    new_maps = set(_decoded_map_names(new))
    if old_maps == new_maps:
        lines.append("DECODED_MAPS_EQUAL")
    else:
        lines.extend(f"DECODED_MAPS_ONLY_OLD {name}" for name in sorted(old_maps - new_maps))
        lines.extend(f"DECODED_MAPS_ONLY_NEW {name}" for name in sorted(new_maps - old_maps))
    by_ext: dict[str, int] = {}
    by_via: dict[str, int] = {}
    for row in new.get("references") or []:
        if not isinstance(row, dict) or row.get("originalExists") is not False:
            continue
        name = str(row.get("name") or "")
        base = _basename(name)
        ext = base.rsplit(".", 1)[-1].casefold() if "." in base else "(none)"
        by_ext[ext] = by_ext.get(ext, 0) + 1
        via = str(row.get("via") or "") or "(empty)"
        by_via[via] = by_via.get(via, 0) + 1
    lines.extend(_count_lines("FALSE_BY_EXT", by_ext))
    lines.extend(_count_lines("FALSE_BY_VIA", by_via))
    by_name: dict[str, dict] = {}
    for row in new.get("references") or []:
        if isinstance(row, dict) and row.get("name"):
            by_name.setdefault(_casefold_name(str(row["name"])), row)
    for name in TILE17_NAMES:
        row = by_name.get(name.casefold())
        if row is None or row.get("originalExists") not in (True, False):
            state = "absent"
        else:
            state = "true" if row.get("originalExists") is True else "false"
        lines.append(f"TILE {name} {state}")
    return "\n".join(lines) + "\n"


def _load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _write_json(path: Path, document) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    derive_parser = sub.add_parser("derive", help="run on ROG; write the name manifest")
    derive_parser.add_argument(
        "--source", type=Path,
        help="loose files and the directory walk used to discover maps, together with --pak. "
             "Other .pak files under here do not add maps. ROG: D:\\MX\\client\\Package\\3DData")
    derive_parser.add_argument(
        "--client-root", type=Path, default=None,
        help="originalExists scope (ROG: D:\\MX\\client). Loose files are keyed relative to this "
             "directory and only .pak files under it count. Defaults to --source. "
             "The known-no-original guard uses this same index. "
             "An explicit path that does not exist or is not a directory is an error.")
    derive_parser.add_argument(
        "--pak", type=Path,
        help="original Map.pak (read to EOF, not the 4192-entry worktree pak). "
             "Its entries supply map names. Header counts are recorded. "
             "It counts for originalExists only when it is under --client-root.")
    derive_parser.add_argument("--output", type=Path, required=True)
    derive_parser.add_argument(
        "--local-decoder", type=Path, default=None,
        help=f"local only, never in CI: decoder plugin .py outside the repo (or {LOCAL_DECODER_ENV}); "
             "no default search path")
    derive_parser.add_argument(
        "--local-header-stats", action="store_true", default=False,
        help="local only, never in CI: print obfuscated-map header byte 2 counts to "
             "stdout; nothing is written to the manifest")

    check_parser = sub.add_parser("check", help="compare a checkout to the manifest; warn-only unless --strict")
    check_parser.add_argument("--manifest", type=Path, required=True)
    check_parser.add_argument("--root", type=Path, required=True)
    check_parser.add_argument("--report", type=Path)
    check_parser.add_argument("--codequality", type=Path)
    check_parser.add_argument("--pak", type=Path, default=None,
                              help="optional original Map.pak, only hashed to verify local-decoder conclusions")
    check_parser.add_argument("--strict", action="store_true",
                              help=f"same switch as {STRICT_ENV}=1; fail on missing files or no conclusion")

    trim_parser = sub.add_parser(
        "trim", help="drop decoder filesystem paths; keep derivation.localDecoder")
    trim_parser.add_argument("--input", type=Path, required=True)
    trim_parser.add_argument("--output", type=Path, required=True)

    compare_parser = sub.add_parser(
        "compare-manifests",
        help="compare two manifests; names and counts only, no file contents")
    compare_parser.add_argument("old", type=Path)
    compare_parser.add_argument("new", type=Path)
    args = parser.parse_args(argv)

    if args.command == "derive":
        if args.source is None and args.pak is None:
            parser.error("derive needs --source and/or --pak on the machine that has the original files")
        header_stats: dict[int, int] | None = {} if args.local_header_stats else None
        decoder_path = resolve_local_decoder_path(args.local_decoder)
        decoder = None
        if decoder_path is not None and args.pak is None:
            parser.error(f"--local-decoder (or {LOCAL_DECODER_ENV}) requires --pak so the "
                         "manifest records the source pak SHA-256")
        if decoder_path is not None:
            try:
                decoder = load_local_decoder(decoder_path)
            except LocalDecoderError as exc:
                parser.error(str(exc))
        try:
            document = derive(
                args.source, args.pak, local_header_stats=header_stats, local_decoder=decoder,
                client_root=args.client_root)
        except ClientRootError as exc:
            parser.error(str(exc))
        except KnownNoOriginalFound as exc:
            print(str(exc), file=sys.stderr)
            return 1
        _write_json(args.output, document)
        print(
            f"wrote {args.output} status={document['derivation']['status']} "
            f"references={len(document['references'])} "
            f"pakEntries={document['derivation']['pak'].get('entriesReadToEof')}"
        )
        if header_stats is not None:
            print("local-only obfuscated-map header byte 2 counts (do not commit or paste into CI):")
            for value, count in sorted(header_stats.items()):
                print(f"  0x{value:02x} {count}")
        # A known-no-original hit already returned 1 without writing.
        # check is warn-only by design unless the strict switch is set.
        return 0

    if args.command == "trim":
        trimmed = trim_manifest(_load_json(args.input))
        _write_json(args.output, trimmed)
        print(f"trimmed {args.output}")
        return 0

    if args.command == "compare-manifests":
        print(compare_manifest_report(_load_json(args.old), _load_json(args.new)), end="")
        return 0

    strict = strict_enabled(args.strict)
    report = check(_load_json(args.manifest), args.root, strict=strict, pak=args.pak)
    text = format_report(report)
    print(text, end="")
    emit_warnings(report)
    if args.report:
        _write_json(args.report, report)
    if args.codequality:
        _write_json(args.codequality, codequality_report(report))
    return int(report["exitCode"])


if __name__ == "__main__":
    raise SystemExit(main())
