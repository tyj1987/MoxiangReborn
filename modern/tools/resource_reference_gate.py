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
* ``caseMismatch`` — found only after case folding (Windows matching is
  case-insensitive). Counted as present, listed separately, severity minor.
* ``extensionSubstituted`` — not found by name, but the same stem exists with
  another extension (``x.tga`` vs ``x.dds``). A fidelity gap: not found, not
  counted as missing, own counter and codequality entry (minor).

Height fields 0, 4, 23, 24, 32, 33, 34, 94 and tile tables 0, 3, 4, 23, 24,
32, 33, 34, 94, 198 are not referenced by the original and are never reported
as missing.

The job exits 0 unless the single switch ``MXH_RESOURCE_REF_STRICT=1``
(or ``--strict``) is set. That switch is what later turns missing files and
an unresolved conclusion into a failing exit.
"""

from __future__ import annotations

import argparse
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

# Tokens printed by the original client EngineMap.cpp loader
# (MHClient-Connect.exe). A .map reference is one of these sections followed
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


def _basename(name: str) -> str:
    return name.replace("\\", "/").rstrip("/").split("/")[-1]


def _casefold_name(name: str) -> str:
    return _basename(name).casefold()


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


def _read_pak_entries(pak_path: Path) -> tuple[int, list] | None:
    """Return the offset-4 field and the entries of the one valid layout.

    ``unpack_pak.read_entries`` walks both layouts and keeps the single walk
    that ends at EOF.  92-byte PlayDH: count is the uint32 at offset 4,
    entries start at 92.  96-byte original: count is the uint32 at offset
    92, entries start at 96.  Original Map.pak stores 0 at offset 4 and the
    real count (11267) at offset 92.  Both layouts, or neither, prints one
    ERROR line naming the pak and the caller skips it.  The count passed to
    the walker is the layout's own field, never an inflated cap.
    """
    try:
        _kind, offset4, _declared, entries = unpack_pak.read_entries(str(pak_path))
    except (OSError, ValueError, struct.error) as exc:
        detail = str(exc)
        if str(pak_path) not in detail:
            detail = f"{pak_path}: {detail}"
        print(f"ERROR: {detail}", file=sys.stderr)
        return None
    return int(offset4), entries


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


class _BlobIndex:
    def __init__(self) -> None:
        self.by_base: dict[str, list[tuple[str, bytes | None, Path | None]]] = {}

    def add(self, name: str, data: bytes | None, path: Path | None) -> None:
        self.by_base.setdefault(_casefold_name(name), []).append((name, data, path))

    def find(self, name: str):
        return self.by_base.get(_casefold_name(name), [])


def _build_source_index(source: Path | None, pak: Path | None) -> tuple[_BlobIndex, dict]:
    index = _BlobIndex()
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
            # Keep entry objects so referenced containers can be loaded once.
            pak_info["entries"] = entries
            pak_info["pakPath"] = pak
    return index, pak_info


def _load_named(index: _BlobIndex, pak_info: dict, name: str) -> bytes | None:
    for stored, data, path in index.find(name):
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


def _original_exists(index: _BlobIndex, name: str) -> bool:
    return bool(index.find(name))


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
    ``decode_map(name: str, data: bytes) -> bytes | None``. The repository
    ships no decoder; a path inside ``repo_root`` is refused.
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
    return module


def _unparsed_summary(unparsed: list[dict]) -> dict:
    by_reason: dict[str, int] = {}
    for item in unparsed:
        reason = str(item.get("reason", ""))
        by_reason[reason] = by_reason.get(reason, 0) + 1
    return {"total": len(unparsed), "byReason": dict(sorted(by_reason.items()))}


def derive(source: Path | None, pak: Path | None = None,
           known_no_original: tuple[str, ...] = KNOWN_NO_ORIGINAL,
           local_header_stats: dict[int, int] | None = None,
           local_decoder=None) -> dict:
    """Build a name-only manifest from original .map / .stm / .hfl bytes.

    ``local_header_stats``, when a dict is passed (local CLI flag only), is
    filled in memory with per-file header byte 2 -> count for obfuscated
    maps. It is never written into the returned manifest.

    ``local_decoder`` is a module loaded by ``load_local_decoder``. Its
    plaintext output is parsed like a plaintext map; only names, the decoder
    version, the --pak SHA-256 and each decoded original .map SHA-256 are
    recorded. References from decoded maps carry ``derivedBy:
    "local-decoder"``.
    """
    if local_decoder is not None and pak is None:
        raise LocalDecoderError(
            "--local-decoder requires --pak: local-decoder conclusions must record the "
            "source pak SHA-256 so check --pak can verify them")
    index, pak_info = _build_source_index(source, pak)
    references: list[dict] = []
    excluded: list[dict] = []
    unparsed: list[dict] = []
    seen_maps = 0
    consumed: set[str] = set()
    decoded_maps: list[dict] = []

    map_names = [name for name in index.by_base if name.endswith(".map")]
    for map_name in sorted(map_names):
        data = _load_named(index, pak_info, map_name)
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
            decoded_maps.append({"name": map_name, "sha256": hashlib.sha256(data).hexdigest()})
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
            payload = _load_named(index, pak_info, ref["name"])
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
    return document


def _absorb(references: list[dict], excluded: list[dict], refs: list[dict],
            via: str, index: _BlobIndex, derived_by: str | None = None) -> None:
    have = {_casefold_name(item["name"]) for item in references}
    have_excluded = {_casefold_name(item["name"]) for item in excluded}
    for ref in refs:
        key = _casefold_name(ref["name"])
        kind = excluded_resource(ref["name"])
        row = {
            "name": ref["name"],
            "via": via,
            "section": ref["section"],
            "originalExists": _original_exists(index, ref["name"]),
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
    A file is a pointer iff its content starts with ``LFS_PREFIX``; size is
    never used.
    """

    def __init__(self) -> None:
        self.names: dict[str, set[str]] = {}
        self.stems: dict[str, set[str]] = {}
        self.pointers: set[str] = set()
        self.pointer_paks: list[str] = []

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
    def from_names(cls, names, pointer_names=(), pointer_paks=()) -> "_CheckoutIndex":
        """Build an index without a filesystem (tests and offline harnesses)."""
        index = cls()
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
                continue
            _header_count, entries = read
            for entry in entries:
                index.add(entry.canonical_name)
    return index


