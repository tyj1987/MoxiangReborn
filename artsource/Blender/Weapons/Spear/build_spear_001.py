import bpy
import math
from pathlib import Path

REPO = Path(r"E:\Workspaces\moxiang\repo")
OUT = REPO / "artsource" / "Blender" / "Weapons" / "Spear" / "MX_WPN_SPEAR_001.blend"
ASSET_ID = "MX_WPN_SPEAR_001"

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
root.empty_display_size = 0.10
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

STEEL = mat("MX_MAT_SPEAR_STEEL_PREVIEW", (0.27,0.31,0.34), 0.96, 0.16)
BRONZE = mat("MX_MAT_SPEAR_BRONZE_PREVIEW", (0.31,0.14,0.038), 0.84, 0.28)
WOOD = mat("MX_MAT_SPEAR_WOOD_PREVIEW", (0.10,0.032,0.015), 0.0, 0.54)
BLACK = mat("MX_MAT_SPEAR_BLACKENED_PREVIEW", (0.021,0.023,0.024), 0.72, 0.31)
RED = mat("MX_MAT_SPEAR_TASSEL_PREVIEW", (0.36,0.012,0.008), 0.0, 0.60)

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

def cyl(name, radius, depth, loc, material, verts=32, bevel=0.001, rot=(0,0,0)):
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=verts, radius=radius, depth=depth, location=loc, rotation=rot)
    obj = bpy.context.object
    obj.name = name
    return finish(obj, material, bevel)

def build_head():
    sections = [
        (0.78, 0.020, 0.007),
        (0.92, 0.055, 0.008),
        (1.08, 0.042, 0.006),
    ]
    verts = []
    for z,w,t in sections:
        verts.extend([(-w,0,z),(0,t,z),(w,0,z),(0,-t,z)])
    tip = len(verts)
    verts.append((0,0,1.24))
    faces = [(0,3,2,1)]
    for s in range(len(sections)-1):
        a=s*4
        b=(s+1)*4
        for i in range(4):
            faces.append((a+i,a+(i+1)%4,b+(i+1)%4,b+i))
    last=(len(sections)-1)*4
    for i in range(4):
        faces.append((last+i,last+(i+1)%4,tip))
    mesh=bpy.data.meshes.new(ASSET_ID+"__HEAD_MESH")
    mesh.from_pydata(verts,[],faces)
    mesh.update()
    obj=bpy.data.objects.new(ASSET_ID+"__HEAD",mesh)
    collection.objects.link(obj)
    obj.parent=root
    return finish(obj,STEEL,0.0009)

# 2.15 m dark hardwood shaft with reinforced collars.
cyl(ASSET_ID+"__SHAFT",0.022,1.88,(0,0,-0.16),WOOD,verts=36,bevel=0.0012)
cyl(ASSET_ID+"__GRIP_WRAP",0.025,0.52,(0,0,-0.46),BLACK,verts=36,bevel=0.0010)
cyl(ASSET_ID+"__HEAD_COLLAR",0.034,0.12,(0,0,0.75),BRONZE,verts=32,bevel=0.0013)
cyl(ASSET_ID+"__BUTT_COLLAR",0.031,0.13,(0,0,-1.105),BRONZE,verts=32,bevel=0.0013)
cyl(ASSET_ID+"__BUTT_CAP",0.024,0.12,(0,0,-1.225),STEEL,verts=24,bevel=0.0010)
build_head()

# Stylized tassel strands; later replace/augment with secondary-motion cards/VFX.
for i in range(12):
    angle = math.tau * i / 12.0
    radius = 0.030
    x = math.cos(angle) * radius
    y = math.sin(angle) * radius
    cyl(
        f"{ASSET_ID}__TASSEL_{i:02d}",
        0.0024, 0.20, (x,y,0.635), RED, verts=8, bevel=0.0004,
        rot=(math.radians(7*math.sin(angle)),math.radians(7*math.cos(angle)),0)
    )

for name,loc in [
    ("MX_SOCKET_GRIP",(0,0,-0.46)),
    ("MX_SOCKET_TRAIL_BASE",(0,0,0.80)),
    ("MX_SOCKET_TRAIL_TIP",(0,0,1.23)),
    ("MX_SOCKET_FX_PRIMARY",(0,0,1.04)),
]:
    e=bpy.data.objects.new(name,None)
    e.empty_display_type="SPHERE"
    e.empty_display_size=0.018
    e.location=loc
    collection.objects.link(e)
    e.parent=root

scene["mx_schema"]="moxiang.art-v2.weapon-source.v1"
scene["mx_asset_id"]=ASSET_ID
scene["mx_weapon_family"]="spear"
scene["mx_total_length_m"]=2.53
scene["mx_socket_contract"]="GRIP,TRAIL_BASE,TRAIL_TIP,FX_PRIMARY"

OUT.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT))
tris=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in scene.objects if o.type=="MESH")
print(f"MXH_SPEAR_SOURCE_OK blend={OUT} meshes={sum(1 for o in scene.objects if o.type=='MESH')} triangles={tris}")
