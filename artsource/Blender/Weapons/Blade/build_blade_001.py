import bpy
import math
from pathlib import Path

REPO = Path(r"E:\Workspaces\moxiang\repo")
OUT = REPO / "artsource" / "Blender" / "Weapons" / "Blade" / "MX_WPN_BLADE_001.blend"
ASSET_ID = "MX_WPN_BLADE_001"

scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = 1.0

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for c in list(bpy.data.collections):
    bpy.data.collections.remove(c)

collection = bpy.data.collections.new(ASSET_ID)
scene.collection.children.link(collection)
root = bpy.data.objects.new(ASSET_ID, None)
root.empty_display_type = "PLAIN_AXES"
root.empty_display_size = 0.08
collection.objects.link(root)

def mat(name, color, metallic, roughness):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = next((n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
    m.diffuse_color = (*color, 1)
    if bsdf:
        bsdf.inputs["Base Color"].default_value = (*color, 1)
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
    return m

STEEL = mat("MX_MAT_BLADE_STEEL_PREVIEW", (0.25, 0.29, 0.32), 0.96, 0.16)
BRONZE = mat("MX_MAT_BLADE_BRONZE_PREVIEW", (0.30, 0.13, 0.035), 0.84, 0.28)
LEATHER = mat("MX_MAT_BLADE_LEATHER_PREVIEW", (0.065, 0.021, 0.013), 0.0, 0.66)
BLACK = mat("MX_MAT_BLADE_BLACKENED_PREVIEW", (0.022, 0.024, 0.025), 0.74, 0.30)

def move(obj):
    for owner in list(obj.users_collection):
        owner.objects.unlink(obj)
    collection.objects.link(obj)
    obj.parent = root

def finish(obj, material=None, bevel=0.0015):
    move(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if bevel and obj.type == "MESH":
        mod = obj.modifiers.new("MX_Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 3
        mod.limit_method = "ANGLE"
        bpy.ops.object.modifier_apply(modifier=mod.name)
    if obj.type == "MESH":
        tri = obj.modifiers.new("MX_Triangulate", "TRIANGULATE")
        bpy.ops.object.modifier_apply(modifier=tri.name)
        if material:
            obj.data.materials.append(material)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.025)
        bpy.ops.object.mode_set(mode="OBJECT")
    obj.select_set(False)
    return obj

def cube(name, dims, loc, material, rot=(0, 0, 0), bevel=0.0015):
    bpy.ops.mesh.primitive_cube_add(location=loc, rotation=rot)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dims
    return finish(obj, material, bevel)

def cyl(name, radius, depth, loc, material, verts=32, bevel=0.001):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=loc)
    obj = bpy.context.object
    obj.name = name
    return finish(obj, material, bevel)

def torus(name, major, minor, z, material):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major, minor_radius=minor,
        major_segments=40, minor_segments=10, location=(0,0,z))
    obj = bpy.context.object
    obj.name = name
    return finish(obj, material, 0)

def build_curved_blade():
    sections = [
        (0.025, 0.000, 0.048, 0.0065),
        (0.30,  0.002, 0.050, 0.0060),
        (0.60,  0.010, 0.054, 0.0054),
        (0.82,  0.028, 0.058, 0.0048),
        (0.94,  0.055, 0.046, 0.0040),
    ]
    verts = []
    for z, cx, width, thick in sections:
        verts.extend([
            (cx - width * 0.44, 0, z),
            (cx, thick, z),
            (cx + width * 0.56, 0, z),
            (cx, -thick, z),
        ])
    tip_index = len(verts)
    verts.append((0.102, 0, 1.015))
    faces = [(0,3,2,1)]
    for s in range(len(sections)-1):
        a = s * 4
        b = (s + 1) * 4
        for i in range(4):
            faces.append((a+i, a+(i+1)%4, b+(i+1)%4, b+i))
    last = (len(sections)-1) * 4
    for i in range(4):
        faces.append((last+i, last+(i+1)%4, tip_index))
    mesh = bpy.data.meshes.new(ASSET_ID + "__BLADE_MESH")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(ASSET_ID + "__BLADE", mesh)
    collection.objects.link(obj)
    obj.parent = root
    return finish(obj, STEEL, 0.0009)

build_curved_blade()
cube(ASSET_ID+"__SPINE",(0.018,0.0045,0.62),(0.016,-0.006,0.43),BLACK,rot=(0,math.radians(1.8),0),bevel=0.0006)
cube(ASSET_ID+"__GUARD",(0.20,0.042,0.026),(0,0,0.002),BRONZE,bevel=0.006)
for side in (-1,1):
    cyl(
        ASSET_ID+("__GUARD_FIN_L" if side < 0 else "__GUARD_FIN_R"),
        0.012, 0.075, (side*0.112,0,-0.010), BRONZE,
        verts=24, bevel=0.0013
    )

cyl(ASSET_ID+"__GRIP_CORE",0.026,0.285,(0,0,-0.165),LEATHER,verts=36,bevel=0.0012)
for i,z in enumerate([-.055,-.085,-.115,-.145,-.175,-.205,-.235,-.265]):
    torus(f"{ASSET_ID}__WRAP_{i:02d}",0.0265,0.0025,z,BLACK if i not in (0,7) else BRONZE)
cyl(ASSET_ID+"__POMMEL_NECK",0.019,0.036,(0,0,-0.326),BRONZE,verts=24,bevel=0.001)
bpy.ops.mesh.primitive_uv_sphere_add(
    segments=32, ring_count=16, location=(0,0,-0.360), scale=(0.032,0.024,0.038))
pom = bpy.context.object
pom.name = ASSET_ID + "__POMMEL"
finish(pom, BRONZE, 0)

for name,loc in [
    ("MX_SOCKET_GRIP",(0,0,-0.165)),
    ("MX_SOCKET_TRAIL_BASE",(0,0,0.055)),
    ("MX_SOCKET_TRAIL_TIP",(0.098,0,1.005)),
    ("MX_SOCKET_FX_PRIMARY",(0.035,0,0.72)),
]:
    e = bpy.data.objects.new(name, None)
    e.empty_display_type = "SPHERE"
    e.empty_display_size = 0.018
    e.location = loc
    collection.objects.link(e)
    e.parent = root

scene["mx_schema"] = "moxiang.art-v2.weapon-source.v1"
scene["mx_asset_id"] = ASSET_ID
scene["mx_weapon_family"] = "blade"
scene["mx_total_length_m"] = 1.375
scene["mx_socket_contract"] = "GRIP,TRAIL_BASE,TRAIL_TIP,FX_PRIMARY"

OUT.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT))
tris = sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in scene.objects if o.type=="MESH")
print(f"MXH_BLADE_SOURCE_OK blend={OUT} meshes={sum(1 for o in scene.objects if o.type=='MESH')} triangles={tris}")
