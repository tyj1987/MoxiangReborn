"""Author high/low/cage pairs and bake geometric normal/AO into the existing atlas."""
from pathlib import Path
import importlib.util, hashlib, json, math, os, time
import bpy
import numpy as np
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
BASE=Path(__file__).resolve().parent
ART=ROOT/"unity/MoxiangClient/Assets/Moxiang/Art/WuguanTrainingHall"
OUT=ROOT/"modern/out/unity-remaster/wuguan-training-hall"
SIZE=4096

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def rel(p): return Path(p).relative_to(ROOT).as_posix()
def count(o):
    o.data.calc_loop_triangles()
    return len(o.data.loop_triangles)
def collection(name):
    c=bpy.data.collections.new(name);bpy.context.scene.collection.children.link(c);return c
def move(o,c):
    for old in list(o.users_collection):old.objects.unlink(o)
    c.objects.link(o)
def merge(objects,name,material):
    vertices,faces,uvs,flags=[],[],[],[]
    for o in objects:
        offset=len(vertices);vertices.extend([tuple(o.matrix_world@v.co) for v in o.data.vertices])
        for f in o.data.polygons:
            faces.append([offset+i for i in f.vertices]);flags.append(f.use_smooth)
            uvs.extend([tuple(o.data.uv_layers.active.data[i].uv) for i in f.loop_indices])
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.update()
    uv=mesh.uv_layers.new(name="PBR_Atlas");uv.data.foreach_set("uv",np.asarray(uvs,dtype=np.float32).ravel())
    for f,smooth in zip(mesh.polygons,flags):f.use_smooth=smooth
    mesh.materials.append(material)
    o=bpy.data.objects.new(name,mesh);bpy.context.scene.collection.objects.link(o);return o

