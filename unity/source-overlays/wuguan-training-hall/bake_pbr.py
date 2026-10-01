"""Bake the existing Blender hall into a unique UV atlas. Original sources remain read-only."""
from pathlib import Path
import hashlib, json, math, os, time, struct, zlib
import bpy
import numpy as np
ROOT = Path(__file__).resolve().parents[3]
SOURCE = Path(__file__).resolve().parent / "WuguanTrainingHall.blend"
ART = ROOT / "unity/MoxiangClient/Assets/Moxiang/Art/WuguanTrainingHall"
OUTPUT = ROOT / "modern/out/unity-remaster/wuguan-training-hall"
SIZE = 4096

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def relative(path):
    return Path(path).relative_to(ROOT).as_posix()
def pixels(image):
    values = np.empty(SIZE*SIZE*4, dtype=np.float32)
    image.pixels.foreach_get(values)
    return values.reshape((-1,4))

def write_png(path, values, srgb=False):
    # Explicit independent RGBA channels: never unpremultiply mask RGB by smoothness A.
    data = np.clip(values,0,1).copy()
    if srgb:
        rgb = data[:,:3]
        data[:,:3] = np.where(rgb<=.0031308,12.92*rgb,1.055*np.power(rgb,1/2.4)-.055)
    encoded = np.rint(data*255).astype(np.uint8).reshape((SIZE,SIZE,4))[::-1]
    rows = np.zeros((SIZE,SIZE*4+1),dtype=np.uint8)
    rows[:,1:] = encoded.reshape((SIZE,SIZE*4))
    compressed = zlib.compress(rows.tobytes(),6)
    decoded = np.frombuffer(zlib.decompress(compressed),dtype=np.uint8).reshape(rows.shape)
    if not np.array_equal(decoded[:,1:],encoded.reshape((SIZE,SIZE*4))):
        raise RuntimeError("PNG channel roundtrip failed")
    def chunk(name, body):
        return struct.pack(">I",len(body))+name+body+struct.pack(">I",zlib.crc32(name+body)&0xffffffff)
    header = struct.pack(">IIBBBBB",SIZE,SIZE,8,6,0,0,0)
    content = b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",header)
    if srgb:
        content += chunk(b"sRGB",b"\x00")
    content += chunk(b"IDAT",compressed)+chunk(b"IEND",b"")
    Path(path).write_bytes(content)

