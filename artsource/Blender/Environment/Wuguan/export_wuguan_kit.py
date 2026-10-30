"""Batch export the Wuguan Art V2 modular kit with digest-bound receipts.

Run:
  blender MX_ENV_WUGUAN_KIT_001.blend --background --python export_wuguan_kit.py

Each authored collection is exported independently so Unity receives modular
runtime assets while all exports retain provenance back to one reviewed .blend.
"""
from __future__ import annotations

import sys
from pathlib import Path

import bpy

REPO = Path(r"E:\Workspaces\moxiang\repo")
TOOLS = REPO / "artsource" / "Blender" / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from mx_asset_contract import build_receipt, write_receipt
from mx_blender_export import export_unity_fbx, validate_scene

CATEGORY = "Environment"
OUT_ROOT = (
    REPO
    / "unity"
    / "MoxiangClient"
    / "Assets"
    / "Moxiang"
    / "ArtV2"
    / "Environment"
    / "Wuguan"
)

MODULE_IDS = (
    "MX_ENV_WUGUAN_FLOOR_4M_001",
    "MX_ENV_WUGUAN_POST_6M_001",
    "MX_ENV_WUGUAN_BEAM_4M_001",
    "MX_ENV_WUGUAN_BEAM_8M_001",
    "MX_ENV_WUGUAN_WALL_SOLID_4M_001",
    "MX_ENV_WUGUAN_WALL_DOOR_4M_001",
    "MX_ENV_WUGUAN_LATTICE_2M_001",
    "MX_ENV_WUGUAN_DOUGONG_S_001",
    "MX_ENV_WUGUAN_DAIS_4M_001",
    "MX_ENV_WUGUAN_DUMMY_001",
    "MX_ENV_WUGUAN_LANTERN_001",
    "MX_ENV_WUGUAN_WEAPON_RACK_001",
    "MX_ENV_WUGUAN_BRAZIER_001",
    "MX_ENV_WUGUAN_BENCH_001",
    "MX_ENV_WUGUAN_EAVE_4M_001",
)


def main() -> int:
    source = Path(bpy.data.filepath)
    if not source.is_file():
        print("MXH_WUGUAN_BATCH_EXPORT_FAILED source .blend is not saved")
        return 3

    missing = [asset_id for asset_id in MODULE_IDS if bpy.data.collections.get(asset_id) is None]
    if missing:
        print("MXH_WUGUAN_BATCH_EXPORT_FAILED missing_collections=" + ",".join(missing))
        return 4

    issues = validate_scene("MX_ENV_WUGUAN_KIT_001", CATEGORY)
    for issue in issues:
        print(f"MXH_WUGUAN_{issue.severity.upper()} {issue.code}: {issue.message}")
    errors = [issue for issue in issues if issue.severity == "error"]
    if errors:
        print(f"MXH_WUGUAN_BATCH_EXPORT_FAILED validation_errors={len(errors)}")
        return 2

    OUT_ROOT.mkdir(parents=True, exist_ok=True)
    for asset_id in MODULE_IDS:
        output = OUT_ROOT / f"{asset_id}.fbx"
        objects = export_unity_fbx(output, asset_id)
        receipt = build_receipt(
            asset_id=asset_id,
            category=CATEGORY,
            source_file=source,
            export_file=output,
            blender_version=bpy.app.version_string,
            scale_meters=bpy.context.scene.unit_settings.scale_length,
            forward_axis="-Z",
            up_axis="Y",
            objects=objects,
        )
        receipt_path = output.with_suffix(output.suffix + ".receipt.json")
        write_receipt(receipt_path, receipt)
        print(
            "MXH_WUGUAN_MODULE_EXPORT_OK "
            f"asset={asset_id} objects={len(objects)} receipt={receipt_path.name}"
        )

    print(
        "MXH_WUGUAN_BATCH_EXPORT_OK "
        f"modules={len(MODULE_IDS)} blender={bpy.app.version_string}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
