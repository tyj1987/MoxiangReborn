#!/usr/bin/env python3
"""Build and validate a deterministic Unity remaster resource manifest.

The source tree is never modified. Loose files and entries from the seven
legacy PAKs are hashed in place. A logical asset can have several physical
sources, but this tool never applies a loose-first (or any other) implicit
precedence rule: ambiguous assets require an explicit selection document.
"""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
import unicodedata
from collections import Counter, defaultdict
from pathlib import Path, PurePosixPath
from typing import Any, Iterable

from gen_hfl_placeholders import (
    HEADER_BYTES,
    PLACEHOLDER_GRID,
    parse_hfl_header,
    synthesize_heights,
)
from unpack_pak import parse_pak, walk_entries


TOOL_NAME = "unity_resource_audit"
TOOL_VERSION = "1.0.0"
SCHEMA_VERSION = 1
REQUIRED_PAKS = (
    "Map.pak",
    "Character.pak",
    "monster.pak",
    "npc.pak",
    "Effect.pak",
    "Titan.pak",
    "Pet.pak",
)
PAK_NAMESPACES = {
    "map.pak": "Map",
    "character.pak": "Character",
    "monster.pak": "Monster",
    "npc.pak": "Npc",
    "effect.pak": "Effect",
    "titan.pak": "Titan",
    "pet.pak": "Pet",
}
HEX_SHA256 = re.compile(r"^[0-9a-f]{64}$")
HEX_COMMIT = re.compile(r"^[0-9a-f]{7,64}$")


class AuditError(RuntimeError):
    """The audit could not safely produce or consume a manifest."""


def _sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _stable_id(kind: str, *parts: object) -> str:
    value = "\0".join((kind, *(str(part) for part in parts)))
    return "mxh:" + hashlib.sha256(value.encode("utf-8")).hexdigest()


def normalize_relative_path(raw: str) -> str:
    """Return a portable relative path or reject traversal/Windows escapes."""
    if not raw or "\x00" in raw:
        raise ValueError("empty or NUL-containing path")
    normalized = unicodedata.normalize("NFC", raw.replace("\\", "/"))
    if normalized.startswith("/") or re.match(r"^[A-Za-z]:", normalized):
        raise ValueError("absolute path")
    path = PurePosixPath(normalized)
    if path.is_absolute() or not path.parts:
        raise ValueError("absolute or empty path")
    if any(part in ("", ".", "..") for part in path.parts):
        raise ValueError("dot or parent path component")
    if any(":" in part for part in path.parts):
        raise ValueError("Windows alternate-stream or drive component")
    return "/".join(path.parts)


def logical_path_for_loose(relative_path: str) -> str:
    path = normalize_relative_path(relative_path)
    parts = path.split("/")
    if len(parts) >= 3 and parts[0].casefold() == "resource" and parts[1].casefold() == "map":
        return "Map/" + "/".join(parts[2:])
    return path


def logical_path_for_pak(pak_name: str, entry_name: str) -> str:
    path = normalize_relative_path(entry_name)
    namespace = PAK_NAMESPACES[pak_name.casefold()]
    first = path.split("/", 1)[0]
    return path if first.casefold() == namespace.casefold() else f"{namespace}/{path}"


def classify_hfl(logical_path: str, data: bytes) -> dict[str, Any] | None:
    """Detect this repository's generated HFL fingerprint from content.

    A numeric filename only supplies the generator seed. It is never enough
    to classify an asset: the grid, extents and every generated height must
    also match exactly. Any non-matching HFL remains unknown provenance.
    """
    if not logical_path.casefold().endswith(".hfl"):
        return None
    stem = PurePosixPath(logical_path).stem
    if not stem.isdecimal():
        return {"kind": "unknown", "detector": "hfl-content-v1", "reason": "non-numeric map id"}
    map_id = int(stem)
    try:
        _version, _desc, hcx, hcz, width, height, _textures, _heights = parse_hfl_header(data)
    except (ValueError, struct.error) as exc:
        return {"kind": "unknown", "detector": "hfl-content-v1", "reason": f"unparsed HFL: {exc}"}
    expected_width = float(2000 + (map_id % 64) * 50)
    expected_height = float(2000 + ((map_id * 7) % 64) * 50)
    generated_grid = hcx == PLACEHOLDER_GRID and hcz == PLACEHOLDER_GRID
    generated_extent = width == expected_width and height == expected_height
    height_end = HEADER_BYTES + hcx * hcz * 4
    expected_heights = struct.pack(
        f"<{PLACEHOLDER_GRID * PLACEHOLDER_GRID}f",
        *synthesize_heights(map_id, PLACEHOLDER_GRID, PLACEHOLDER_GRID),
    )
    generated_heights = generated_grid and data[HEADER_BYTES:height_end] == expected_heights
    if generated_grid and generated_extent and generated_heights:
        return {
            "kind": "generated-placeholder",
            "detector": "gen_hfl_placeholders-v1",
            "evidence": ["16x16-grid", "map-id-derived-extents", "exact-generated-height-sequence"],
        }
    return {
        "kind": "unknown",
        "detector": "hfl-content-v1",
        "reason": "does not match the complete generated-placeholder fingerprint",
    }


