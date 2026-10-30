import bpy,os,math
R=r"C:\moxiang"; D=os.path.join(R,"unity","MoxiangClient","Assets","Moxiang","Art","WuguanTrainingHall","Source"); S=os.path.dirname(__file__); O=os.path.join(R,"modern","out","unity-remaster","wuguan-training-hall")
for p in (D,S,O): os.makedirs(p,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
def M(n,c,rough=.6,metal=.0):
 m=bpy.data.materials.new(n); m.diffuse_color=(*c,1); m.use_nodes=True; b=m.node_tree.nodes.get("Principled BSDF"); b.inputs["Base Color"].default_value=(*c,1); b.inputs["Roughness"].default_value=rough; b.inputs["Metallic"].default_value=metal; return m
W=M("Wood",(.22,.07,.02),.72,0); P=M("Paper",(.7,.62,.48),.88,0); K=M("Ink",(.05,.04,.035),.95,0); B=M("Bronze",(.3,.18,.05),.42,.55); J=M("Jade",(.1,.3,.22),.32,.05); G=M("Stone",(.15,.16,.16),.9,0)
def PROC(m,c1,c2,scale,bump):
 n=m.node_tree.nodes; l=m.node_tree.links; bs=n.get("Principled BSDF"); tex=n.new("ShaderNodeTexNoise"); tex.inputs["Scale"].default_value=scale; tex.inputs["Detail"].default_value=4.0; tex.inputs["Roughness"].default_value=.7; ramp=n.new("ShaderNodeValToRGB"); ramp.color_ramp.elements[0].color=(*c1,1); ramp.color_ramp.elements[1].color=(*c2,1); bp=n.new("ShaderNodeBump"); bp.inputs["Strength"].default_value=bump; bp.inputs["Distance"].default_value=.08; l.new(tex.outputs["Fac"],ramp.inputs["Fac"]); l.new(ramp.outputs["Color"],bs.inputs["Base Color"]); l.new(tex.outputs["Fac"],bp.inputs["Height"]); l.new(bp.outputs["Normal"],bs.inputs["Normal"]); return m
PROC(W,(.09,.018,.006),(.34,.12,.025),3.5,.28); PROC(P,(.54,.48,.36),(.78,.72,.58),48,.08); PROC(G,(.08,.085,.08),(.24,.25,.23),7,.22); PROC(B,(.12,.07,.018),(.42,.27,.07),5,.12); PROC(J,(.04,.16,.12),(.16,.42,.31),9,.06)
def C(n,l,s,m):
 bpy.ops.mesh.primitive_cube_add(location=l); o=bpy.context.object; o.name=n; o.scale=s; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True); o.data.materials.append(m); q=o.modifiers.new("Bevel","BEVEL"); q.width=.035; q.segments=2; return o
def Y(n,l,r,d,m):
 bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=r,depth=d,location=l); o=bpy.context.object; o.name=n; o.data.materials.append(m); return o
def E(n,l):
 o=bpy.data.objects.new(n,None); bpy.context.collection.objects.link(o); o.location=l; return o
C("COL_Floor",(0,0,-.25),(9,7,.20),G); C("Combat_Mat",(0,0,.05),(3.5,3.5,.05),K)
for n,l,s in [("COL_South_L",(-5.25,-6.85,3),(3.75,.15,3)),("COL_South_R",(5.25,-6.85,3),(3.75,.15,3)),("COL_North_A",(-2.2,6.85,3),(6.8,.15,3)),("COL_North_B",(8.0,6.85,3),(1.0,.15,3)),("COL_West",(-8.85,0,3),(.15,7,3)),("COL_East",(8.85,0,3),(.15,7,3))]: C(n,l,s,P)
for x in (-8,-4,0,4,8):
 for y in (-6.1,6.1): C(f"COL_Post_{x}_{y}",(x,y,3),(.22,.22,3),W)
for y in (-6.1,6.1): C(f"Beam_{y}",(0,y,5.7),(8.5,.16,.18),W)
C("Instructor_Dais",(-2.2,5.4,.18),(2.1,.9,.18),W); C("Instructor_Backdrop",(-2.2,6.66,3.25),(2.25,.06,1.65),K)
for side,x in (("L",-7.4),("R",7.4)):
 C("Rack_"+side,(x,1.2,1.15),(.65,.28,1.15),W)
 for i,dx in enumerate((-.5,-.15,.2,.55)): a=Y(f"Weapon_{side}_{i}",(x+dx,1.0,1.45),.022,2.25,B); a.rotation_euler[0]=math.radians(90)
for i,(x,y) in enumerate(((-5.3,-2.5),(5.3,-2.5),(-5.3,2.5),(5.3,2.5)),1):
 Y(f"Dummy_{i}",(x,y,1.05),.20,2.1,W); C(f"DummyArm_{i}",(x,y,1.55),(.62,.09,.09),W); E(f"ANCHOR_Dummy_{i}",(x,y,0))
