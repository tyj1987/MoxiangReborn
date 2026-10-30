"""Build an original, editable low-poly ground-drop cloth pouch with Blender 5.x.

Run: blender --background --python build_cloth_pouch.py
This script writes only beside itself. No source images or meshes are imported.
"""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parent
ASSET_ID = "MX_ENV_GROUNDDROP_CLOTH_POUCH_001"
COLLECTION = ASSET_ID
BLEND = ROOT / f"{ASSET_ID}.blend"
FBX = ROOT / f"{ASSET_ID}.fbx"
GLB = ROOT / f"{ASSET_ID}.glb"
TEXTURES = ROOT / "Textures"
PREVIEWS = ROOT / "Previews"
TEXTURES.mkdir(exist_ok=True)
PREVIEWS.mkdir(exist_ok=True)

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for block in list(bpy.data.collections):
    if block.name != bpy.context.scene.collection.name:
        bpy.data.collections.remove(block)
production = bpy.data.collections.new(COLLECTION)
bpy.context.scene.collection.children.link(production)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = 1.0
bpy.context.preferences.filepaths.save_version = 0


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def tex(name: str, size: int, kind: str) -> bpy.types.Image:
    image = bpy.data.images.new(name, width=size, height=size, alpha=True)
    px = [0.0] * (size * size * 4)
    for y in range(size):
        for x in range(size):
            u, v = x / size, y / size
            weave = math.sin(2 * math.pi * u * 76) * math.sin(2 * math.pi * v * 72)
            grain = math.sin(2 * math.pi * (u * 17 + v * 13)) * 0.5 + math.sin(2 * math.pi * (u * 41 - v * 31)) * 0.2
            ink = 0.045 * weave + 0.022 * grain
            i = (y * size + x) * 4
            if kind == "basecolor":
                px[i:i + 4] = [0.51 + ink, 0.38 + ink * 0.85, 0.25 + ink * 0.6, 1.0]
            elif kind == "roughness":
                val = 0.83 + 0.045 * weave + 0.025 * grain
                px[i:i + 4] = [val, val, val, 1.0]
            else:
                # Tangent-space woven-fibre normal. Deliberately subtle for URP.
                dx = 0.05 * math.cos(2 * math.pi * u * 76) * math.sin(2 * math.pi * v * 72)
                dy = 0.05 * math.sin(2 * math.pi * u * 76) * math.cos(2 * math.pi * v * 72)
                px[i:i + 4] = [0.5 + dx, 0.5 + dy, 0.5 + math.sqrt(max(0.0, 1 - dx * dx - dy * dy)) * 0.5, 1.0]
    image.pixels.foreach_set(px)
    image.filepath_raw = str(TEXTURES / f"{ASSET_ID}_{kind}.png")
    image.file_format = "PNG"
    image.save()
    if kind != "basecolor":
        image.colorspace_settings.name = "Non-Color"
    return image


base = tex("Cloth_BaseColor", 256, "basecolor")
rough = tex("Cloth_Roughness", 256, "roughness")
normal = tex("Cloth_Normal", 256, "normal")
cloth = bpy.data.materials.new("MAT_OchreWovenCloth_PBR")
cloth.diffuse_color = (0.51, 0.38, 0.25, 1)
cloth.use_nodes = True
nodes = cloth.node_tree.nodes
nodes.clear()
links = cloth.node_tree.links
out = nodes.new("ShaderNodeOutputMaterial")
bsdf = nodes.new("ShaderNodeBsdfPrincipled")
bsdf.inputs["Metallic"].default_value = 0
links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
for img, socket in [(base, "Base Color"), (rough, "Roughness")]:
    node = nodes.new("ShaderNodeTexImage")
    node.image = img
    links.new(node.outputs["Color"], bsdf.inputs[socket])
ntex = nodes.new("ShaderNodeTexImage")
ntex.image = normal
nmap = nodes.new("ShaderNodeNormalMap")
nmap.inputs["Strength"].default_value = 0.55
links.new(ntex.outputs["Color"], nmap.inputs["Color"])
links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])


def solid_material(name, rgb, roughness, metal=0):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*rgb, 1)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*rgb, 1)
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["Metallic"].default_value = metal
    return mat


cord_mat = solid_material("MAT_IndigoHempCord", (0.075, 0.12, 0.16), 0.86)
mouth_mat = solid_material("MAT_InnerFold", (0.028, 0.019, 0.012), 0.94)
stitch_mat = solid_material("MAT_HandStitch", (0.39, 0.28, 0.17), 0.87)


def mesh_obj(name, vertices, faces, material, uv_fn=None):
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    production.objects.link(obj)
    mesh.materials.append(material)
    uv = mesh.uv_layers.new(name="UV0")
    for poly in mesh.polygons:
        for loop_index in poly.loop_indices:
            vert_index = mesh.loops[loop_index].vertex_index
            if uv_fn:
                uv.data[loop_index].uv = uv_fn(vert_index, poly.index)
            else:
                x, y, z = vertices[vert_index]
                uv.data[loop_index].uv = ((x + 0.2) * 2.3, z * 3.2 + y * 0.2)
    return obj