def _source_record(*, source_type: str, logical_path: str, physical_path: str,
                   size: int, sha256: str, container: str | None = None,
                   index: int | None = None, offset: int | None = None,
                   classification: dict[str, Any] | None = None) -> dict[str, Any]:
    logical_key = unicodedata.normalize("NFC", logical_path).casefold()
    source_key = f"{source_type}:{physical_path}"
    if index is not None:
        source_key += f"#{index}"
    record: dict[str, Any] = {
        "sourceId": _stable_id("mxh-source-v1", source_key),
        "sourceType": source_type,
        "physicalPath": physical_path,
        "bytes": size,
        "sha256": sha256,
    }
    if container is not None:
        record["container"] = container
    if index is not None:
        record["entryIndex"] = index
    if offset is not None:
        record["entryOffset"] = offset
    if classification is not None:
        record["provenance"] = classification
    record["_logicalPath"] = logical_path
    record["_logicalKey"] = logical_key
    return record


def _load_selection(path: Path | None, profile_id: str) -> dict[str, Any]:
    if path is None:
        return {"selections": {}, "attestations": {}}
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise AuditError(f"selection document cannot be read: {exc}") from exc
    if document.get("schemaVersion") != 1 or document.get("profileId") != profile_id:
        raise AuditError("selection schemaVersion/profileId does not match the audit")
    if not isinstance(document.get("selections", {}), dict):
        raise AuditError("selection 'selections' must be an object")
    if not isinstance(document.get("attestations", {}), dict):
        raise AuditError("selection 'attestations' must be an object")
    return document


def _discover_commit() -> str:
    repository = Path(__file__).resolve().parents[2]
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=repository,
            check=True, capture_output=True, text=True, timeout=10,
        )
        commit = result.stdout.strip().lower()
        return commit if HEX_COMMIT.fullmatch(commit) else "unknown"
    except (OSError, subprocess.SubprocessError):
        return "unknown"


def _iter_loose_files(root: Path, unsafe: list[dict[str, str]],
                      source_label: str = "loose") -> Iterable[tuple[Path, str]]:
    root_resolved = root.resolve(strict=True)
    for base, dirs, files in os.walk(root, followlinks=False):
        base_path = Path(base)
        safe_dirs: list[str] = []
        for name in sorted(dirs, key=lambda value: (value.casefold(), value)):
            candidate = base_path / name
            if candidate.is_symlink():
                unsafe.append({"source": source_label, "path": str(candidate.relative_to(root)), "reason": "directory symlink"})
            else:
                safe_dirs.append(name)
        dirs[:] = safe_dirs
        for name in sorted(files, key=lambda value: (value.casefold(), value)):
            candidate = base_path / name
            relative = candidate.relative_to(root).as_posix()
            try:
                resolved = candidate.resolve(strict=True)
                resolved.relative_to(root_resolved)
                normalized = normalize_relative_path(relative)
            except (OSError, ValueError) as exc:
                unsafe.append({"source": source_label, "path": relative, "reason": str(exc)})
                continue
            if candidate.is_symlink():
                unsafe.append({"source": source_label, "path": relative, "reason": "file symlink"})
                continue
            yield candidate, normalized


