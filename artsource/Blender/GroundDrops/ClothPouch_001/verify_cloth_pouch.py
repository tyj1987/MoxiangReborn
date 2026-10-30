"""Offline Blender round-trip and digest validation for the cloth pouch asset."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

import bpy
from mathutils import Vector


root = Path(__file__).resolve().parent
manifest = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
asset_id = manifest["asset_id"]


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def bounds(objects):
    coords = [o.matrix_world @ Vector(v.co) for o in objects for v in o.data.vertices]
    low = [min(v[i] for v in coords) for i in range(3)]
    high = [max(v[i] for v in coords) for i in range(3)]
    return {"min": [round(v, 5) for v in low], "max": [round(v, 5) for v in high],
            "span": [round(b - a, 5) for a, b in zip(low, high)]}


def check_geometry(label, meshes):
    assert meshes, f"{label}: no meshes"
    box = bounds(meshes)
    assert 0.26 < box["span"][0] < 0.34, (label, box)
    assert 0.26 < box["span"][1] < 0.34, (label, box)
    assert 0.27 < box["span"][2] < 0.33, (label, box)
    assert -0.01 < box["min"][2] < 0.02, (label, box)
    return {"mesh_objects": len(meshes), "bounds_m": box,
            "triangles": sum(len(p.vertices) - 2 for o in meshes for p in o.data.polygons)}


for filename, metadata in manifest["files"].items():
    file = root / filename
    assert file.is_file(), filename
    assert file.stat().st_size == metadata["bytes"], filename
    assert sha256(file) == metadata["sha256"], filename
assert (root / f"{asset_id}.fbx").read_bytes()[:18] == b"Kaydara FBX Binary"
assert (root / f"{asset_id}.glb").read_bytes()[:4] == b"glTF"

source = root / f"{asset_id}.blend"
bpy.ops.wm.open_mainfile(filepath=str(source))
collection = bpy.data.collections.get(asset_id)
assert collection is not None
meshes = [obj for obj in collection.objects if obj.type == "MESH"]
assert all(obj.data.uv_layers for obj in meshes)
assert all(tuple(obj.scale) == (1, 1, 1) for obj in meshes)
source_result = check_geometry("blend", meshes)
assert source_result["triangles"] < 1500
for suffix in ("basecolor", "roughness", "normal"):
    texture = bpy.data.images.get({"basecolor": "Cloth_BaseColor", "roughness": "Cloth_Roughness", "normal": "Cloth_Normal"}[suffix])
    assert texture is not None and texture.packed_file is not None, suffix
    assert tuple(texture.size) == (256, 256), suffix

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(root / f"{asset_id}.fbx"))
fbx_meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
fbx_result = check_geometry("fbx", fbx_meshes)

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(root / f"{asset_id}.glb"))
glb_meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
glb_result = check_geometry("glb", glb_meshes)

assert len(fbx_meshes) == len(glb_meshes) == len(meshes)
for label in ("near", "far"):
    preview = bpy.data.images.load(str(root / "Previews" / f"{asset_id}_{label}.png"), check_existing=False)
    assert tuple(preview.size) == (512, 512)
    assert preview.pixels[3] == 0, label
    assert preview.pixels[(256 * 512 + 256) * 4 + 3] > 0.5, label

result = {"status": "passed", "blender_version": bpy.app.version_string,
          "file_hashes_match_manifest": True, "preview_alpha": True,
          "source": source_result, "fbx_reimport": fbx_result, "glb_reimport": glb_result}
(root / "validation.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print("ASSET_VERIFY_OK " + json.dumps(result))

