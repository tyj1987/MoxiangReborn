import bpy
import math
from pathlib import Path

REPO = Path(r"E:\Workspaces\moxiang\repo")
OUT = REPO / "artsource" / "Blender" / "Weapons" / "Sword" / "MX_WPN_SWORD_001.blend"
ASSET_ID = "MX_WPN_SWORD_001"

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
    m=bpy.data.materials.new(name)
    m.use_nodes=True
    bsdf=next((node for node in m.node_tree.nodes if node.type=="BSDF_PRINCIPLED"),None)
    m.diffuse_color=(*color,1)
    if bsdf:
        if bsdf.inputs.get("Base Color"): bsdf.inputs["Base Color"].default_value=(*color,1)
        if bsdf.inputs.get("Metallic"): bsdf.inputs["Metallic"].default_value=metallic
        if bsdf.inputs.get("Roughness"): bsdf.inputs["Roughness"].default_value=roughness
    return m

STEEL=mat("MX_MAT_SWORD_STEEL_PREVIEW",(0.28,0.32,0.36),0.95,0.18)
BRONZE=mat("MX_MAT_SWORD_BRONZE_PREVIEW",(0.32,0.16,0.045),0.82,0.30)
LEATHER=mat("MX_MAT_SWORD_LEATHER_PREVIEW",(0.075,0.025,0.016),0.0,0.62)
BLACK=mat("MX_MAT_SWORD_BLACKENED_PREVIEW",(0.025,0.028,0.03),0.72,0.32)

def move(obj):
    for owner in list(obj.users_collection):
        owner.objects.unlink(obj)
    collection.objects.link(obj)
    obj.parent=root

def finish(obj, material=None, bevel=0.0015):
    move(obj)
    bpy.context.view_layer.objects.active=obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if bevel and obj.type=="MESH":
        mod=obj.modifiers.new("MX_Bevel","BEVEL")
        mod.width=bevel
        mod.segments=3
        mod.limit_method="ANGLE"
        bpy.ops.object.modifier_apply(modifier=mod.name)
    if obj.type=="MESH":
        tri=obj.modifiers.new("MX_Triangulate","TRIANGULATE")
        tri.quad_method="BEAUTY"; tri.ngon_method="BEAUTY"
        bpy.ops.object.modifier_apply(modifier=tri.name)
        if material: obj.data.materials.append(material)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.025)
        bpy.ops.object.mode_set(mode="OBJECT")
    obj.select_set(False)
    return obj

def cube(name,dims,loc,material,rot=(0,0,0),bevel=0.0015):
    bpy.ops.mesh.primitive_cube_add(location=loc,rotation=rot)
    o=bpy.context.object; o.name=name; o.dimensions=dims
    return finish(o,material,bevel)

def cyl(name,radius,depth,loc,material,verts=32,bevel=0.001):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=radius,depth=depth,location=loc)
    o=bpy.context.object; o.name=name
    return finish(o,material,bevel)

def torus(name,major,minor,z,material):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major,minor_radius=minor,major_segments=32,minor_segments=8,
        location=(0,0,z)
    )
    o=bpy.context.object; o.name=name
    return finish(o,material,0)

def build_blade():
    sections=[
        (0.025,0.052,0.0060),
        (0.40,0.050,0.0055),
        (0.72,0.044,0.0048),
        (0.845,0.030,0.0040),
    ]
    verts=[]
    for z,w,t in sections:
        verts.extend([
            (-w/2,0,z),
            (0,t,z),
            (w/2,0,z),
            (0,-t,z),
        ])
    tip_index=len(verts)
    verts.append((0,0,0.915))
    faces=[]
    # bottom cap
    faces.append((0,3,2,1))
    for s in range(len(sections)-1):
        a=s*4; b=(s+1)*4
        for i in range(4):
            faces.append((a+i,a+(i+1)%4,b+(i+1)%4,b+i))
    last=(len(sections)-1)*4
    for i in range(4):
        faces.append((last+i,last+(i+1)%4,tip_index))
    mesh=bpy.data.meshes.new(ASSET_ID+"__BLADE_MESH")
    mesh.from_pydata(verts,[],faces); mesh.update()
    o=bpy.data.objects.new(ASSET_ID+"__BLADE",mesh)
    collection.objects.link(o); o.parent=root
    return finish(o,STEEL,0.0008)