def _apply_attestation(source: dict[str, Any], attestation: Any,
                       issues: list[dict[str, str]]) -> None:
    if source.get("provenance", {}).get("kind") == "generated-placeholder":
        issues.append({"code": "placeholder-attestation", "subject": source["sourceId"],
                       "detail": "content-proven placeholders cannot be attested as original"})
        return
    if not isinstance(attestation, dict):
        issues.append({"code": "invalid-attestation", "subject": source["sourceId"], "detail": "attestation must be an object"})
        return
    if (attestation.get("classification") != "verified-original" or
            attestation.get("sha256") != source["sha256"] or
            not isinstance(attestation.get("evidence"), str) or
            not attestation["evidence"].strip()):
        issues.append({"code": "invalid-attestation", "subject": source["sourceId"], "detail": "verified-original requires matching sha256 and non-empty evidence"})
        return
    source["provenance"] = {
        "kind": "verified-original",
        "detector": "explicit-attestation-v1",
        "evidence": attestation["evidence"].strip(),
    }


def build_manifest(root: Path, profile_id: str, source_commit: str | None = None,
                   selection_path: Path | None = None,
                   additional_roots: list[tuple[str, Path]] | None = None) -> dict[str, Any]:
    root = root.resolve(strict=True)
    if not root.is_dir():
        raise AuditError(f"resource root is not a directory: {root}")
    selection = _load_selection(selection_path, profile_id)
    selections = selection.get("selections", {})
    attestations = selection.get("attestations", {})
    issues: list[dict[str, str]] = []
    unsafe: list[dict[str, str]] = []
    sources: list[dict[str, Any]] = []
    resolved_additional_roots: list[tuple[str, Path]] = []
    for label, additional_root in additional_roots or []:
        if not re.fullmatch(r"[a-z0-9][a-z0-9._-]*", label):
            raise AuditError(f"invalid additional root label: {label}")
        if any(existing == label for existing, _ in resolved_additional_roots):
            raise AuditError(f"duplicate additional root label: {label}")
        resolved = additional_root.resolve(strict=True)
        if not resolved.is_dir():
            raise AuditError(f"additional root is not a directory: {resolved}")
        resolved_additional_roots.append((label, resolved))

    root_files = defaultdict(list)
    for item in root.iterdir():
        if item.is_file() and item.suffix.casefold() == ".pak":
            root_files[item.name.casefold()].append(item)

    packs: list[dict[str, Any]] = []
    required_keys = {name.casefold() for name in REQUIRED_PAKS}
    for required in REQUIRED_PAKS:
        matches = root_files.get(required.casefold(), [])
        if len(matches) != 1:
            issues.append({"code": "missing-or-ambiguous-pak", "subject": required,
                           "detail": f"expected exactly one file, found {len(matches)}"})
            continue
        pak = matches[0]
        record: dict[str, Any] = {
            "path": pak.name,
            "bytes": pak.stat().st_size,
            "sha256": _sha256_file(pak),
        }
        parsed_count = 0
        try:
            _header, version, expected, flag = parse_pak(str(pak))
            record.update({"version": version, "headerEntryCount": expected, "flag": flag})
            with pak.open("rb") as stream:
                for entry in walk_entries(str(pak), expected):
                    parsed_count += 1
                    try:
                        logical = logical_path_for_pak(pak.name, entry.name)
                    except ValueError as exc:
                        unsafe.append({"source": f"pak:{pak.name}#{entry.index}", "path": entry.name, "reason": str(exc)})
                        continue
                    stream.seek(entry.data_offset)
                    data = stream.read(entry.real_size)
                    if len(data) != entry.real_size:
                        issues.append({"code": "pak-entry-truncated", "subject": f"{pak.name}#{entry.index}", "detail": entry.name})
                        continue
                    physical = f"{pak.name}!/{normalize_relative_path(entry.name)}"
                    sources.append(_source_record(
                        source_type="pak", logical_path=logical, physical_path=physical,
                        container=pak.name, index=entry.index, offset=entry.data_offset,
                        size=len(data), sha256=_sha256_bytes(data),
                        classification=classify_hfl(logical, data),
                    ))
        except (OSError, ValueError) as exc:
            issues.append({"code": "pak-read-error", "subject": pak.name, "detail": str(exc)})
        record["parsedEntryCount"] = parsed_count
        if record.get("headerEntryCount") != parsed_count:
            issues.append({"code": "pak-entry-count-mismatch", "subject": pak.name,
                           "detail": f"header={record.get('headerEntryCount')} parsed={parsed_count}"})
        packs.append(record)

    for unexpected in sorted((key for key in root_files if key not in required_keys)):
        issues.append({"code": "unexpected-pak", "subject": unexpected, "detail": "not in the seven-pack profile"})

    for path, relative in _iter_loose_files(root, unsafe):
        if path.suffix.casefold() == ".pak":
            continue
        try:
            data = path.read_bytes()
            logical = logical_path_for_loose(relative)
            sources.append(_source_record(
                source_type="loose", logical_path=logical, physical_path=relative,
                size=len(data), sha256=_sha256_bytes(data),
                classification=classify_hfl(logical, data),
            ))
        except (OSError, ValueError) as exc:
            issues.append({"code": "loose-read-error", "subject": relative, "detail": str(exc)})

    for label, additional_root in resolved_additional_roots:
        source_type = f"overlay:{label}"
        for path, relative in _iter_loose_files(additional_root, unsafe, source_type):
            if path.name.casefold() in ("manifest.json", "provenance.json"):
                continue
            if path.suffix.casefold() == ".pak":
                issues.append({"code": "overlay-pak-not-supported", "subject": label,
                               "detail": relative})
                continue
            try:
                data = path.read_bytes()
                logical = logical_path_for_loose(relative)
                sources.append(_source_record(
                    source_type=source_type, logical_path=logical,
                    physical_path=relative, size=len(data),
                    sha256=_sha256_bytes(data),
                    classification=classify_hfl(logical, data),
                ))
            except (OSError, ValueError) as exc:
                issues.append({"code": "overlay-read-error", "subject": f"{label}:{relative}",
                               "detail": str(exc)})

    for source in sources:
        if source["sourceId"] in attestations:
            _apply_attestation(source, attestations[source["sourceId"]], issues)

    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for source in sources:
        grouped[source.pop("_logicalKey")].append(source)

    assets: list[dict[str, Any]] = []
    duplicates: list[dict[str, Any]] = []
    conflicts: list[dict[str, Any]] = []
    used_selections: set[str] = set()
    for logical_key in sorted(grouped):
        members = sorted(grouped[logical_key], key=lambda item: item["sourceId"])
        logical_paths = sorted({member.pop("_logicalPath") for member in members}, key=lambda value: (value.casefold(), value))
        logical_path = logical_paths[0]
        stable_id = _stable_id("mxh-resource-v1", logical_key)
        hashes = sorted({member["sha256"] for member in members})
        relation = "unique" if len(members) == 1 else ("duplicate" if len(hashes) == 1 else "conflict")
        selected = selections.get(stable_id)
        if selected is not None:
            used_selections.add(stable_id)
            if selected not in {member["sourceId"] for member in members}:
                issues.append({"code": "invalid-source-selection", "subject": stable_id, "detail": str(selected)})
                selected = None
        asset = {
            "stableId": stable_id,
            "logicalPath": logical_path,
            "logicalAliases": logical_paths[1:],
            "relation": relation,
            "requiresSelection": len(members) > 1,
            "selectedSourceId": selected,
            "sources": members,
        }
        assets.append(asset)
        summary = {"stableId": stable_id, "logicalPath": logical_path,
                   "sourceIds": [member["sourceId"] for member in members], "sha256": hashes}
        if relation == "duplicate":
            duplicates.append(summary)
        elif relation == "conflict":
            conflicts.append(summary)

    for unused in sorted(set(selections) - used_selections):
        issues.append({"code": "selection-for-unknown-asset", "subject": unused, "detail": str(selections[unused])})
    for unused in sorted(set(attestations) - {source["sourceId"] for source in sources}):
        issues.append({"code": "attestation-for-unknown-source", "subject": unused, "detail": "sourceId not present"})
    unsafe.sort(key=lambda item: (item["source"].casefold(), item["path"].casefold(), item["reason"]))
    issues.sort(key=lambda item: (item["code"], item["subject"], item["detail"]))

    extension_counts: Counter[str] = Counter()
    source_counts: Counter[str] = Counter()
    hfl_counts: Counter[str] = Counter()
    map_ids: set[int] = set()
    hfl_ids: set[int] = set()
    ttb_ids: set[int] = set()
    for asset in assets:
        suffix = PurePosixPath(asset["logicalPath"]).suffix.casefold() or "(none)"
        extension_counts[suffix] += 1
        for source in asset["sources"]:
            source_counts[source["sourceType"]] += 1
            provenance = source.get("provenance")
            if suffix == ".hfl" and provenance:
                hfl_counts[provenance["kind"]] += 1
        stem = PurePosixPath(asset["logicalPath"]).stem
        if suffix == ".bmhm":
            match = re.fullmatch(r"map(\d+)", stem, re.IGNORECASE)
            if match:
                map_ids.add(int(match.group(1)))
        elif suffix == ".hfl" and stem.isdecimal():
            hfl_ids.add(int(stem))
        elif suffix == ".ttb" and stem.isdecimal():
            ttb_ids.add(int(stem))

    manifest: dict[str, Any] = {
        "schemaVersion": SCHEMA_VERSION,
        "tool": {"name": TOOL_NAME, "version": TOOL_VERSION},
        "profileId": profile_id,
        "source": {"root": ".", "commit": (source_commit or _discover_commit()).lower(),
                   "additionalRoots": [label for label, _ in resolved_additional_roots]},
        "policy": {
            "sourcePrecedence": "none",
            "ambiguousSourceSelection": "explicit-source-id-required",
            "hflUnattestedClassification": "unknown",
        },
        "packs": sorted(packs, key=lambda item: item["path"].casefold()),
        "assets": assets,
        "duplicates": duplicates,
        "conflicts": conflicts,
        "unsafePaths": unsafe,
        "issues": issues,
        "coverage": {
            "logicalAssetCount": len(assets),
            "physicalSourceCount": len(sources),
            "sourceCounts": dict(sorted(source_counts.items())),
            "extensionCounts": dict(sorted(extension_counts.items())),
            "duplicateAssetCount": len(duplicates),
            "conflictAssetCount": len(conflicts),
            "unresolvedSelectionCount": sum(1 for asset in assets if asset["requiresSelection"] and not asset["selectedSourceId"]),
            "hflProvenanceCounts": dict(sorted(hfl_counts.items())),
            "mapDescriptorCount": len(map_ids),
            "mapsMissingHfl": sorted(map_ids - hfl_ids),
            "mapsMissingTtb": sorted(map_ids - ttb_ids),
        },
    }
    manifest["inventorySha256"] = calculate_inventory_sha256(manifest)
    manifest["documentSha256"] = calculate_document_sha256(manifest)
    manifest["releaseValidation"] = validate_manifest_document(manifest, mode="release")
    return manifest