N = 20
profile = [
    (0.008, 0.060, 0.89),
    (0.019, 0.101, 0.98),
    (0.050, 0.131, 1.03),
    (0.095, 0.149, 1.00),
    (0.143, 0.146, 1.02),
    (0.185, 0.133, 1.00),
    (0.218, 0.113, 1.03),
    (0.238, 0.087, 0.97),
    (0.258, 0.067, 0.96),
]
vertices = []
for ring, (z, radius, wobble) in enumerate(profile):
    for j in range(N):
        angle = 2 * math.pi * j / N
        fold = 1 + 0.061 * math.cos(5 * angle + 0.38 * ring) + 0.029 * math.sin(8 * angle - 0.29 * ring)
        r = radius * fold
        vertices.append((r * math.cos(angle), r * math.sin(angle) * wobble, z + 0.0028 * math.sin(5 * angle + ring * 0.48)))
faces = []
for ring in range(len(profile) - 1):
    for j in range(N):
        k = (j + 1) % N
        faces.append((ring * N + j, ring * N + k, (ring + 1) * N + k, (ring + 1) * N + j))
faces.append(tuple(reversed(tuple(range(N)))))


def bag_uv(index, poly_index):
    ring, j = divmod(index, N)
    u = j / N
    if poly_index < (len(profile) - 1) * N and poly_index % N == N - 1 and j == 0:
        u = 1.0
    return (u * 2.0, profile[ring][0] * 3.8)


mesh_obj("Pouch_Body", vertices, faces, cloth, bag_uv)

# Gathered neck and a jagged folded rim make the bag read as cloth at a distance.
neck_verts = []
for ring, (z, radius) in enumerate([(0.247, 0.071), (0.267, 0.065), (0.286, 0.079)]):
    for j in range(N):
        a = 2 * math.pi * j / N
        pleat = 1 + 0.065 * math.cos(5 * a + ring * 0.3)
        zz = z + (0.008 * math.sin(5 * a) if ring == 2 else 0)
        neck_verts.append((radius * pleat * math.cos(a), radius * pleat * math.sin(a), zz))
neck_faces = [(r * N + j, r * N + (j + 1) % N, (r + 1) * N + (j + 1) % N, (r + 1) * N + j)
              for r in range(2) for j in range(N)]
mesh_obj("Pouch_GatheredNeck", neck_verts, neck_faces, cloth)

# Dark inner mouth, recessed below the irregular rim.
inner_verts = [(0, 0, 0.264)] + [(0.054 * math.cos(2 * math.pi * j / N), 0.054 * math.sin(2 * math.pi * j / N), 0.278) for j in range(N)]
inner_faces = [(0, 1 + j, 1 + (j + 1) % N) for j in range(N)]
mesh_obj("Pouch_InnerMouth", inner_verts, inner_faces, mouth_mat)


def tube(name, points, radius, material, sides=6):
    verts = []
    faces = []
    for i, p0 in enumerate(points):
        p = Vector(p0)
        tangent = Vector(points[min(i + 1, len(points) - 1)]) - Vector(points[max(i - 1, 0)])
        tangent.normalize()
        side = tangent.cross(Vector((0, 0, 1)))
        if side.length < 0.001:
            side = tangent.cross(Vector((1, 0, 0)))
        side.normalize()
        other = tangent.cross(side).normalized()
        for j in range(sides):
            a = 2 * math.pi * j / sides
            v = p + radius * (math.cos(a) * side + math.sin(a) * other)
            verts.append(tuple(v))
    for i in range(len(points) - 1):
        for j in range(sides):
            k = (j + 1) % sides
            faces.append((i * sides + j, i * sides + k, (i + 1) * sides + k, (i + 1) * sides + j))
    faces.extend([tuple(reversed(tuple(range(sides)))), tuple((len(points) - 1) * sides + j for j in range(sides))])
    return mesh_obj(name, verts, faces, material)


drawcord = []
for j in range(25):
    a = 2 * math.pi * j / 24
    drawcord.append((0.073 * math.cos(a), 0.073 * math.sin(a), 0.253 + 0.002 * math.sin(5 * a)))
tube("Cord_CinchLoop", drawcord, 0.0043, cord_mat)
for sign, suffix in [(1, "L"), (-1, "R")]:
    points = [(sign * 0.011, -0.073, 0.252), (sign * 0.035, -0.081, 0.235),
              (sign * 0.055, -0.077, 0.207), (sign * 0.056, -0.084, 0.174)]
    tube("Cord_Tail_" + suffix, points, 0.0046, cord_mat)
    tube("Cord_Tip_" + suffix, [points[-1], (sign * 0.057, -0.085, 0.161)], 0.0062, stitch_mat)