for n,l in [("ANCHOR_CombatZone",(0,0,0)),("ANCHOR_Instructor",(-2.2,4.8,0)),("ANCHOR_Quest",(.2,5.2,0)),("ANCHOR_WeaponRack_Left",(-7.4,1.2,0)),("ANCHOR_WeaponRack_Right",(7.4,1.2,0)),("ANCHOR_Entrance",(0,-6.2,0)),("ANCHOR_BackyardExit",(5.8,6.2,0))]: E(n,l)
for i,(x,y) in enumerate(((-5.8,-5.1),(5.8,-5.1),(-5.8,5.0),(5.8,5.0)),1): Y(f"Lantern_{i}_Frame",(x,y,4.6),.18,.48,B); C(f"Lantern_{i}_Paper",(x,y,4.6),(.14,.14,.24),P); E(f"ANCHOR_Lantern_{i}",(x,y,4.6))
for i,y in enumerate((-4.4,4.0),1): C(f"Bench_W_{i}",(-7.9,y,.44),(.34,1.2,.10),W); C(f"Bench_E_{i}",(7.9,y,.44),(.34,1.2,.10),W)
for x in (-6,-3,0,3,6): C(f"Ceiling_Rafter_{x}",(x,0,5.55),(.10,6.0,.12),W)
for x in (-3.8,3.8): C(f"Mat_Border_Long_{x}",(x,0,.08),(.08,3.75,.06),J)
for y in (-3.8,3.8): C(f"Mat_Border_Short_{y}",(0,y,.08),(3.75,.08,.06),J)
for side,x in (("W",-8.65),("E",8.65)):
 for j,y in enumerate((-3.6,0,3.6),1):
  C(f"Lattice_{side}_{j}_Frame",(x,y,3.45),(.07,1.05,1.15),W)
  for z in (2.75,3.2,3.65,4.1): C(f"Lattice_{side}_{j}_H_{z}",(x,y,z),(.08,1.0,.035),W)
  for dy in (-.72,-.36,0,.36,.72): C(f"Lattice_{side}_{j}_V_{dy}",(x,y+dy,3.45),(.08,.035,1.08),W)
for i,x in enumerate((-3.8,3.8),1): C(f"Scroll_{i}_Paper",(x,6.69,3.55),(1.0,.025,1.35),P); C(f"Scroll_{i}_Ink",(x,6.655,3.55),(.18,.02,.95),K)
for i,x in enumerate((-4.8,4.8),1): Y(f"Brazier_{i}_Bowl",(x,4.8,.78),.42,.34,B); Y(f"Brazier_{i}_Stem",(x,4.8,.42),.12,.45,B); C(f"Brazier_{i}_Foot",(x,4.8,.16),(.42,.42,.10),G)
C("Plaque",(0,-6.68,4.55),(1.8,.08,.48),W); C("PlaqueJade",(0,-6.57,4.55),(.45,.03,.25),J)
def JOIN_PREFIX(prefix,newname):
 objs=[o for o in bpy.context.scene.objects if o.type=="MESH" and o.name.startswith(prefix)]
 if len(objs)<2: return
 for o in objs:
  bpy.context.view_layer.objects.active=o; o.select_set(True)
  if "Bevel" in o.modifiers: bpy.ops.object.modifier_apply(modifier="Bevel")
  o.select_set(False)
 for o in objs: o.select_set(True)
 bpy.context.view_layer.objects.active=objs[0]; bpy.ops.object.join(); objs[0].name=newname; objs[0].select_set(False)
JOIN_PREFIX("Lattice_","Lattice_All")
JOIN_PREFIX("Mat_Border_","Mat_Border_All")
JOIN_PREFIX("Ceiling_Rafter_","Ceiling_Rafters_All")
root=E("WuguanTrainingHall_Root",(0,0,0))
for o in list(bpy.context.scene.objects):
 if o is not root and o.parent is None:o.parent=root
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(S,"WuguanTrainingHall.blend"))
for o in bpy.context.scene.objects:o.select_set(o.type not in {'CAMERA','LIGHT'})
bpy.context.view_layer.objects.active=root
bpy.ops.export_scene.fbx(filepath=os.path.join(D,"WuguanTrainingHall.fbx"),use_selection=True,axis_forward='-Z',axis_up='Y',add_leaf_bones=False,bake_anim=False)
camd=bpy.data.cameras.new("cam"); cam=bpy.data.objects.new("cam",camd); bpy.context.collection.objects.link(cam); cam.location=(14,-19,12); bpy.context.scene.camera=cam
from mathutils import Vector
cam.rotation_euler=(Vector((0,0,2))-cam.location).to_track_quat('-Z','Y').to_euler()
ld=bpy.data.lights.new("sun","SUN"); ld.energy=2; sun=bpy.data.objects.new("sun",ld); bpy.context.collection.objects.link(sun); sun.rotation_euler=(.7,0,-.5)
bpy.context.scene.render.engine='BLENDER_EEVEE'; bpy.context.scene.render.resolution_x=1280; bpy.context.scene.render.resolution_y=720; bpy.context.scene.render.resolution_percentage=100; bpy.context.scene.render.filepath=os.path.join(O,"blender-preview.png")
bpy.ops.render.render(write_still=True)
print("WUGUAN_BUILD_OK hall=18x14x6 sparring=7x7 entrance=3.0 backyard_exit=2.4")










