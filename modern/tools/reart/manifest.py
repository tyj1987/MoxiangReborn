"""Strict, fail-closed validation for the original presentation asset profile.

The ledger describes artifacts, not claims of whole-game completion. This
module intentionally has no fallback into PlayDH and no network dependency.
"""
from __future__ import annotations
import hashlib
import json
import re
import unicodedata
from pathlib import Path, PurePosixPath
from typing import Any

PROJECT = "tyj1987/MoxiangReborn"
PROFILE = "moxiang-reart-v1"
BASELINE = "89a11b3653549bab7471f53f8f0aafb72e98ce2a"
STAGES = ("planned", "reference_audited", "concept_approved", "authored", "import_pass", "gameplay_bound", "visual_audio_pass", "device_pass", "release_approved")
RELEASE_EVIDENCE = {"format", "gameplay", "visual_audio", "device", "provenance"}
SHA256 = re.compile(r"[0-9a-f]{64}\Z")
ASSET_ID = re.compile(r"[a-z][a-z0-9/_-]{2,127}\Z")

class AssetError(ValueError):
    """The asset pack is invalid or does not meet the requested gate."""

def relative_name(value: object) -> str:
    if not isinstance(value, str) or not value or "\\" in value or ":" in value:
        raise AssetError(f"Invalid relative asset path: {value!r}")
    if any(ord(c) < 32 for c in value):
        raise AssetError("Control character in asset path")
    path = PurePosixPath(value)
    if path.is_absolute() or any(p in ("", ".", "..") for p in value.split("/")):
        raise AssetError(f"Non-canonical asset path: {value!r}")
    reserved = {"con", "prn", "aux", "nul", *("com"+str(i) for i in range(1,10)), *("lpt"+str(i) for i in range(1,10))}
    if any(part.endswith((" ", ".")) or part.split(".")[0].casefold() in reserved for part in path.parts):
        raise AssetError("Windows-ambiguous asset path")
    if unicodedata.normalize("NFC", value) != value:
        raise AssetError(f"Non-normalized asset path: {value!r}")
    return value

def checked_path(root: Path, value: object, *, must_exist: bool = True) -> Path:
    name = relative_name(value)
    if root.is_symlink():
        raise AssetError("Asset root must not be a symlink")
    root = root.resolve()
    current = root
    for part in PurePosixPath(name).parts:
        current /= part
        if current.is_symlink():
            raise AssetError(f"Symlink in asset path: {name}")
    if not current.resolve().is_relative_to(root):
        raise AssetError(f"Asset escapes pack root: {name}")
    if must_exist and not current.is_file():
        raise AssetError(f"Missing asset: {name}")
    return current

def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def read_json(path: Path) -> Any:
    def pairs(items: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in items:
            if key in result:
                raise AssetError(f"Duplicate JSON key: {key}")
            result[key] = value
        return result
    def invalid_number(value: str) -> None:
        raise AssetError(f"Invalid JSON numeric constant: {value}")
    try:
        return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=pairs, parse_constant=invalid_number)
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise AssetError(f"Cannot read JSON {path}: {exc}") from exc