def calculate_inventory_sha256(manifest: dict[str, Any]) -> str:
    rows: list[str] = []
    for pack in manifest.get("packs", []):
        rows.append(f"pack\0{pack.get('path')}\0{pack.get('bytes')}\0{pack.get('sha256')}")
    for asset in manifest.get("assets", []):
        for source in asset.get("sources", []):
            rows.append(f"source\0{asset.get('stableId')}\0{source.get('sourceId')}\0{source.get('bytes')}\0{source.get('sha256')}")
    return _sha256_bytes("\n".join(sorted(rows)).encode("utf-8"))


def calculate_document_sha256(manifest: dict[str, Any]) -> str:
    """Bind provenance, selections and paths as well as byte hashes.

    This is a reproducibility checksum, not a signature or an authenticity claim.
    """
    content = {key: value for key, value in manifest.items()
               if key not in ("documentSha256", "releaseValidation")}
    return _sha256_bytes(json.dumps(content, sort_keys=True, ensure_ascii=False,
                                   separators=(",", ":")).encode("utf-8"))


def validate_manifest_document(manifest: dict[str, Any], mode: str = "release") -> dict[str, Any]:
    blockers: list[dict[str, str]] = []
    if not manifest.get("assets"):
        blockers.append({"code": "empty-inventory", "subject": "manifest", "detail": "no assets"})
    if manifest.get("documentSha256") != calculate_document_sha256(manifest):
        blockers.append({"code": "document-digest", "subject": "manifest", "detail": "metadata digest mismatch"})
    if manifest.get("schemaVersion") != SCHEMA_VERSION:
        blockers.append({"code": "schema-version", "subject": "manifest", "detail": "unsupported schemaVersion"})
    tool = manifest.get("tool", {})
    if tool.get("name") != TOOL_NAME:
        blockers.append({"code": "tool-name", "subject": "manifest", "detail": "wrong producer"})
    source_commit = str(manifest.get("source", {}).get("commit", ""))
    if not HEX_COMMIT.fullmatch(source_commit):
        blockers.append({"code": "source-commit", "subject": "manifest", "detail": "missing or invalid source commit"})
    recorded = manifest.get("inventorySha256")
    calculated = calculate_inventory_sha256(manifest)
    if recorded != calculated or not isinstance(recorded, str) or not HEX_SHA256.fullmatch(recorded):
        blockers.append({"code": "inventory-digest", "subject": "manifest", "detail": "inventory digest mismatch"})
    for issue in manifest.get("issues", []):
        blockers.append({"code": str(issue.get("code", "issue")), "subject": str(issue.get("subject", "manifest")), "detail": str(issue.get("detail", ""))})
    for unsafe in manifest.get("unsafePaths", []):
        blockers.append({"code": "unsafe-path", "subject": str(unsafe.get("source", "unknown")), "detail": str(unsafe.get("path", ""))})
    if mode == "release":
        present_paks = {str(pack.get("path", "")).casefold() for pack in manifest.get("packs", [])}
        for required in REQUIRED_PAKS:
            if required.casefold() not in present_paks:
                blockers.append({"code": "required-pak", "subject": required, "detail": "not audited"})
        for asset in manifest.get("assets", []):
            selected_id = asset.get("selectedSourceId")
            if asset.get("requiresSelection") and not selected_id:
                blockers.append({"code": "source-selection", "subject": str(asset.get("stableId")), "detail": str(asset.get("logicalPath"))})
                continue
            eligible = asset.get("sources", [])
            if selected_id:
                eligible = [source for source in eligible if source.get("sourceId") == selected_id]
            if not eligible:
                blockers.append({"code": "source-selection", "subject": str(asset.get("stableId")),
                                 "detail": "selected source does not exist"})
                continue
            if str(asset.get("logicalPath", "")).casefold().endswith(".hfl"):
                for source in eligible:
                    kind = source.get("provenance", {}).get("kind", "unknown")
                    if kind != "verified-original":
                        blockers.append({"code": "hfl-provenance", "subject": str(source.get("sourceId")), "detail": kind})
        coverage = manifest.get("coverage", {})
        for map_id in coverage.get("mapsMissingHfl", []):
            blockers.append({"code": "map-missing-hfl", "subject": str(map_id), "detail": "no logical HFL source"})
        for map_id in coverage.get("mapsMissingTtb", []):
            blockers.append({"code": "map-missing-ttb", "subject": str(map_id), "detail": "no logical TTB source"})
    unique = {(entry["code"], entry["subject"], entry["detail"]): entry for entry in blockers}
    ordered = [unique[key] for key in sorted(unique)]
    return {"mode": mode, "passed": not ordered, "blockerCount": len(ordered), "blockers": ordered}


