using NUnit.Framework;
using UnityEditor;
using UnityEngine;
namespace Moxiang.Tests {
public sealed class WuguanTrainingHallTests {
 [Test] public void PbrBuildReceiptMatchesSavedPrefabAndDependencies(){
  var receipt=JsonUtility.FromJson<Moxiang.Editor.WuguanTrainingHallPbr.BuildReceipt>(System.IO.File.ReadAllText(Moxiang.Editor.WuguanTrainingHallPbr.ReceiptPath));
  Moxiang.Editor.WuguanTrainingHallPbr.VerifyHash(receipt.prefabPath,receipt.prefabSha256);
  Moxiang.Editor.WuguanTrainingHallPbr.VerifyHash("unity/MoxiangClient/"+Moxiang.Editor.WuguanTrainingHallPbr.ManifestPath,receipt.manifestSha256);
  Moxiang.Editor.WuguanTrainingHallPbr.VerifyHash("unity/MoxiangClient/"+Moxiang.Editor.WuguanTrainingHallPbr.MaterialPath,receipt.materialSha256);
  Assert.That(receipt.textureCount,Is.EqualTo(3));Assert.That(System.DateTime.Parse(receipt.completedUtc).ToUniversalTime(),Is.LessThanOrEqualTo(System.DateTime.UtcNow));
 }

 [Test] public void PbrExportPreservesOriginalGeometryBoundsAndSurfaceArea(){
  var source=Object.Instantiate(AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Art/WuguanTrainingHall/Source/WuguanTrainingHall.fbx"));
  var baked=Object.Instantiate(AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingHallPbr.ModelPath));
  try{
   var original=source.GetComponentsInChildren<MeshFilter>(true);var derived=baked.GetComponentsInChildren<MeshFilter>(true);
   Assert.That(derived.Length,Is.EqualTo(original.Length));
   foreach(var f in original){
    var match=System.Array.Find(derived,v=>v.name==f.name);Assert.That(match,Is.Not.Null,f.name);
    var b=f.GetComponent<Renderer>().bounds;var d=match.GetComponent<Renderer>().bounds;
    Assert.That(Vector3.Distance(b.center,d.center),Is.LessThan(.003f),f.name+" center");
    Assert.That(Vector3.Distance(b.size,d.size),Is.LessThan(.003f),f.name+" bounds");
    float area=MeshWorldArea(f);Assert.That(MeshWorldArea(match),Is.EqualTo(area).Within(Mathf.Max(.003f,area*.001f)),f.name+" surface area");
   }
  }finally{Object.DestroyImmediate(source);Object.DestroyImmediate(baked);}
 }
 static float MeshWorldArea(MeshFilter f){
  var v=f.sharedMesh.vertices;var t=f.sharedMesh.triangles;float area=0;
  for(int i=0;i<t.Length;i+=3){var a=f.transform.TransformPoint(v[t[i]]);var b=f.transform.TransformPoint(v[t[i+1]]);var c=f.transform.TransformPoint(v[t[i+2]]);area+=Vector3.Cross(b-a,c-a).magnitude*.5f;}
  return area;
 }