def check(manifest: dict, root: Path, strict: bool = False, pak: Path | None = None) -> dict:
    """Compare a checkout to the manifest. Never loads a decoder plugin.

    ``pak`` is only hashed, to verify ``derivation.localDecoder``.
    """
    return check_index(manifest, _checkout_index(root), strict=strict, pak=pak)


def check_index(manifest: dict, index: _CheckoutIndex, strict: bool = False,
                pak: Path | None = None) -> dict:
    """``check`` against a prebuilt checkout index.

    Resolution order for a reference whose original exists:

    1. exact-case basename in the index -> ``present``;
    2. same name after case folding only -> ``present`` and also listed in
       ``caseMismatch`` (severity minor; Windows matching is case-insensitive);
    3. the basename itself is an LFS pointer file -> ``noConclusion``;
    4. one or more .pak files under root are LFS pointers -> ``pakLfsPointer``
       (reason ``pak-lfs-pointer``). There is no reliable ref-to-pak mapping
       (the manifest does not record the containing pak, and the same name
       family is spread across Map/Character/Effect/monster), so ANY
       unresolved reference is treated as possibly inside ANY pointer pak.
       No conclusion: neither missing nor present;
    5. same stem with a different extension (``x.tga`` vs ``x.dds``) ->
       ``extensionSubstituted``: a fidelity gap, not found, not missing;
    6. otherwise -> ``missing``.

    Step 4 precedes step 5 because with pointer paks the exact name may still
    be inside one, so an extension substitute is not yet a conclusion.
    """
    if manifest.get("schema") != MANIFEST_SCHEMA:
        raise ValueError("unsupported resource-reference manifest schema")
    derivation = manifest.get("derivation") or {}
    status = derivation.get("status", "not-run")
    known = list(manifest.get("knownReferencedNoOriginalFile") or KNOWN_NO_ORIGINAL)
    references = list(manifest.get("references") or [])
    pointer_names = index.pointers
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

    if status == "partial":
        conclusion = "partial"
    elif no_conclusion or pak_lfs_pointer:
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
            "caseMismatch": len(case_mismatch),
            "extensionSubstituted": len(extension_substituted),
        },
        "present": present,
        "missing": missing,
        "referencedButNoOriginalFile": no_original,
        "noConclusion": no_conclusion,
        "pakLfsPointer": pak_lfs_pointer,
        "caseMismatch": case_mismatch,
        "extensionSubstituted": extension_substituted,
        "excludedNotReferenced": {
            "hfl": [f"{number}.hfl" for number in NOT_REFERENCED_HFL],
            "ttb": [f"{number}.ttb" for number in NOT_REFERENCED_TTB],
        },
        "where": {
            "derive": "ROG, against the original .map/.stm/.hfl files and D:/MX/client/Package/3DData/Map.pak",
            "check": "CI and local checkouts, against the checked-in manifest only",
        },
    }
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
    report["exitCode"] = 0 if not strict or conclusion == "clean" else 1
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


def _load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _write_json(path: Path, document) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    derive_parser = sub.add_parser("derive", help="run on ROG; write the name manifest")
    derive_parser.add_argument("--source", type=Path, help="directory of original .map/.stm/.hfl files")
    derive_parser.add_argument("--pak", type=Path, help="original Map.pak (read to EOF, not the 4192-entry worktree pak)")
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
        document = derive(args.source, args.pak, local_header_stats=header_stats, local_decoder=decoder)
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
        # derive always exits 0. check is warn-only by design unless the
        # strict switch is set.
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