def verify_source_bytes(manifest: dict[str, Any], root: Path,
                        additional_roots: dict[str, Path] | None = None) -> dict[str, Any]:
    """Rehash loose files and complete containers against the recorded baseline."""
    result = validate_manifest_document(manifest, "integrity")
    blockers = result["blockers"]
    root = root.resolve(strict=True)
    records: list[tuple[dict[str, Any], Path]] = [(record, root) for record in manifest.get("packs", [])]
    records.extend((source, root) for asset in manifest.get("assets", [])
                   for source in asset.get("sources", []) if source.get("sourceType") == "loose")
    supplied = {label: path.resolve(strict=True) for label, path in (additional_roots or {}).items()}
    required_labels = set(manifest.get("source", {}).get("additionalRoots", []))
    for label in sorted(required_labels - set(supplied)):
        blockers.append({"code": "source-root", "subject": label, "detail": "additional root not supplied"})
    for asset in manifest.get("assets", []):
        for source in asset.get("sources", []):
            source_type = str(source.get("sourceType", ""))
            if source_type.startswith("overlay:"):
                label = source_type.removeprefix("overlay:")
                if label in supplied:
                    records.append((source, supplied[label]))
    for record, record_root in records:
        relative = record.get("physicalPath", record.get("path", ""))
        try:
            relative = normalize_relative_path(relative)
            path = (record_root / relative).resolve(strict=True)
            path.relative_to(record_root)
            if path.stat().st_size != record["bytes"] or _sha256_file(path) != record["sha256"]:
                raise ValueError("recorded bytes/hash differ")
        except (OSError, ValueError, KeyError) as exc:
            blockers.append({"code": "source-bytes", "subject": str(relative), "detail": str(exc)})
    return {"mode": "source-bytes", "passed": not blockers, "checkedFiles": len(records),
            "blockerCount": len(blockers), "blockers": blockers}


