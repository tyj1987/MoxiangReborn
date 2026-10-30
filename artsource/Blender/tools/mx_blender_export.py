"""Moxiang Asset Reset V2 Blender validator and Unity FBX exporter.

Usage inside Blender:
  blender my_asset.blend --background --python mx_blender_export.py -- \
      --asset-id MX_ENV_WUGUAN_GATE_001 \
      --category Environment \
      --output <path-to-fbx>

The script validates the scene, exports selected production objects to FBX and
writes a digest-bound JSON receipt next to the FBX.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import bpy

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from mx_asset_contract import (
    ValidationIssue,
    build_receipt,
    validate_asset_id,
    validate_category,
    validate_object_names,
    write_receipt,
)

COLLISION_PREFIXES = ("UCX_", "UBX_", "USP_", "UCP_")
IGNORED_TYPES = {"CAMERA", "LIGHT"}
EXPORTABLE_TYPES = {"MESH", "ARMATURE", "EMPTY"}


def _issue(severity: str, code: str, message: str) -> ValidationIssue:
    return ValidationIssue(severity, code, message)


def validate_scene(asset_id: str, category: str) -> list[ValidationIssue]:
    issues = validate_asset_id(asset_id) + validate_category(category)
    scene = bpy.context.scene
    if scene.unit_settings.system != "METRIC":
        issues.append(_issue("error", "scene.units", "Scene unit system must be METRIC."))
    if abs(scene.unit_settings.scale_length - 1.0) > 1e-6:
        issues.append(
            _issue(
                "error",
                "scene.scale_length",
                f"Scene scale_length must be 1.0, got {scene.unit_settings.scale_length}.",
            )
        )

    objects = [obj for obj in scene.objects if obj.type not in IGNORED_TYPES]
    issues.extend(validate_object_names(obj.name for obj in objects))

    meshes = [obj for obj in objects if obj.type == "MESH"]
    if category not in {"UI", "Audio", "VFX"} and not meshes:
        issues.append(_issue("error", "scene.no_mesh", "No mesh objects found."))

    for obj in objects:
        if obj.type not in EXPORTABLE_TYPES:
            continue
        if any(abs(value - 1.0) > 1e-4 for value in obj.scale):
            issues.append(
                _issue(
                    "error",
                    "object.unapplied_scale",
                    f"{obj.name}: apply scale before export; scale={tuple(round(v, 6) for v in obj.scale)}.",
                )
            )
        if obj.type == "MESH":
            mesh = obj.data
            if len(mesh.uv_layers) == 0:
                issues.append(_issue("error", "mesh.uv0_missing", f"{obj.name}: UV0 is required."))
            if len(mesh.materials) > 8:
                issues.append(
                    _issue(
                        "warning",
                        "mesh.material_slots_high",
                        f"{obj.name}: {len(mesh.materials)} material slots; review category budget.",
                    )
                )

    return issues


def _production_objects(collection_name: str | None = None) -> list[bpy.types.Object]:
    if collection_name:
        collection = bpy.data.collections.get(collection_name)
        if collection is None:
            raise ValueError(f"Collection not found: {collection_name}")
        candidates = list(collection.all_objects)
    else:
        candidates = list(bpy.context.scene.objects)

    result = []
    seen = set()
    for obj in candidates:
        if obj.name in seen or obj.hide_render or obj.type in IGNORED_TYPES:
            continue
        if obj.type in EXPORTABLE_TYPES:
            seen.add(obj.name)
            result.append(obj)
    return result


def _select_only(objects: list[bpy.types.Object]) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = objects[0]


def export_unity_fbx(output: Path, collection_name: str | None = None) -> list[str]:
    output.parent.mkdir(parents=True, exist_ok=True)
    objects = _production_objects(collection_name)
    if not objects:
        raise ValueError("No exportable objects found for FBX export.")
    _select_only(objects)

    bpy.ops.export_scene.fbx(
        filepath=str(output),
        use_selection=True,
        object_types={"MESH", "ARMATURE", "EMPTY"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        use_space_transform=True,
        bake_space_transform=False,
        axis_forward="-Z",
        axis_up="Y",
        add_leaf_bones=False,
        use_armature_deform_only=True,
        mesh_smooth_type="FACE",
        use_mesh_modifiers=True,
        use_mesh_edges=False,
        use_tspace=True,
        path_mode="AUTO",
        embed_textures=False,
        bake_anim=True,
    )
    return [obj.name for obj in objects]


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate and export Moxiang Art V2 Blender source.")
    parser.add_argument("--asset-id", required=True)
    parser.add_argument("--category", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--validate-only", action="store_true")
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--collection", help="Export only objects inside this Blender collection.")
    return parser.parse_args(argv)


def blender_script_args() -> list[str]:
    if "--" not in sys.argv:
        return []
    return sys.argv[sys.argv.index("--") + 1 :]


def main() -> int:
    args = parse_args(blender_script_args())
    issues = validate_scene(args.asset_id, args.category)
    for issue in issues:
        print(f"MXH_ART_V2_{issue.severity.upper()} {issue.code}: {issue.message}")

    errors = [issue for issue in issues if issue.severity == "error"]
    if errors:
        print(f"MXH_ART_V2_VALIDATE_FAILED errors={len(errors)} warnings={len(issues)-len(errors)}")
        return 2

    print(f"MXH_ART_V2_VALIDATE_OK warnings={len(issues)}")
    if args.validate_only:
        return 0

    source = Path(bpy.data.filepath)
    if not source.is_file():
        print("MXH_ART_V2_EXPORT_FAILED source .blend must be saved before export")
        return 3

    output = args.output.resolve()
    object_names = export_unity_fbx(output, args.collection)
    receipt_path = (args.receipt or output.with_suffix(output.suffix + ".receipt.json")).resolve()
    receipt = build_receipt(
        asset_id=args.asset_id,
        category=args.category,
        source_file=source,
        export_file=output,
        blender_version=bpy.app.version_string,
        scale_meters=bpy.context.scene.unit_settings.scale_length,
        forward_axis="-Z",
        up_axis="Y",
        objects=object_names,
    )
    write_receipt(receipt_path, receipt)
    print(f"MXH_ART_V2_EXPORT_OK asset={args.asset_id} fbx={output} receipt={receipt_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