build_blade()

# Darkened central ricasso / visual spine detail.
cube(ASSET_ID+"__RICASSO",(0.019,0.004,0.13),(0,-0.006,0.095),BLACK,bevel=0.0007)

# Wing-shaped guard assembled from beveled segments.
cube(ASSET_ID+"__GUARD_CENTER",(0.105,0.030,0.020),(0,0,0.005),BRONZE,bevel=0.004)
for side in (-1,1):
    sx=side
    cube(
        ASSET_ID+("__GUARD_L1" if side<0 else "__GUARD_R1"),
        (0.082,0.024,0.016),(sx*0.088,0,-0.002),BRONZE,
        rot=(0,math.radians(-side*8),0),bevel=0.0035
    )
    cube(
        ASSET_ID+("__GUARD_L2" if side<0 else "__GUARD_R2"),
        (0.070,0.020,0.014),(sx*0.154,0,-0.012),BRONZE,
        rot=(0,math.radians(-side*18),0),bevel=0.003
    )
    # stylized quillon fin
    cyl(
        ASSET_ID+("__GUARD_FIN_L" if side<0 else "__GUARD_FIN_R"),
        0.010,0.060,(sx*0.190,0,-0.028),BRONZE,verts=20,bevel=0.0015
    )

# Grip core and leather wrapping.
cyl(ASSET_ID+"__GRIP_CORE",0.023,0.245,(0,0,-0.145),LEATHER,verts=32,bevel=0.0012)
for i,z in enumerate([-.045,-.070,-.095,-.120,-.145,-.170,-.195,-.220,-.245]):
    torus(f"{ASSET_ID}__WRAP_{i:02d}",0.0235,0.0024,z,BRONZE if i in (0,8) else BLACK)

# Pommel stack.
cyl(ASSET_ID+"__POMMEL_NECK",0.018,0.035,(0,0,-0.283),BRONZE,verts=24,bevel=0.001)
bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,location=(0,0,-0.315),scale=(0.029,0.022,0.034))
pom=bpy.context.object; pom.name=ASSET_ID+"__POMMEL"
finish(pom,BRONZE,0)
torus(ASSET_ID+"__POMMEL_RING",0.017,0.0035,-0.350,BRONZE)

# Standard runtime sockets. Empties are production semantics, not DCC-only hierarchy.
for name,loc in [
    ("MX_SOCKET_GRIP",(0,0,-0.145)),
    ("MX_SOCKET_TRAIL_BASE",(0,0,0.045)),
    ("MX_SOCKET_TRAIL_TIP",(0,0,0.905)),
    ("MX_SOCKET_FX_GUARD",(0,0,0.010)),
]:
    e=bpy.data.objects.new(name,None)
    e.empty_display_type="SPHERE"; e.empty_display_size=0.018
    e.location=loc
    collection.objects.link(e); e.parent=root

scene["mx_schema"]="moxiang.art-v2.weapon-source.v1"
scene["mx_asset_id"]=ASSET_ID
scene["mx_weapon_family"]="sword"
scene["mx_total_length_m"]=1.265
scene["mx_socket_contract"]="GRIP,TRAIL_BASE,TRAIL_TIP,FX_GUARD"

OUT.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT))
tris=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in scene.objects if o.type=="MESH")
print(f"MXH_SWORD_SOURCE_OK blend={OUT} meshes={sum(1 for o in scene.objects if o.type=='MESH')} triangles={tris}")
