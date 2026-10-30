"""Pure-Python contract helpers for the Moxiang Asset Reset V2 Blender pipeline.

This module intentionally has no bpy dependency so it can be tested in ordinary
Python and reused by CI when Blender is not running.
"""
from __future__ import annotations

import hashlib
import json
import re
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Sequence

SCHEMA = "moxiang.blender-export-receipt.v1"
ASSET_ID_RE = re.compile(r"^MX_[A-Z0-9][A-Z0-9_]{2,95}$")
ALLOWED_CATEGORIES = {
    "Characters",
    "NPCs",
    "Monsters",
    "Weapons",
    "Environment",
    "Terrain",
    "Vegetation",
    "VFX",
    "UI",
    "Audio",
}


@dataclass(frozen=True)
class ValidationIssue:
    severity: str
    code: str
    message: str


@dataclass(frozen=True)
class ExportReceipt:
    schema: str
    generated_utc: str
    asset_id: str
    category: str
    source_file: str
    source_sha256: str
    export_file: str
    export_sha256: str
    blender_version: str
    scale_meters: float
    forward_axis: str
    up_axis: str
    objects: Sequence[str]


def sha256_file(path: Path, chunk_size: int = 1024 * 1024) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while True:
            chunk = stream.read(chunk_size)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def validate_asset_id(asset_id: str) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    if not ASSET_ID_RE.fullmatch(asset_id):
        issues.append(
            ValidationIssue(
                "error",
                "asset_id.invalid",
                "Asset ID must match MX_[A-Z0-9_]+ and be 5-99 characters.",
            )
        )
    return issues


def validate_category(category: str) -> list[ValidationIssue]:
    if category in ALLOWED_CATEGORIES:
        return []
    return [
        ValidationIssue(
            "error",
            "category.invalid",
            f"Category '{category}' is not one of: {', '.join(sorted(ALLOWED_CATEGORIES))}.",
        )
    ]


def validate_object_names(names: Iterable[str]) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    seen: set[str] = set()
    for raw_name in names:
        name = raw_name.strip()
        if not name:
            issues.append(ValidationIssue("error", "object.empty_name", "Object name is empty."))
            continue
        if name in seen:
            issues.append(
                ValidationIssue("error", "object.duplicate_name", f"Duplicate object name: {name}")
            )
        seen.add(name)
        if " " in name:
            issues.append(
                ValidationIssue("warning", "object.space_in_name", f"Object contains spaces: {name}")
            )
    return issues


def build_receipt(
    *,
    asset_id: str,
    category: str,
    source_file: Path,
    export_file: Path,
    blender_version: str,
    scale_meters: float,
    forward_axis: str,
    up_axis: str,
    objects: Sequence[str],
) -> ExportReceipt:
    problems = validate_asset_id(asset_id) + validate_category(category)
    errors = [issue for issue in problems if issue.severity == "error"]
    if errors:
        raise ValueError("; ".join(issue.message for issue in errors))
    if not source_file.is_file():
        raise FileNotFoundError(source_file)
    if not export_file.is_file():
        raise FileNotFoundError(export_file)

    return ExportReceipt(
        schema=SCHEMA,
        generated_utc=datetime.now(timezone.utc).isoformat(),
        asset_id=asset_id,
        category=category,
        source_file=source_file.name,
        source_sha256=sha256_file(source_file),
        export_file=export_file.name,
        export_sha256=sha256_file(export_file),
        blender_version=blender_version,
        scale_meters=float(scale_meters),
        forward_axis=forward_axis,
        up_axis=up_axis,
        objects=tuple(sorted(objects)),
    )


def write_receipt(path: Path, receipt: ExportReceipt) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(asdict(receipt), ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