def main():
    source_hash = sha(SOURCE)
    stage = OUTPUT / ("pbr-stage-" + time.strftime("%Y%m%d-%H%M%S"))
    stage.mkdir(parents=True, exist_ok=False)
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE), load_ui=False, use_scripts=False)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 8
    scene.render.bake.margin = 16
    scene.render.bake.use_clear = False
    scene.render.bake.use_selected_to_active = False
    scene.render.bake.normal_space = "TANGENT"
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    meshes = sorted([o for o in scene.objects if o.type=="MESH"], key=lambda o:o.name)
    mats = sorted({m for o in meshes for m in o.data.materials if m}, key=lambda m:m.name)
    if {m.name for m in mats}!={"Wood","Paper","Ink","Bronze","Jade","Stone"} or len(meshes)!=66:
        raise RuntimeError("Source structure changed; review required")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in meshes:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.convert(target="MESH")
    meshes = sorted([o for o in scene.objects if o.type=="MESH"], key=lambda o:o.name)
    source_triangles = 0
    for obj in meshes:
        obj.data.calc_loop_triangles()
        source_triangles += len(obj.data.loop_triangles)
    print("SOURCE_EVALUATED_TRIANGLES",source_triangles,flush=True)
    for obj in meshes:
        for uv in list(obj.data.uv_layers):
            obj.data.uv_layers.remove(uv)
        obj.data.uv_layers.new(name="PBR_Atlas")
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.003,
                             area_weight=0, correct_aspect=True, scale_to_bounds=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    # A transient joined bake surface avoids rebuilding Cycles 66 times per pass.
    # Exported objects are NOT joined. Their shared atlas UVs and transforms stay intact.
    verts, faces, uvs, generated, face_materials, smooth = [], [], [], [], [], []
    for obj in meshes:
        mesh = obj.data
        coords = np.array([tuple(v.co) for v in mesh.vertices])
        lo, hi = coords.min(axis=0), coords.max(axis=0)
        gen = (coords-lo)/np.maximum(hi-lo,1e-8)
        start = len(verts)
        verts.extend([tuple(obj.matrix_world @ v.co) for v in mesh.vertices])
        generated.extend(gen.tolist())
        for poly in mesh.polygons:
            faces.append([start+i for i in poly.vertices])
            uvs.extend([tuple(mesh.uv_layers.active.data[i].uv) for i in poly.loop_indices])
            face_materials.append(mats.index(mesh.materials[poly.material_index]))
            smooth.append(poly.use_smooth)
        obj.hide_render = True
        obj.select_set(False)
    bake_mesh = bpy.data.meshes.new("PBR_BakeSurface")
    bake_mesh.from_pydata(verts,[],faces)
    bake_mesh.update()
    for mat in mats:
        bake_mesh.materials.append(mat)
    for i,poly in enumerate(bake_mesh.polygons):
        poly.material_index = face_materials[i]
        poly.use_smooth = smooth[i]
    uv = bake_mesh.uv_layers.new(name="PBR_Atlas")
    uv.data.foreach_set("uv",np.asarray(uvs,dtype=np.float32).ravel())
    attribute = bake_mesh.attributes.new("SourceGenerated","FLOAT_VECTOR","POINT")
    attribute.data.foreach_set("vector",np.asarray(generated,dtype=np.float32).ravel())
    bake_object = bpy.data.objects.new("PBR_BakeSurface",bake_mesh)
    scene.collection.objects.link(bake_object)
    bake_object.select_set(True)
    bpy.context.view_layer.objects.active = bake_object
    attribute_nodes = []
    for mat in mats:
        node = mat.node_tree.nodes.new("ShaderNodeAttribute")
        node.attribute_name = "SourceGenerated"
        for noise in list(mat.node_tree.nodes):
            if noise.type=="TEX_NOISE" and not noise.inputs["Vector"].is_linked:
                mat.node_tree.links.new(node.outputs["Vector"],noise.inputs["Vector"])
        attribute_nodes.append((mat,node))
    states = []
    for mat in mats:
        mat.use_fake_user = True
        nodes = mat.node_tree.nodes
        shader = nodes.get("Principled BSDF")
        output = next(n for n in nodes if n.type=="OUTPUT_MATERIAL" and n.is_active_output)
        original = output.inputs["Surface"].links[0].from_socket
        emission = nodes.new("ShaderNodeEmission")
        target = nodes.new("ShaderNodeTexImage")
        states.append((mat,shader,output,original,emission,target))

    def bake(role, kind, socket=None):
        image = bpy.data.images.new("Hall_"+role,width=SIZE,height=SIZE,alpha=True,float_buffer=True)
        image.generated_color = (0,0,0,0)
        image.colorspace_settings.name = "sRGB" if role=="BaseColor" else "Non-Color"
        for mat,shader,output,original,emission,target in states:
            links = mat.node_tree.links
            if socket:
                for link in list(emission.inputs["Color"].links):
                    links.remove(link)
                value = shader.inputs[socket]
                if value.is_linked:
                    links.new(value.links[0].from_socket,emission.inputs["Color"])
                else:
                    v = value.default_value
                    emission.inputs["Color"].default_value = tuple(v) if socket=="Base Color" else (v,v,v,1)
                links.new(emission.outputs[0],output.inputs["Surface"])
            else:
                links.new(original,output.inputs["Surface"])
            for node in mat.node_tree.nodes:
                node.select = False
            target.image = image
            target.select = True
            mat.node_tree.nodes.active = target
        print("BAKE_START",role,SIZE,"objects",len(meshes),flush=True)
        bpy.ops.object.bake(type=kind,use_clear=False,margin=16)
        data = pixels(image)
        coverage = float(np.mean(data[:,3]>.5))
        if not np.isfinite(data).all() or coverage<.35:
            raise RuntimeError("Incomplete bake: "+role+" coverage="+str(coverage))
        print("BAKE_DONE",role,"coverage",coverage,flush=True)
        return image,data,coverage

    base, bp, bc = bake("BaseColor", "EMIT", "Base Color")
    normal, npixels, nc = bake("Normal", "NORMAL")
    rough, rp, rc = bake("Roughness", "ROUGHNESS")
    metal, mp, mc = bake("Metallic", "EMIT", "Metallic")
    packed = bpy.data.images.new("Hall_MetallicSmoothness",width=SIZE,height=SIZE,alpha=True,float_buffer=True)
    packed.alpha_mode = "CHANNEL_PACKED"  # Mask alpha is data, not transparency.
    packed.colorspace_settings.name = "Non-Color"
    pv = np.zeros_like(mp)
    pv[:,:3] = np.clip(mp[:,:1],0,1)
    pv[:,3] = np.where(rp[:,3]>.5,1-np.clip(rp[:,0],0,1),0)
    packed.pixels.foreach_set(pv.ravel())
    packed.update()
    images = [("BaseColor",base,bc),("Normal",normal,nc),("MetallicSmoothness",packed,mc)]
    records, publications = [], []
    for role,image,coverage in images:
        filename = "Hall_"+role+".png"
        file = stage / filename
        values = bp if role=="BaseColor" else npixels if role=="Normal" else pv
        write_png(file,values,srgb=role=="BaseColor")
        print("PNG_CHANNELS_VERIFIED",role,"depth=8",flush=True)
        image = bpy.data.images.load(str(file),check_existing=False)
        image.colorspace_settings.name = "sRGB" if role=="BaseColor" else "Non-Color"
        image.alpha_mode = "CHANNEL_PACKED" if role=="MetallicSmoothness" else "STRAIGHT"
        if role=="BaseColor": base = image
        elif role=="Normal": normal = image
        else: packed = image
        dest = ART / "Textures" / filename
        records.append({"role":role,"path":relative(dest),"sha256":sha(file),"width":SIZE,"height":SIZE,
                        "coverage":coverage,"colorSpace":"sRGB" if role=="BaseColor" else "Linear"})
        image.pack()
        image.filepath = str(dest)
        publications.append((file,dest))
    bpy.data.objects.remove(bake_object,do_unlink=True)
    bpy.data.meshes.remove(bake_mesh)
    for obj in meshes:
        obj.hide_render = False
    for mat,node in attribute_nodes:
        mat.node_tree.nodes.remove(node)
    for mat,shader,output,original,emission,target in states:
        mat.node_tree.links.new(original,output.inputs["Surface"])
        mat.node_tree.nodes.remove(emission)
        mat.node_tree.nodes.remove(target)
    # Derived authoring scene uses the exact three textures consumed by Unity.
    baked = bpy.data.materials.new("Wuguan_PBR_Atlas")
    baked.use_nodes = True
    n,l = baked.node_tree.nodes,baked.node_tree.links
    n.clear()
    bs = n.new("ShaderNodeBsdfPrincipled")
    output = n.new("ShaderNodeOutputMaterial")
    l.new(bs.outputs["BSDF"],output.inputs["Surface"])
    tb = n.new("ShaderNodeTexImage"); tb.image = base
    tn = n.new("ShaderNodeTexImage"); tn.image = normal
    tp = n.new("ShaderNodeTexImage"); tp.image = packed
    nm = n.new("ShaderNodeNormalMap")
    sep = n.new("ShaderNodeSeparateColor")
    inv = n.new("ShaderNodeMath"); inv.operation = "SUBTRACT"; inv.inputs[0].default_value = 1
    l.new(tb.outputs["Color"],bs.inputs["Base Color"])
    l.new(tn.outputs["Color"],nm.inputs["Color"]); l.new(nm.outputs["Normal"],bs.inputs["Normal"])
    l.new(tp.outputs["Color"],sep.inputs["Color"]); l.new(sep.outputs["Red"],bs.inputs["Metallic"])
    l.new(tp.outputs["Alpha"],inv.inputs[1]); l.new(inv.outputs[0],bs.inputs["Roughness"])
    for obj in meshes:
        obj.data.materials.clear(); obj.data.materials.append(baked)
        for poly in obj.data.polygons:
            poly.material_index = 0
    triangles = 0
    for obj in meshes:
        obj.data.calc_loop_triangles(); triangles += len(obj.data.loop_triangles)
    if triangles!=source_triangles or triangles>100000:
        raise RuntimeError("Topology changed: "+str(triangles))
    model = stage / "WuguanTrainingHall_PBR.fbx"
    for obj in scene.objects:
        obj.select_set(obj.type in {"MESH","EMPTY"})
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.export_scene.fbx(filepath=str(model),use_selection=True,axis_forward="-Z",axis_up="Y",
                             add_leaf_bones=False,bake_anim=False,path_mode="STRIP")
    model_dest = ART / "Source" / model.name
    publications.append((model,model_dest))
    baked_blend = stage / "WuguanTrainingHall_PBR.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(baked_blend),compress=True)
    blend_dest = SOURCE.with_name(baked_blend.name)
    publications.append((baked_blend,blend_dest))
    manifest = {"schema":1,"engine":"CYCLES","blenderVersion":bpy.app.version_string,
        "atlasSize":SIZE,"objects":len(meshes),"triangles":triangles,
        "sourceEvaluatedTriangles":source_triangles,
        "sourceBlend":relative(SOURCE),"sourceBlendSha256":source_hash,
        "bakeScript":relative(Path(__file__).resolve()),"bakeScriptSha256":sha(__file__),
        "modelPath":relative(model_dest),"modelSha256":sha(model),
        "bakedBlendPath":relative(blend_dest),"bakedBlendSha256":sha(baked_blend),
        "textures":records,"normalConvention":"Tangent +X +Y +Z",
        "packing":"Metallic RGB; Smoothness A = 1 - linear Roughness",
        "uvPolicy":"Unique multi-object smart UV atlas on evaluated source meshes",
        "limitations":"No lightmap/AO; no sculpt high-to-low transfer; no commercial art acceptance"}
    if sha(SOURCE)!=source_hash:
        raise RuntimeError("Authoring source changed during bake")
    manifest_path = ART / "pbr-manifest.json"
    old_hashes = {}
    if manifest_path.exists():
        old = json.loads(manifest_path.read_text(encoding="utf-8"))
        old_hashes = {x["path"]:x["sha256"] for x in old["textures"]}
        old_hashes[old["modelPath"]] = old["modelSha256"]
        old_hashes[old["bakedBlendPath"]] = old["bakedBlendSha256"]
    for source,dest in publications:
        if dest.exists() and old_hashes.get(relative(dest))!=sha(dest):
            raise RuntimeError("Untracked or edited bake output: "+str(dest))
    for source,dest in publications:
        dest.parent.mkdir(parents=True,exist_ok=True)
        os.replace(source,dest)
    temp = stage / "pbr-manifest.json"
    temp.write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    os.replace(temp,manifest_path)
    print("WUGUAN_PBR_BAKE_OK textures=3 atlas="+str(SIZE)+" triangles="+str(triangles),flush=True)

if __name__=="__main__":
    main()
