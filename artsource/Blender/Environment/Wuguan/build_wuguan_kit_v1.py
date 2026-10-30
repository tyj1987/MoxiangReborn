import bpy
import math
from pathlib import Path

REPO = Path(r"E:\Workspaces\moxiang\repo")
OUT_BLEND = REPO / "artsource" / "Blender" / "Environment" / "Wuguan" / "MX_ENV_WUGUAN_KIT_001.blend"

scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = 1.0

# Clear authored scene data without touching external files.
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for collection in list(bpy.data.collections):
    bpy.data.collections.remove(collection)

def material(name, color, metallic=0.0, roughness=0.55):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = (*color, 1.0)
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
    mat.diffuse_color = (*color, 1.0)
    return mat

MAT_WOOD = material("MX_MAT_WoodDark_Preview", (0.12, 0.045, 0.022), 0.0, 0.48)
MAT_LACQUER = material("MX_MAT_LacquerRed_Preview", (0.30, 0.035, 0.022), 0.0, 0.36)
MAT_STONE = material("MX_MAT_StoneGray_Preview", (0.28, 0.30, 0.30), 0.0, 0.78)
MAT_PLASTER = material("MX_MAT_PlasterWarm_Preview", (0.50, 0.45, 0.36), 0.0, 0.82)
MAT_PAPER = material("MX_MAT_PaperWarm_Preview", (0.78, 0.56, 0.25), 0.0, 0.62)
MAT_BRONZE = material("MX_MAT_Bronze_Preview", (0.17, 0.075, 0.025), 0.72, 0.28)

def new_asset(asset_id):
    collection = bpy.data.collections.new(asset_id)
    scene.collection.children.link(collection)
    root = bpy.data.objects.new(asset_id, None)
    root.empty_display_type = "PLAIN_AXES"
    root.empty_display_size = 0.35
    collection.objects.link(root)
    return collection, root

def move_to_collection(obj, collection):
    for owner in list(obj.users_collection):
        owner.objects.unlink(obj)
    collection.objects.link(obj)

def smart_uv(obj):
    if obj.type != "MESH":
        return
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), island_margin=0.025)
    bpy.ops.object.mode_set(mode="OBJECT")
    obj.select_set(False)