def validate(root: Path, *, release: bool = False) -> dict[str, Any]:
    manifest = read_json(checked_path(root, "manifest.json"))
    if not isinstance(manifest, dict) or type(manifest.get("schema")) is not int or manifest.get("schema") != 1:
        raise AssetError("Unsupported asset manifest schema")
    if manifest.get("project") != PROJECT or manifest.get("profile") != PROFILE:
        raise AssetError("Wrong project or asset profile")
    if manifest.get("legacy_fallback") is not False:
        raise AssetError("Legacy fallback is prohibited")
    if manifest.get("baseline_sha") != BASELINE:
        raise AssetError("Baseline differs; re-audit before changing the baseline")
    assets = manifest.get("assets")
    if not isinstance(assets, list) or not assets:
        raise AssetError("Empty/missing asset ledger")
    ids: dict[str, dict[str, Any]] = {}
    names: set[str] = set()
    total_bytes = 0
    artifacts = 0
    release_blockers: list[str] = []
    for asset in assets:
        if not isinstance(asset, dict):
            raise AssetError("Asset entry must be an object")
        asset_id = asset.get("asset_id")
        if not isinstance(asset_id, str) or not ASSET_ID.fullmatch(asset_id) or asset_id in ids:
            raise AssetError(f"Invalid/duplicate asset ID: {asset_id!r}")
        ids[asset_id] = asset
        stage = asset.get("stage")
        if stage not in STAGES:
            raise AssetError(f"Unknown stage for {asset_id}")
        if not isinstance(asset.get("gameplay_binding"), dict):
            raise AssetError(f"Missing explicit binding object: {asset_id}")
        if not isinstance(asset.get("provenance"), dict):
            raise AssetError(f"Missing provenance: {asset_id}")
        if not isinstance(asset.get("dependencies"), list) or not all(isinstance(x, str) for x in asset["dependencies"]):
            raise AssetError(f"Invalid dependency list: {asset_id}")
        if len(set(asset["dependencies"])) != len(asset["dependencies"]):
            raise AssetError(f"Duplicate dependency: {asset_id}")
        files = asset.get("files")
        if not isinstance(files, list) or not files:
            raise AssetError(f"No artifact files: {asset_id}")
        for item in files:
            if not isinstance(item, dict):
                raise AssetError(f"Invalid artifact: {asset_id}")
            path = checked_path(root, item.get("path"))
            key = unicodedata.normalize("NFKC", item["path"]).casefold()
            if key in names or key == "manifest.json":
                raise AssetError(f"Path collision: {item['path']}")
            names.add(key)
            if not isinstance(item.get("sha256"), str) or not SHA256.fullmatch(item["sha256"]):
                raise AssetError(f"Invalid checksum: {item['path']}")
            if type(item.get("bytes")) is not int or item["bytes"] < 1:
                raise AssetError(f"Invalid byte count: {item['path']}")
            if path.stat().st_size != item["bytes"] or digest(path) != item["sha256"]:
                raise AssetError(f"Checksum/size mismatch: {item['path']}")
            artifacts += 1
            total_bytes += item["bytes"]
        evidence = asset.get("acceptance_evidence", [])
        if not isinstance(evidence, list):
            raise AssetError(f"Invalid evidence list: {asset_id}")
        evidence_kinds = set()
        for entry in evidence:
            if not isinstance(entry, dict) or entry.get("result") not in ("pass", "fail", "not_run"):
                raise AssetError(f"Invalid evidence record: {asset_id}")
            if entry.get("result") == "pass":
                evidence_path = checked_path(root, entry.get("path"))
                if entry.get("sha256") != digest(evidence_path):
                    raise AssetError(f"Evidence checksum mismatch: {asset_id}")
                if not isinstance(entry.get("kind"), str) or not entry["kind"]:
                    raise AssetError("Invalid evidence kind")
                evidence_kinds.add(entry["kind"])
        if stage != "release_approved":
            release_blockers.append(f"{asset_id}: stage={stage}")
        if asset["provenance"].get("review") != "approved":
            release_blockers.append(f"{asset_id}: provenance not approved")
        if asset.get("technical_sample") is not False:
            release_blockers.append(f"{asset_id}: sample/unverified production status")
        if not RELEASE_EVIDENCE.issubset(evidence_kinds):
            release_blockers.append(f"{asset_id}: missing release evidence")
        binding = asset["gameplay_binding"]
        if not binding or any(v is None for v in binding.values()):
            release_blockers.append(f"{asset_id}: unresolved gameplay binding")
    active: set[str] = set()
    done: set[str] = set()
    def visit(asset_id: str) -> None:
        if asset_id not in ids:
            raise AssetError(f"Missing dependency: {asset_id}")
        if asset_id in active:
            raise AssetError(f"Dependency cycle at {asset_id}")
        if asset_id in done:
            return
        active.add(asset_id)
        for dependency in ids[asset_id]["dependencies"]:
            visit(dependency)
        active.remove(asset_id)
        done.add(asset_id)
    for asset_id in ids:
        visit(asset_id)
    required = manifest.get("required_assets")
    if not isinstance(required, list) or not required or not all(isinstance(x, str) for x in required):
        raise AssetError("Explicit required-asset set is missing")
    if len(set(required)) != len(required) or not set(required).issubset(ids):
        raise AssetError("Missing or duplicate required assets")
    coverage = manifest.get("coverage", {})
    if not isinstance(coverage, dict):
        raise AssetError("Invalid coverage object")
    if coverage.get("active_gameplay_inventory_complete") is not True:
        release_blockers.append("Active gameplay inventory is incomplete")
    if release and release_blockers:
        raise AssetError("Release blocked:\n" + "\n".join(release_blockers))
    return {"schema": 1, "profile": PROFILE, "assets": len(ids), "artifact_files": artifacts, "artifact_bytes": total_bytes, "integrity": "pass", "release_eligible": not release_blockers, "release_blockers": release_blockers, "scope": "Pack integrity only; does not prove rendering, gameplay or device acceptance."}