def main():
    manifest_path=ART/"pbr-manifest.json";manifest_hash=sha(manifest_path)
    m=json.loads(manifest_path.read_text());source=ROOT/m["bakedBlendPath"]
    if sha(source)!=m["bakedBlendSha256"]:raise RuntimeError("PBR source hash mismatch")
    spec=importlib.util.spec_from_file_location("atlas_writer",BASE/"bake_pbr.py")
    writer=importlib.util.module_from_spec(spec);spec.loader.exec_module(writer)
    bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False,use_scripts=False)
    scene=bpy.context.scene;scene.render.engine="CYCLES";scene.cycles.device="CPU";scene.cycles.samples=32
    scene.render.bake.use_clear=False;scene.render.bake.margin=12
    low=sorted([o for o in scene.objects if o.type=="MESH"],key=lambda o:o.name)
    if len(low)!=66:raise RuntimeError("Unexpected source mesh count")
    low_collection=collection("LOW_GAME_READY");high_collection=collection("HIGH_REFINED");cage_collection=collection("CAGES")
    high=[];pairs=[]
    for o in low:move(o,low_collection)
    original_material=low[0].data.materials[0]
    for o in low:
        h=o.copy();h.data=o.data.copy();h.name="HIGH_"+o.name;high_collection.objects.link(h)
        refined=o.name.startswith(("Dummy_","DummyArm_","COL_Post_")) or (o.name.startswith("Brazier_") and o.name.endswith("_Bowl")) or o.name=="Plaque"
        if refined:
            bpy.ops.object.select_all(action="DESELECT");h.select_set(True);bpy.context.view_layer.objects.active=h
            modifier=h.modifiers.new("GeometricCarvingGrid","SUBSURF");modifier.subdivision_type="SIMPLE";modifier.levels=3
            bpy.ops.object.modifier_apply(modifier=modifier.name)
            for v in h.data.vertices:
                x,y,z=v.co;normal=v.normal.copy()
                grain=math.sin(x*87+math.sin(z*2.1)*.8)*math.sin(y*49+z*.9)
                detail=.0014*grain
                if o.name.startswith("Dummy_"):
                    radius=math.hypot(x,y)
                    if radius>.15 and abs(z)<1.03:
                        theta=math.atan2(y,x)
                        groove=.0015*math.sin(theta*31+math.sin(z*1.8))
                        dent=.005*math.exp(-((z-.35)/.18)**2)*math.exp(-((theta+1.57)/.5)**2)
                        target=.2+groove-dent
                        v.co.x=x/radius*target;v.co.y=y/radius*target
                    else:v.co+=normal*detail*.3
                elif o.name.startswith("Brazier_"):
                    detail=.0016*math.sin(math.atan2(y,x)*28)*math.sin(z*65)
                    v.co+=normal*detail
                else:
                    cut=.0028*math.exp(-((z+.37*x-.12)/.028)**2)
                    v.co+=normal*(detail-cut)
            h.data.update()
        c=o.copy();c.data=o.data.copy();c.name="CAGE_"+o.name;cage_collection.objects.link(c)
        for v in c.data.vertices:v.co+=v.normal*.04
        c.display_type="WIRE";c.hide_render=True;c.hide_set(True)
        pairs.append({"low":o.name,"high":h.name,"cage":c.name,"refined":refined,
                      "lowTriangles":count(o),"highTriangles":count(h),"cageDistance":.04})
        high.append(h)
    target_material=original_material.copy();target_material.name="AtlasTransferTarget"
    target=merge(low,"BAKE_LOW_SURFACE",target_material)
    cage=target.copy();cage.data=target.data.copy();cage.name="CAGE_LOW_SURFACE";scene.collection.objects.link(cage)
    for v in cage.data.vertices:v.co+=v.normal*.04
    cage.hide_render=True;cage.hide_set(True);move(cage,cage_collection)
    for o in low:o.hide_render=True
    for o in high:o.hide_render=False
    before=bpy.data.images.load(str(ART/"Textures/Hall_Normal.png"),check_existing=False)
    before.colorspace_settings.name="Non-Color";baseline=writer.pixels(before)
    image=bpy.data.images.new("HighLow_Normal",width=SIZE,height=SIZE,alpha=True,float_buffer=True)
    image.colorspace_settings.name="Non-Color";image.pixels.foreach_set(baseline.ravel());image.update()
    node=target_material.node_tree.nodes.new("ShaderNodeTexImage");node.image=image
    target_material.node_tree.nodes.active=node
    bpy.ops.object.select_all(action="DESELECT")
    for o in high:o.select_set(True)
    target.select_set(True);bpy.context.view_layer.objects.active=target
    print("HIGHLOW_BAKE_START",sum(p["refined"] for p in pairs),sum(p["highTriangles"] for p in pairs),flush=True)
    bpy.ops.object.bake(type="NORMAL",use_selected_to_active=True,use_cage=True,cage_object=cage.name,
                        max_ray_distance=.1,normal_space="TANGENT",use_clear=False,margin=12)
    normal_pixels=writer.pixels(image)
    mask=baseline[:,3]>.5
    delta=float(np.mean(np.linalg.norm(normal_pixels[mask,:3]-baseline[mask,:3],axis=1)>.025))
    if not np.isfinite(normal_pixels).all() or delta<.005:raise RuntimeError("Geometric projection produced no measurable detail")
    stage=OUT/("highlow-stage-"+time.strftime("%Y%m%d-%H%M%S"));stage.mkdir(parents=True,exist_ok=False)
    normal_file=stage/"Hall_HighLow_Normal.png";writer.write_png(normal_file,normal_pixels)
    ao=bpy.data.images.new("HighLow_AO",width=SIZE,height=SIZE,alpha=True,float_buffer=True)
    ao.generated_color=(1,1,1,1);ao.colorspace_settings.name="Non-Color";node.image=ao
    print("AO_RAY_BAKE_START",flush=True)
    bpy.ops.object.bake(type="AO",use_selected_to_active=True,use_cage=True,cage_object=cage.name,
                        max_ray_distance=.1,use_clear=False,margin=12)
    ao_pixels=writer.pixels(ao);values=ao_pixels[mask,0]
    stats={"mean":float(np.mean(values)),"min":float(np.min(values)),"max":float(np.max(values)),
           "occludedRatio":float(np.mean(values<.95)),"normalChangedRatio":delta}
    if not np.isfinite(ao_pixels).all() or stats["mean"]<.04 or stats["occludedRatio"]<.005:
        raise RuntimeError("AO bake is empty or invalid: "+str(stats))
    ao_file=stage/"Hall_AO.png";ao_pixels[:,3]=1;writer.write_png(ao_file,ao_pixels)
    target_material.node_tree.nodes.remove(node)
    target_mesh=target.data;bpy.data.objects.remove(target,do_unlink=True);bpy.data.meshes.remove(target_mesh)
    for o in low:o.hide_render=False;o.hide_set(False)
    for o in high:o.hide_render=True;o.hide_set(True)
    # Keep editable geometry pairs, cages and packed bake outputs in a separate authoring file.
    for file in [normal_file,ao_file]:
        baked_image=bpy.data.images.load(str(file),check_existing=False);baked_image.colorspace_settings.name="Non-Color";baked_image.pack()
    derived=stage/"WuguanTrainingHall_HighLow.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(derived),compress=True)
    normal_dest=ART/"Textures"/normal_file.name;ao_dest=ART/"Textures"/ao_file.name
    blend_dest=BASE/derived.name
    report={"schema":1,"sourceBlend":rel(source),"sourceSha256":sha(source),"pbrManifestSha256":manifest_hash,
            "script":rel(Path(__file__).resolve()),"scriptSha256":sha(__file__),"size":SIZE,
            "method":"Cycles selected-to-active with explicit 0.04m cages; geometric high detail",
            "pairs":pairs,"refinedPairs":sum(p["refined"] for p in pairs),"stats":stats,
            "lowTriangles":sum(p["lowTriangles"] for p in pairs),"highTriangles":sum(p["highTriangles"] for p in pairs),
            "normalPath":rel(normal_dest),"normalSha256":sha(normal_file),"aoPath":rel(ao_dest),"aoSha256":sha(ao_file),
            "blendPath":rel(blend_dest),"blendSha256":sha(derived),"scope":"Geometric wear/detail pass; not final commercial art acceptance"}
    if sha(manifest_path)!=manifest_hash or sha(source)!=m["bakedBlendSha256"]:raise RuntimeError("Source changed during bake")
    report_path=ART/"highlow-manifest.json"
    prior=json.loads(report_path.read_text()) if report_path.exists() else {}
    for file,dest,key in [(normal_file,normal_dest,"normalSha256"),(ao_file,ao_dest,"aoSha256"),(derived,blend_dest,"blendSha256")]:
        if dest.exists() and sha(dest)!=prior.get(key):raise RuntimeError("Output edited independently: "+str(dest))
    for file,dest in [(normal_file,normal_dest),(ao_file,ao_dest),(derived,blend_dest)]:os.replace(file,dest)
    tmp=stage/"highlow-manifest.json";tmp.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8");os.replace(tmp,report_path)
    print("WUGUAN_HIGHLOW_AO_OK",report["refinedPairs"],report["lowTriangles"],report["highTriangles"],stats,flush=True)

if __name__=="__main__":main()