def finish_mesh(obj, collection, root, mat, bevel=0.025):
    move_to_collection(obj, collection)
    obj.parent = root
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if bevel > 0.0:
        mod = obj.modifiers.new("MX_Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 3
        mod.limit_method = "ANGLE"
        bpy.ops.object.modifier_apply(modifier=mod.name)
    tri = obj.modifiers.new("MX_Triangulate", "TRIANGULATE")
    tri.quad_method = "BEAUTY"
    tri.ngon_method = "BEAUTY"
    bpy.ops.object.modifier_apply(modifier=tri.name)
    if mat:
        obj.data.materials.append(mat)
    smart_uv(obj)
    return obj

def box(name, dims, loc, collection, root, mat, bevel=0.025, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dims
    return finish_mesh(obj, collection, root, mat, min(bevel, min(dims) * 0.18))

def cylinder(name, radius, depth, loc, collection, root, mat, vertices=24, bevel=0.018, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    return finish_mesh(obj, collection, root, mat, bevel)

def torus(name, major, minor, loc, collection, root, mat, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major, minor_radius=minor,
        major_segments=24, minor_segments=8,
        location=loc, rotation=rotation
    )
    obj = bpy.context.object
    obj.name = name
    return finish_mesh(obj, collection, root, mat, 0.0)

def build_floor():
    aid = "MX_ENV_WUGUAN_FLOOR_4M_001"
    c, r = new_asset(aid)
    box(aid+"__STONE", (4.0, 4.0, 0.20), (0,0,-0.10), c,r,MAT_STONE,0.035)
    # Recessed wood border gives the tile a reusable martial-hall identity.
    box(aid+"__BORDER_N", (4.0,0.12,0.07), (0,1.94,0.035), c,r,MAT_WOOD,0.018)
    box(aid+"__BORDER_S", (4.0,0.12,0.07), (0,-1.94,0.035), c,r,MAT_WOOD,0.018)
    box(aid+"__BORDER_E", (0.12,3.76,0.07), (1.94,0,0.035), c,r,MAT_WOOD,0.018)
    box(aid+"__BORDER_W", (0.12,3.76,0.07), (-1.94,0,0.035), c,r,MAT_WOOD,0.018)

def build_post():
    aid = "MX_ENV_WUGUAN_POST_6M_001"
    c, r = new_asset(aid)
    box(aid+"__PLINTH_LOW", (0.78,0.78,0.18), (0,0,0.09), c,r,MAT_STONE,0.035)
    box(aid+"__PLINTH_TOP", (0.62,0.62,0.16), (0,0,0.25), c,r,MAT_STONE,0.025)
    box(aid+"__POST", (0.44,0.44,5.34), (0,0,3.00), c,r,MAT_LACQUER,0.035)
    box(aid+"__CAPITAL_1", (0.72,0.62,0.20), (0,0,5.72), c,r,MAT_WOOD,0.025)
    box(aid+"__CAPITAL_2", (0.96,0.46,0.18), (0,0,5.91), c,r,MAT_WOOD,0.022)

def build_beam(aid, length):
    c, r = new_asset(aid)
    box(aid+"__MAIN", (length,0.34,0.38), (0,0,0), c,r,MAT_WOOD,0.035)
    end = length/2 - 0.13
    box(aid+"__END_L", (0.26,0.50,0.48), (-end,0,0), c,r,MAT_LACQUER,0.028)
    box(aid+"__END_R", (0.26,0.50,0.48), (end,0,0), c,r,MAT_LACQUER,0.028)

def build_wall():
    aid = "MX_ENV_WUGUAN_WALL_SOLID_4M_001"
    c, r = new_asset(aid)
    box(aid+"__STONE_BASE", (4.0,0.28,0.42), (0,0,0.21), c,r,MAT_STONE,0.028)
    box(aid+"__PLASTER", (3.72,0.18,2.78), (0,0,1.82), c,r,MAT_PLASTER,0.018)
    box(aid+"__TRIM_TOP", (4.0,0.30,0.26), (0,0,3.34), c,r,MAT_WOOD,0.025)
    box(aid+"__TRIM_L", (0.22,0.25,3.0), (-1.89,0,1.75), c,r,MAT_LACQUER,0.022)
    box(aid+"__TRIM_R", (0.22,0.25,3.0), (1.89,0,1.75), c,r,MAT_LACQUER,0.022)

def build_door():
    aid = "MX_ENV_WUGUAN_WALL_DOOR_4M_001"
    c, r = new_asset(aid)
    box(aid+"__STONE_BASE_L", (0.48,0.32,0.36), (-1.72,0,0.18), c,r,MAT_STONE,0.025)
    box(aid+"__STONE_BASE_R", (0.48,0.32,0.36), (1.72,0,0.18), c,r,MAT_STONE,0.025)
    box(aid+"__POST_L", (0.36,0.36,3.62), (-1.55,0,1.99), c,r,MAT_LACQUER,0.032)
    box(aid+"__POST_R", (0.36,0.36,3.62), (1.55,0,1.99), c,r,MAT_LACQUER,0.032)
    box(aid+"__LINTEL", (3.74,0.44,0.38), (0,0,3.76), c,r,MAT_WOOD,0.035)
    box(aid+"__HEAD_PANEL", (2.80,0.18,0.58), (0,0,3.34), c,r,MAT_PLASTER,0.018)
    box(aid+"__THRESHOLD", (3.00,0.48,0.14), (0,0,0.07), c,r,MAT_STONE,0.022)

def build_lattice():
    aid = "MX_ENV_WUGUAN_LATTICE_2M_001"
    c, r = new_asset(aid)
    box(aid+"__FRAME_TOP", (2.0,0.14,0.14), (0,0,1.13), c,r,MAT_WOOD,0.018)
    box(aid+"__FRAME_BOTTOM", (2.0,0.14,0.14), (0,0,-1.13), c,r,MAT_WOOD,0.018)
    box(aid+"__FRAME_L", (0.14,0.14,2.12), (-0.93,0,0), c,r,MAT_WOOD,0.018)
    box(aid+"__FRAME_R", (0.14,0.14,2.12), (0.93,0,0), c,r,MAT_WOOD,0.018)
    for i, x in enumerate((-0.62,-0.31,0.0,0.31,0.62)):
        box(f"{aid}__V_{i}", (0.075,0.11,2.02), (x,0,0), c,r,MAT_LACQUER,0.010)
    for i, z in enumerate((-0.70,-0.35,0.0,0.35,0.70)):
        box(f"{aid}__H_{i}", (1.78,0.11,0.075), (0,0,z), c,r,MAT_LACQUER,0.010)

def build_dougong():
    aid = "MX_ENV_WUGUAN_DOUGONG_S_001"
    c, r = new_asset(aid)
    box(aid+"__CORE", (0.48,0.48,0.22), (0,0,0), c,r,MAT_LACQUER,0.018)
    box(aid+"__ARM_X_LOW", (1.25,0.34,0.20), (0,0,0.20), c,r,MAT_WOOD,0.022)
    box(aid+"__ARM_Y_LOW", (0.34,1.25,0.20), (0,0,0.20), c,r,MAT_WOOD,0.022)
    box(aid+"__ARM_X_TOP", (1.65,0.28,0.18), (0,0,0.39), c,r,MAT_LACQUER,0.020)
    box(aid+"__ARM_Y_TOP", (0.28,1.65,0.18), (0,0,0.39), c,r,MAT_LACQUER,0.020)

def build_dais():
    aid = "MX_ENV_WUGUAN_DAIS_4M_001"
    c, r = new_asset(aid)
    box(aid+"__PLATFORM", (4.0,2.6,0.44), (0,0,0.22), c,r,MAT_WOOD,0.035)
    box(aid+"__STONE_PLINTH", (4.18,2.78,0.16), (0,0,0.08), c,r,MAT_STONE,0.025)
    box(aid+"__STEP_1", (3.0,0.65,0.18), (0,-1.56,0.09), c,r,MAT_STONE,0.025)
    box(aid+"__STEP_2", (2.4,0.46,0.18), (0,-1.38,0.25), c,r,MAT_WOOD,0.022)

def build_dummy():
    aid = "MX_ENV_WUGUAN_DUMMY_001"
    c, r = new_asset(aid)
    cylinder(aid+"__BASE",0.50,0.22,(0,0,0.11),c,r,MAT_STONE,32,0.025)
    cylinder(aid+"__TRUNK",0.18,2.35,(0,0,1.30),c,r,MAT_WOOD,24,0.022)
    cylinder(aid+"__CAP",0.22,0.20,(0,0,2.53),c,r,MAT_LACQUER,24,0.018)
    cylinder(aid+"__ARM_L",0.075,1.05,(-0.31,0,1.73),c,r,MAT_WOOD,16,0.012,rotation=(0,math.radians(90),math.radians(18)))
    cylinder(aid+"__ARM_R",0.075,1.05,(0.31,0,1.38),c,r,MAT_WOOD,16,0.012,rotation=(0,math.radians(90),math.radians(-18)))
    cylinder(aid+"__ARM_F",0.07,0.92,(0,-0.32,1.55),c,r,MAT_WOOD,16,0.012,rotation=(math.radians(90),0,0))

def build_lantern():
    aid = "MX_ENV_WUGUAN_LANTERN_001"
    c, r = new_asset(aid)
    for x in (-0.36,0.36):
        for y in (-0.36,0.36):
            box(f"{aid}__FRAME_{x}_{y}",(0.055,0.055,0.92),(x,y,0),c,r,MAT_WOOD,0.008)
    box(aid+"__TOP_X",(0.84,0.055,0.055),(0,0,0.46),c,r,MAT_WOOD,0.008)
    box(aid+"__BOT_X",(0.84,0.055,0.055),(0,0,-0.46),c,r,MAT_WOOD,0.008)
    box(aid+"__PAPER_N",(0.68,0.025,0.78),(0,0.35,0),c,r,MAT_PAPER,0.004)
    box(aid+"__PAPER_S",(0.68,0.025,0.78),(0,-0.35,0),c,r,MAT_PAPER,0.004)
    box(aid+"__PAPER_E",(0.025,0.68,0.78),(0.35,0,0),c,r,MAT_PAPER,0.004)
    box(aid+"__PAPER_W",(0.025,0.68,0.78),(-0.35,0,0),c,r,MAT_PAPER,0.004)
    torus(aid+"__RING",0.14,0.025,(0,0,0.62),c,r,MAT_BRONZE,rotation=(math.radians(90),0,0))

def build_weapon_rack():
    aid = "MX_ENV_WUGUAN_WEAPON_RACK_001"
    c, r = new_asset(aid)
    box(aid+"__BASE_L",(0.72,0.54,0.18),(-1.35,0,0.09),c,r,MAT_STONE,0.025)
    box(aid+"__BASE_R",(0.72,0.54,0.18),(1.35,0,0.09),c,r,MAT_STONE,0.025)
    box(aid+"__POST_L",(0.24,0.24,2.35),(-1.35,0,1.28),c,r,MAT_WOOD,0.024)
    box(aid+"__POST_R",(0.24,0.24,2.35),(1.35,0,1.28),c,r,MAT_WOOD,0.024)
    box(aid+"__BAR_TOP",(3.12,0.22,0.24),(0,0,2.23),c,r,MAT_LACQUER,0.024)
    box(aid+"__BAR_LOW",(3.12,0.20,0.22),(0,0,1.15),c,r,MAT_WOOD,0.022)
    for i,x in enumerate((-1.0,-0.5,0.0,0.5,1.0)):
        cylinder(f"{aid}__PEG_{i}",0.045,0.42,(x,-0.18,2.18),c,r,MAT_BRONZE,12,0.008,rotation=(math.radians(90),0,0))

def build_brazier():
    aid = "MX_ENV_WUGUAN_BRAZIER_001"
    c, r = new_asset(aid)
    cylinder(aid+"__FOOT",0.46,0.16,(0,0,0.08),c,r,MAT_STONE,32,0.020)
    cylinder(aid+"__STEM",0.11,0.48,(0,0,0.38),c,r,MAT_BRONZE,20,0.012)
    cylinder(aid+"__BOWL_LOW",0.38,0.18,(0,0,0.68),c,r,MAT_BRONZE,32,0.018)
    cylinder(aid+"__BOWL_RIM",0.48,0.10,(0,0,0.82),c,r,MAT_BRONZE,32,0.018)

def build_bench():
    aid = "MX_ENV_WUGUAN_BENCH_001"
    c, r = new_asset(aid)
    box(aid+"__SEAT",(2.4,0.64,0.18),(0,0,0.58),c,r,MAT_WOOD,0.028)
    for x in (-0.88,0.88):
        box(f"{aid}__LEG_{x}",(0.22,0.48,0.56),(x,0,0.29),c,r,MAT_LACQUER,0.020)
    box(aid+"__BRACE",(1.86,0.18,0.18),(0,0,0.27),c,r,MAT_WOOD,0.018)

def build_eave():
    aid = "MX_ENV_WUGUAN_EAVE_4M_001"
    c, r = new_asset(aid)
    box(aid+"__RAFTER_MAIN",(4.0,0.30,0.30),(0,0,0),c,r,MAT_WOOD,0.025)
    # Repeated roof battens define the silhouette while remaining inexpensive.
    for i,x in enumerate((-1.75,-1.25,-0.75,-0.25,0.25,0.75,1.25,1.75)):
        box(f"{aid}__BATTEN_{i}",(0.16,1.65,0.13),(x,0.58,0.18),c,r,MAT_LACQUER,0.015,rotation=(math.radians(-8),0,0))
    box(aid+"__FASCIA",(4.18,0.20,0.34),(0,1.35,0.03),c,r,MAT_LACQUER,0.022)

build_floor()
build_post()
build_beam("MX_ENV_WUGUAN_BEAM_4M_001", 4.0)
build_beam("MX_ENV_WUGUAN_BEAM_8M_001", 8.0)
build_wall()
build_door()
build_lattice()
build_dougong()
build_dais()
build_dummy()
build_lantern()
build_weapon_rack()
build_brazier()
build_bench()
build_eave()

# Metadata stored in the .blend makes the source self-describing.
scene["mx_schema"] = "moxiang.art-v2.blender-source.v1"
scene["mx_kit_id"] = "MX_ENV_WUGUAN_KIT_001"
scene["mx_reference_bounds_m"] = "18x14x6.45"
scene["mx_reference_policy"] = "legacy semantic/scale reference only; geometry independently authored"
scene["mx_export_collection_count"] = len(bpy.data.collections)

OUT_BLEND.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT_BLEND))

mesh_count = sum(1 for o in bpy.context.scene.objects if o.type == "MESH")
tri_count = sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in bpy.context.scene.objects if o.type == "MESH")
print(f"MXH_WUGUAN_V2_SOURCE_OK blend={OUT_BLEND} collections={len(bpy.data.collections)} meshes={mesh_count} triangles={tri_count}")