def _write_manifest(output: Path, root: Path, manifest: dict[str, Any]) -> None:
    root_resolved = root.resolve(strict=True)
    output_resolved = output.resolve(strict=False)
    try:
        output_resolved.relative_to(root_resolved)
    except ValueError:
        pass
    else:
        raise AuditError("output must be outside the immutable resource root")
    output.parent.mkdir(parents=True, exist_ok=True)
    payload = json.dumps(manifest, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    temporary = output.with_name(output.name + ".tmp")
    if output.suffix == ".gz":
        temporary.write_bytes(gzip.compress(payload.encode("utf-8"), mtime=0))
    else:
        temporary.write_text(payload, encoding="utf-8", newline="\n")
    os.replace(temporary, output)


def _read_manifest(path: Path) -> dict[str, Any]:
    try:
        raw = gzip.decompress(path.read_bytes()).decode("utf-8") if path.suffix == ".gz" else path.read_text(encoding="utf-8")
        document = json.loads(raw)
    except (OSError, json.JSONDecodeError) as exc:
        raise AuditError(f"manifest cannot be read: {exc}") from exc
    if not isinstance(document, dict):
        raise AuditError("manifest root must be an object")
    return document


def _parse_additional_roots(values: list[str] | None) -> list[tuple[str, Path]]:
    parsed: list[tuple[str, Path]] = []
    for value in values or []:
        label, separator, path = value.partition("=")
        if not separator or not path:
            raise AuditError("additional root must use LABEL=PATH")
        parsed.append((label, Path(path)))
    return parsed


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subcommands = parser.add_subparsers(dest="command", required=True)
    audit = subcommands.add_parser("audit", help="hash loose files and all seven PAK inventories")
    audit.add_argument("root", type=Path)
    audit.add_argument("--output", type=Path, required=True)
    audit.add_argument("--profile-id", required=True)
    audit.add_argument("--source-commit")
    audit.add_argument("--selection", type=Path)
    audit.add_argument("--additional-root", action="append", default=[], metavar="LABEL=PATH")
    validate = subcommands.add_parser("validate", help="validate an existing manifest")
    validate.add_argument("manifest", type=Path)
    validate.add_argument("--mode", choices=("integrity", "release"), default="release")
    verify = subcommands.add_parser("verify-source", help="rehash original files and PAK containers against a baseline")
    verify.add_argument("manifest", type=Path)
    verify.add_argument("root", type=Path)
    verify.add_argument("--additional-root", action="append", default=[], metavar="LABEL=PATH")
    args = parser.parse_args(argv)
    try:
        if args.command == "audit":
            manifest = build_manifest(args.root, args.profile_id, args.source_commit, args.selection,
                                      _parse_additional_roots(args.additional_root))
            _write_manifest(args.output, args.root, manifest)
            coverage = manifest["coverage"]
            validation = manifest["releaseValidation"]
            print(json.dumps({
                "manifest": str(args.output),
                "inventorySha256": manifest["inventorySha256"],
                "logicalAssets": coverage["logicalAssetCount"],
                "physicalSources": coverage["physicalSourceCount"],
                "conflicts": coverage["conflictAssetCount"],
                "duplicates": coverage["duplicateAssetCount"],
                "unresolvedSelections": coverage["unresolvedSelectionCount"],
                "releasePassed": validation["passed"],
                "releaseBlockers": validation["blockerCount"],
            }, sort_keys=True))
            return 0
        manifest = _read_manifest(args.manifest)
        result = (verify_source_bytes(manifest, args.root,
                    dict(_parse_additional_roots(args.additional_root))) if args.command == "verify-source"
                  else validate_manifest_document(manifest, args.mode))
        print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))
        return 0 if result["passed"] else 1
    except AuditError as exc:
        print(f"{TOOL_NAME}: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