tube("Cord_TiedKnot", [(-0.016, -0.077, 0.252), (0, -0.087, 0.256), (0.016, -0.077, 0.252)], 0.009, cord_mat, 8)

# Two hand-sewn vertical fold lines, kept subdued so the silhouette remains neutral.
for index, angle in enumerate([-2.12, -1.05]):
    line = []
    for z, r in [(0.033, 0.113), (0.073, 0.140), (0.118, 0.150), (0.161, 0.142), (0.199, 0.122)]:
        line.append(((r + 0.003) * math.cos(angle), (r + 0.003) * math.sin(angle), z))
    tube("Cloth_StitchFold_%02d" % index, line, 0.0016, stitch_mat, 4)

for obj in production.objects:
    if obj.type == "MESH":
        for poly in obj.data.polygons:
            poly.use_smooth = False

# Production objects have identity transforms and a ground-contact origin.
bpy.ops.object.select_all(action="DESELECT")
for obj in production.objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = production.objects[0]
bpy.ops.file.pack_all()
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))

bpy.ops.export_scene.fbx(
    filepath=str(FBX), use_selection=True, object_types={"MESH"},
    apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
    use_space_transform=True, bake_space_transform=False,
    axis_forward="-Z", axis_up="Y", path_mode="RELATIVE", embed_textures=False,
    bake_anim=False, add_leaf_bones=False,
)
bpy.ops.export_scene.gltf(filepath=str(GLB), export_format="GLB", use_selection=True,
                          export_apply=True, export_yup=True)

# Clear isolated render setup; it is excluded from FBX/GLB and kept in the .blend.
bpy.ops.object.select_all(action="DESELECT")
camera_data = bpy.data.cameras.new("PreviewCamera")
camera = bpy.data.objects.new("PreviewCamera", camera_data)
scene.collection.objects.link(camera)
scene.camera = camera
camera.rotation_euler = (Vector((0, 0, 0.15)) - Vector((0.55, -0.75, 0.42))).to_track_quat("-Z", "Y").to_euler()
camera.location = (0.55, -0.75, 0.42)
camera_data.type = "ORTHO"
for name, location, power, size in [("Key", (0.3, -0.5, 0.9), 13, 0.65), ("Fill", (-0.5, 0.2, 0.55), 8, 0.85)]:
    light_data = bpy.data.lights.new(name, "AREA")
    light_data.energy = power
    light_data.shape = "DISK"
    light_data.size = size
    light = bpy.data.objects.new(name, light_data)
    light.location = location
    light.rotation_euler = (Vector((0, 0, 0.14)) - light.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(light)
scene.render.engine = "BLENDER_EEVEE"
scene.render.film_transparent = True
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.render.resolution_percentage = 100
scene.render.resolution_x = 512
scene.render.resolution_y = 512
scene.world.color = (0.25, 0.25, 0.25)
scene.view_settings.view_transform = "Standard"
for label, scale in [("near", 0.52), ("far", 1.40)]:
    camera_data.ortho_scale = scale
    scene.render.filepath = str(PREVIEWS / f"{ASSET_ID}_{label}.png")
    bpy.ops.render.render(write_still=True)

verts = [Vector(v.co) for obj in production.objects for v in obj.data.vertices]
triangles = sum(len(poly.vertices) - 2 for obj in production.objects for poly in obj.data.polygons)
files = [BLEND, FBX, GLB, *sorted(TEXTURES.glob("*.png")), *sorted(PREVIEWS.glob("*.png")), Path(__file__)]
manifest = {
    "asset_id": ASSET_ID,
    "provenance": "Original procedural meshes and textures authored by this local Blender script; no legacy or external asset bytes used.",
    "license": "Project-owned original asset; use subject to Moxiang project ownership policy.",
    "source": BLEND.name,
    "blender_version": bpy.app.version_string,
    "unit": "meter",
    "pivot": "ground contact at (0,0,0)",
    "blender_axes": "+Z up, -Y front",
    "unity_fbx_export_axes": "-Z forward, +Y up",
    "bounds_blender_m": {"min": [round(min(v[i] for v in verts), 5) for i in range(3)],
                         "max": [round(max(v[i] for v in verts), 5) for i in range(3)]},
    "mesh_objects": len(production.objects),
    "vertices": len(verts),
    "triangles": triangles,
    "texture_channels": {"basecolor": "sRGB RGBA", "roughness": "linear grayscale R", "normal": "linear tangent-space RGB"},
    "files": {str(p.relative_to(ROOT)).replace("\\", "/"): {"bytes": p.stat().st_size, "sha256": digest(p)} for p in files},
}
(ROOT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print("ASSET_BUILD_OK " + json.dumps({k: manifest[k] for k in ["asset_id", "blender_version", "bounds_blender_m", "vertices", "triangles"]}))