 [Test] public void PbrManifestBindsVerifiedAuthoringAndExportBytes(){
  var m=Moxiang.Editor.WuguanTrainingHallPbr.LoadManifest();
  Assert.That(m.textures,Has.Length.EqualTo(3));Assert.That(m.atlasSize,Is.EqualTo(4096));
  Assert.That(m.objects,Is.EqualTo(66));Assert.That(m.triangles,Is.EqualTo(m.sourceEvaluatedTriangles));
 }
 [Test] public void PbrRejectsTamperedDigestAndDuplicateTextureRole(){
  var m=Moxiang.Editor.WuguanTrainingHallPbr.LoadManifest();
  m.modelSha256=new string('0',64);
  Assert.Throws<System.IO.InvalidDataException>(()=>Moxiang.Editor.WuguanTrainingHallPbr.ValidateManifest(m));
  m=Moxiang.Editor.WuguanTrainingHallPbr.LoadManifest();m.textures[0].role=m.textures[1].role;
  Assert.Throws<System.IO.InvalidDataException>(()=>Moxiang.Editor.WuguanTrainingHallPbr.ValidateManifest(m));
 }
 [Test] public void PbrMaterialUsesCorrectColorSpaceAndChannelKeywords(){
  var p=AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingHallSetup.PrefabPath);
  var m=AssetDatabase.LoadAssetAtPath<Material>(Moxiang.Editor.WuguanTrainingHallPbr.MaterialPath);
  Assert.That(m,Is.Not.Null);Assert.That(m.shader.name,Is.EqualTo("Universal Render Pipeline/Lit"));
  Assert.That(m.IsKeywordEnabled("_NORMALMAP"),Is.True);Assert.That(m.IsKeywordEnabled("_METALLICSPECGLOSSMAP"),Is.True);
  Assert.That(m.GetFloat("_Smoothness"),Is.EqualTo(1));Assert.That(m.GetFloat("_SmoothnessTextureChannel"),Is.Zero);
  foreach(var r in p.GetComponentsInChildren<Renderer>(true))foreach(var assigned in r.sharedMaterials)Assert.That(assigned,Is.EqualTo(m));
  var roles=new[]{"BaseColor","Normal","MetallicSmoothness"};var properties=new[]{"_BaseMap","_BumpMap","_MetallicGlossMap"};
  for(int i=0;i<3;i++){
   var texture=m.GetTexture(properties[i]);Assert.That(texture,Is.Not.Null);
   Assert.That(texture.width,Is.EqualTo(4096));Assert.That(texture.height,Is.EqualTo(4096));
   var importer=(TextureImporter)AssetImporter.GetAtPath(AssetDatabase.GetAssetPath(texture));
   Assert.That(importer.sRGBTexture,Is.EqualTo(i==0));Assert.That(importer.mipmapEnabled,Is.True);
   Assert.That(importer.wrapMode,Is.EqualTo(TextureWrapMode.Clamp));
   Assert.That(importer.textureType,Is.EqualTo(i==1?TextureImporterType.NormalMap:TextureImporterType.Default));
   Assert.That(importer.GetPlatformTextureSettings("Standalone").format,Is.EqualTo(i==1?TextureImporterFormat.BC5:TextureImporterFormat.BC7));
  }
 }
 [Test] public void PbrAtlasUvsSurviveImportWithoutMissingChannels(){
  var p=AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingHallSetup.PrefabPath);
  foreach(var filter in p.GetComponentsInChildren<MeshFilter>(true)){
   var mesh=filter.sharedMesh;Assert.That(mesh.uv.Length,Is.EqualTo(mesh.vertexCount),filter.name);
   foreach(var uv in mesh.uv){Assert.That(uv.x,Is.InRange(-.0001f,1.0001f));Assert.That(uv.y,Is.InRange(-.0001f,1.0001f));}
  }
 }
 [Test] public void PbrMetallicSmoothnessPixelsMatchAllSixSourceMaterials(){
  var root=AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingHallSetup.PrefabPath);
  var file=System.IO.Path.Combine(Application.dataPath,"Moxiang/Art/WuguanTrainingHall/Textures/Hall_MetallicSmoothness.png");
  var image=new Texture2D(2,2,TextureFormat.RGBA32,false,true);
  try{
   Assert.That(image.LoadImage(System.IO.File.ReadAllBytes(file)),Is.True);
   var names=new[]{"COL_Floor","COL_South_L","Dummy_1","Weapon_L_0","Mat_Border_All","Combat_Mat"};
   var metallic=new[]{0f,0f,0f,.55f,.05f,0f};var smoothness=new[]{.10f,.12f,.28f,.58f,.68f,.05f};
   for(int n=0;n<names.Length;n++){
    var filter=System.Array.Find(root.GetComponentsInChildren<MeshFilter>(true),x=>x.name==names[n]);
    Assert.That(filter,Is.Not.Null,names[n]);var uv=filter.sharedMesh.uv;var triangles=filter.sharedMesh.triangles;
    float largest=-1;Vector2 sample=Vector2.zero;
    for(int i=0;i<triangles.Length;i+=3){var a=uv[triangles[i]];var b=uv[triangles[i+1]];var c=uv[triangles[i+2]];
     float area=Mathf.Abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x));
     if(area>largest){largest=area;sample=(a+b+c)/3f;}
    }
    Assert.That(largest,Is.GreaterThan(.0000001f));
    var value=image.GetPixel(Mathf.Clamp((int)(sample.x*image.width),0,image.width-1),Mathf.Clamp((int)(sample.y*image.height),0,image.height-1));
    Assert.That(value.r,Is.EqualTo(metallic[n]).Within(.012f),names[n]+" metallic R");
    Assert.That(value.a,Is.EqualTo(smoothness[n]).Within(.012f),names[n]+" smoothness A");
   }
  }finally{Object.DestroyImmediate(image);}
 }

 [Test] public void PreviewCameraIsAboveFloorAndFramesCombatZone(){
  var asset=AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingHallSetup.PrefabPath);
  var instance=Object.Instantiate(asset); var cameraObject=new GameObject("TestCamera");
  try{var m=instance.GetComponent<WuguanTrainingHallModule>(); var camera=cameraObject.AddComponent<Camera>();
   Moxiang.Editor.WuguanTrainingHallPreview.ConfigureCamera(camera,m);
   Assert.That(camera.transform.position.y,Is.GreaterThan(m.combatZone.position.y+.5f));
   var v=camera.WorldToViewportPoint(m.combatZone.position+Vector3.up*1.2f);
   Assert.That(v.z,Is.GreaterThan(camera.nearClipPlane)); Assert.That(v.x,Is.InRange(.05f,.95f)); Assert.That(v.y,Is.InRange(.05f,.95f));
  }finally{Object.DestroyImmediate(cameraObject); Object.DestroyImmediate(instance);}
 }
 [Test] public void DummyReactionPreservesAuthoredRestRotation(){
  var root=new GameObject("RestPoseTest"); var visual=new GameObject("Visual"); visual.transform.SetParent(root.transform,false);
  try{var rest=Quaternion.Euler(12f,31f,9f); visual.transform.localRotation=rest; var d=root.AddComponent<WuguanTrainingDummyTarget>(); d.visual=visual.transform;
   d.ApplyVisualReaction(Vector3.forward,0f); Assert.That(Quaternion.Angle(rest,visual.transform.localRotation),Is.LessThan(.01f));
   d.ApplyVisualReaction(Vector3.forward,.5f); Assert.That(Quaternion.Angle(rest,visual.transform.localRotation),Is.GreaterThan(1f));
   d.ApplyVisualReaction(Vector3.forward,1f); Assert.That(Quaternion.Angle(rest,visual.transform.localRotation),Is.LessThan(.01f));
   Assert.That(d.EvaluateLocalRotation(Vector3.forward,float.NaN),Is.EqualTo(Quaternion.identity));
   Assert.That(d.EvaluateLocalRotation(new Vector3(float.PositiveInfinity,0,0),.5f),Is.EqualTo(Quaternion.identity));
  }finally{Object.DestroyImmediate(root);}
 }
 [Test] public void MissingArrayReferencesCannotPassConfiguration(){
  var p=AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingHallSetup.PrefabPath); var go=Object.Instantiate(p);
  try{var m=go.GetComponent<WuguanTrainingHallModule>(); var old=m.dummyAnchors[0]; m.dummyAnchors[0]=null;
   Assert.That(m.IsConfigured,Is.False); m.dummyAnchors[0]=old; m.lanternAnchors[0]=null; Assert.That(m.IsConfigured,Is.False);
  }finally{Object.DestroyImmediate(go);}
 }
 [Test] public void DummyBodyAndArmMoveAsOneRigidVisual(){
  var p=AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingHallSetup.PrefabPath); var go=Object.Instantiate(p);
  try{foreach(var d in go.GetComponentsInChildren<WuguanTrainingDummyTarget>(true)){
   var body=System.Array.Find(go.GetComponentsInChildren<Transform>(true),x=>x.name=="Dummy_"+d.index);
   var arm=System.Array.Find(go.GetComponentsInChildren<Transform>(true),x=>x.name=="DummyArm_"+d.index);
   Assert.That(body.IsChildOf(d.visual),Is.True); Assert.That(arm.IsChildOf(d.visual),Is.True);
   var distance=Vector3.Distance(body.position,arm.position); d.ApplyVisualReaction(Vector3.forward,.5f);
   Assert.That(Vector3.Distance(body.position,arm.position),Is.EqualTo(distance).Within(.001f));
  }}finally{Object.DestroyImmediate(go);}
 }
 [Test] public void ImportedGeometryAndTriggersUseMetresAndYUp(){
  var p=AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingHallSetup.PrefabPath); var go=Object.Instantiate(p);
  try{var floor=System.Array.Find(go.GetComponentsInChildren<MeshRenderer>(true),x=>x.name=="COL_Floor");
   Assert.That(floor,Is.Not.Null); Assert.That(floor.bounds.size.x,Is.EqualTo(18f).Within(.02f)); Assert.That(floor.bounds.size.z,Is.EqualTo(14f).Within(.02f));
   Assert.That(go.transform.localScale,Is.EqualTo(Vector3.one));
   Assert.That(Quaternion.Angle(go.transform.rotation,Quaternion.identity),Is.LessThan(.01f));
   Physics.SyncTransforms();
   var floorCollider=System.Array.Find(go.GetComponentsInChildren<BoxCollider>(true),x=>x.name=="Collision_COL_Floor");
   Assert.That(floorCollider,Is.Not.Null);
   Assert.That(Vector3.Distance(floorCollider.bounds.center,floor.bounds.center),Is.LessThan(.02f));
   Assert.That(Vector3.Distance(floorCollider.bounds.size,floor.bounds.size),Is.LessThan(.02f));
   foreach(var point in go.GetComponentsInChildren<WuguanTrainingPoint>(true))
    Assert.That(Vector3.Dot(point.transform.up,Vector3.up),Is.GreaterThan(.999f),point.name);
   var m=go.GetComponent<WuguanTrainingHallModule>(); var box=m.combatZone.GetComponent<BoxCollider>();
   Assert.That(m.LocalTrainingBounds.center,Is.EqualTo(box.center)); Assert.That(m.LocalTrainingBounds.size,Is.EqualTo(box.size));
  }finally{Object.DestroyImmediate(go);}
 }
 [Test] public void PreviewCapturePreservesOpenSceneAndRenderTarget(){
  var sentinel=new GameObject("PreviewMustNotDestroyThis");
  var scene=UnityEngine.SceneManagement.SceneManager.GetActiveScene();
  var count=UnityEngine.SceneManagement.SceneManager.sceneCount; var target=RenderTexture.active;
  try{Moxiang.Editor.WuguanTrainingHallPreview.Capture(); Assert.That(sentinel,Is.Not.Null);
   Assert.That(UnityEngine.SceneManagement.SceneManager.GetActiveScene(),Is.EqualTo(scene));
   Assert.That(UnityEngine.SceneManagement.SceneManager.sceneCount,Is.EqualTo(count)); Assert.That(RenderTexture.active,Is.EqualTo(target));
  }finally{if(sentinel)Object.DestroyImmediate(sentinel);}
 }
 [Test] public void BlenderAssetProducesConfiguredReusablePrefab(){
  var p=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Art/WuguanTrainingHall/Prefabs/WuguanTrainingHall.prefab");
  Assert.That(p,Is.Not.Null);
  var m=p.GetComponent<WuguanTrainingHallModule>();
  Assert.That(m,Is.Not.Null);
  Assert.That(m.IsConfigured,Is.True);
  Assert.That(m.dummyAnchors,Has.Length.EqualTo(4)); Assert.That(m.HallSize,Is.EqualTo(new Vector3(18f,6f,14f))); Assert.That(m.backyardExitAnchor,Is.Not.Null); Assert.That(m.lanternAnchors,Has.Length.EqualTo(4));
  Assert.That(m.LocalTrainingBounds.size,Is.EqualTo(new Vector3(7f,2.5f,7f)));
 }
 [Test] public void TrainingHallPointQueryIsStable(){ var p=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Art/WuguanTrainingHall/Prefabs/WuguanTrainingHall.prefab"); var m=p.GetComponent<WuguanTrainingHallModule>(); Assert.That(m.TryGetPoint(WuguanTrainingPointKind.Instructor,0,out var instructor),Is.True); Assert.That(instructor.transform,Is.EqualTo(m.instructorAnchor)); Assert.That(m.TryGetPoint(WuguanTrainingPointKind.Dummy,4,out var dummy),Is.True); Assert.That(dummy.index,Is.EqualTo(4)); Assert.That(m.TryGetPoint(WuguanTrainingPointKind.Dummy,99,out _),Is.False); }
 [Test] public void DummyHitReactionIsVisualOnlyAndDeterministic(){ var p=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Art/WuguanTrainingHall/Prefabs/WuguanTrainingHall.prefab"); var ds=p.GetComponentsInChildren<WuguanTrainingDummyTarget>(true); Assert.That(ds.Length,Is.EqualTo(4)); foreach(var d in ds){ Assert.That(d.visual,Is.Not.Null); Assert.That(d.EvaluateLocalRotation(Vector3.forward,0f),Is.EqualTo(Quaternion.identity)); Assert.That(Quaternion.Angle(Quaternion.identity,d.EvaluateLocalRotation(Vector3.forward,.5f)),Is.GreaterThan(1f)); Assert.That(d.EvaluateLocalRotation(Vector3.forward,1f),Is.EqualTo(Quaternion.identity)); } }
 [Test] public void TrainingHallMaterialsUseValidShaders(){ var p=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Art/WuguanTrainingHall/Prefabs/WuguanTrainingHall.prefab"); var rs=p.GetComponentsInChildren<Renderer>(true); Assert.That(rs.Length,Is.GreaterThan(20)); foreach(var r in rs) foreach(var m in r.sharedMaterials){ Assert.That(m,Is.Not.Null,r.name); Assert.That(m.shader,Is.Not.Null,r.name); Assert.That(m.shader.name,Is.Not.EqualTo("Hidden/InternalErrorShader"),r.name); Assert.That(m.shader.isSupported,Is.True,r.name+" / "+m.shader.name); } }
 [Test] public void TrainingHallRuntimePointsAreTyped(){ var p=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Art/WuguanTrainingHall/Prefabs/WuguanTrainingHall.prefab"); var pts=p.GetComponentsInChildren<WuguanTrainingPoint>(true); Assert.That(pts.Length,Is.EqualTo(10)); Assert.That(System.Array.FindAll(pts,x=>x.kind==WuguanTrainingPointKind.Dummy).Length,Is.EqualTo(4)); Assert.That(System.Array.FindAll(pts,x=>x.kind==WuguanTrainingPointKind.WeaponRack).Length,Is.EqualTo(2)); Assert.That(System.Array.Find(pts,x=>x.kind==WuguanTrainingPointKind.Instructor).IsInteractive,Is.True); Assert.That(System.Array.Find(pts,x=>x.kind==WuguanTrainingPointKind.Entrance).IsInteractive,Is.False); Assert.That(System.Array.Find(pts,x=>x.kind==WuguanTrainingPointKind.Instructor).GetComponent<CapsuleCollider>().isTrigger,Is.True); Assert.That(System.Array.Find(pts,x=>x.kind==WuguanTrainingPointKind.Quest).GetComponent<CapsuleCollider>().isTrigger,Is.True); Assert.That(System.Array.FindAll(pts,x=>x.kind==WuguanTrainingPointKind.WeaponRack)[0].GetComponent<CapsuleCollider>().isTrigger,Is.True); }
 [Test] public void TrainingHallCollisionAndTriggerAreAuthored(){
  var p=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Art/WuguanTrainingHall/Prefabs/WuguanTrainingHall.prefab");
  Assert.That(p,Is.Not.Null);
  Assert.That(p.GetComponentsInChildren<Collider>(true).Length,Is.GreaterThanOrEqualTo(5));
  var m=p.GetComponent<WuguanTrainingHallModule>();
  Assert.That(m.combatZone.GetComponent<BoxCollider>().isTrigger,Is.True); Assert.That(p.GetComponentsInChildren<Light>(true).Length,Is.GreaterThanOrEqualTo(4)); Assert.That(System.Array.FindAll(p.GetComponentsInChildren<Light>(true),x=>x.shadows!=LightShadows.None).Length,Is.LessThanOrEqualTo(2)); foreach(var d in m.dummyAnchors) Assert.That(d.GetComponent<CapsuleCollider>().isTrigger,Is.True);
 }
}}









