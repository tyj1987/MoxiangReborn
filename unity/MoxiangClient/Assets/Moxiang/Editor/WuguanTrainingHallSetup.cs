using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;
namespace Moxiang.Editor {
public static class WuguanTrainingHallSetup {
 public const string ModelPath=WuguanTrainingHallPbr.ModelPath;
 public const string PrefabPath="Assets/Moxiang/Art/WuguanTrainingHall/Prefabs/WuguanTrainingHall.prefab";
 public static void Build(){
  WuguanTrainingHallPbr.LoadManifest();
  AssetDatabase.ImportAsset(ModelPath,ImportAssetOptions.ForceSynchronousImport|ImportAssetOptions.ForceUpdate);
  var model=AssetDatabase.LoadAssetAtPath<GameObject>(ModelPath);
  if(!model) throw new InvalidDataException("Missing Blender FBX");
  Directory.CreateDirectory("Assets/Moxiang/Art/WuguanTrainingHall/Prefabs");
  var visual=(GameObject)PrefabUtility.InstantiatePrefab(model);
  if(!visual) throw new InvalidOperationException("Instantiate failed");
  PrefabUtility.UnpackPrefabInstance(visual,PrefabUnpackMode.Completely,InteractionMode.AutomatedAction);
  var go=new GameObject("WuguanTrainingHall");
  visual.name="Visual";
  // Preserve imported geometry in world metres, isolate FBX axis conversion from runtime objects.
  visual.transform.SetParent(go.transform,true);
  try{
   WuguanTrainingHallPbr.Apply(go);
   foreach(var f in go.GetComponentsInChildren<MeshFilter>(true).Where(x=>x.name.StartsWith("COL_",StringComparison.Ordinal)&&x.sharedMesh).ToArray()){
    var proxy=new GameObject("Collision_"+f.name);
    proxy.transform.SetParent(go.transform,true);
    proxy.transform.SetPositionAndRotation(f.transform.position,f.transform.rotation);
    proxy.transform.localScale=f.transform.lossyScale;
    var c=proxy.AddComponent<BoxCollider>();
    c.center=f.sharedMesh.bounds.center; c.size=f.sharedMesh.bounds.size;
   }
   var m=go.AddComponent<WuguanTrainingHallModule>();
   var sourceCombat=R(go.transform,"ANCHOR_CombatZone");
   var runtimeCombat=new GameObject("Runtime_CombatZone"); runtimeCombat.transform.SetParent(go.transform,true); runtimeCombat.transform.position=sourceCombat.position;
   m.combatZone=runtimeCombat.transform;
   m.instructorAnchor=P(go.transform,"ANCHOR_Instructor","Runtime_InstructorSlot"); Mark(m.instructorAnchor,WuguanTrainingPointKind.Instructor,0);
   m.questAnchor=P(go.transform,"ANCHOR_Quest","Runtime_QuestSlot"); Mark(m.questAnchor,WuguanTrainingPointKind.Quest,0);
   m.entranceAnchor=P(go.transform,"ANCHOR_Entrance","Runtime_Entrance"); Mark(m.entranceAnchor,WuguanTrainingPointKind.Entrance,0);
   m.backyardExitAnchor=P(go.transform,"ANCHOR_BackyardExit","Runtime_BackyardExit"); Mark(m.backyardExitAnchor,WuguanTrainingPointKind.BackyardExit,0);
   m.weaponRackLeft=P(go.transform,"ANCHOR_WeaponRack_Left","Runtime_WeaponRack_Left"); Mark(m.weaponRackLeft,WuguanTrainingPointKind.WeaponRack,1);
   m.weaponRackRight=P(go.transform,"ANCHOR_WeaponRack_Right","Runtime_WeaponRack_Right"); Mark(m.weaponRackRight,WuguanTrainingPointKind.WeaponRack,2);
   Trigger(m.instructorAnchor,1.0f,2.2f); Trigger(m.questAnchor,.8f,1.8f); Trigger(m.weaponRackLeft,.9f,1.8f); Trigger(m.weaponRackRight,.9f,1.8f);
   m.dummyAnchors=Enumerable.Range(1,4).Select(i=>P(go.transform,"ANCHOR_Dummy_"+i,"Runtime_Dummy_"+i)).ToArray();
   for(int i=0;i<m.dummyAnchors.Length;i++){var d=m.dummyAnchors[i]; Mark(d,WuguanTrainingPointKind.Dummy,i+1); var q=d.gameObject.AddComponent<CapsuleCollider>(); q.isTrigger=true; q.radius=.55f; q.height=2.2f; q.center=new Vector3(0,1.1f,0); var hit=d.gameObject.AddComponent<WuguanTrainingDummyTarget>(); hit.index=i+1; hit.visual=DummyVisual(go.transform,d,i+1); var receiver=d.gameObject.AddComponent<WuguanServerHitReceiver>(); receiver.target=hit;}
   m.lanternAnchors=Enumerable.Range(1,4).Select(i=>P(go.transform,"ANCHOR_Lantern_"+i,"Runtime_Lantern_"+i)).ToArray();
   for(int i=0;i<m.lanternAnchors.Length;i++){var a=m.lanternAnchors[i]; var l=a.gameObject.AddComponent<Light>(); l.type=LightType.Point; l.color=new Color(1f,.68f,.36f); l.intensity=2.0f; l.range=5.5f; l.shadows=i<2?LightShadows.Soft:LightShadows.None;}
   var z=m.combatZone.gameObject.AddComponent<BoxCollider>();
   z.isTrigger=true; z.center=new Vector3(0,1.25f,0); z.size=new Vector3(7f,2.5f,7f);
   if(!PrefabUtility.SaveAsPrefabAsset(go,PrefabPath)) throw new IOException("Prefab save failed");
  } finally { UnityEngine.Object.DestroyImmediate(go); }
  AssetDatabase.SaveAssets();
  WuguanTrainingHallPbr.WriteBuildReceipt();
  Debug.Log("MXH_WUGUAN_TRAINING_HALL_READY source=blender runtime_proxies=true lights=4 gameplay_binding=false");
 }
 public static void Validate(){
  var p=AssetDatabase.LoadAssetAtPath<GameObject>(PrefabPath);
  if(!p) throw new InvalidDataException("Prefab missing");
  var m=p.GetComponent<WuguanTrainingHallModule>();
  if(m==null||!m.IsConfigured) throw new InvalidDataException("Anchors incomplete");
  if(p.GetComponentsInChildren<Collider>(true).Length<13) throw new InvalidDataException("Collision incomplete");
  if(p.GetComponentsInChildren<Light>(true).Length<4) throw new InvalidDataException("Lighting incomplete");
  Debug.Log("MXH_WUGUAN_TRAINING_HALL_VALID runtime_proxies=true lights=4");
 }
 static Transform DummyVisual(Transform root,Transform anchor,int index)
 {
  var body=R(root,"Dummy_"+index);
  var arm=R(root,"DummyArm_"+index);
  var pivot=new GameObject("Runtime_DummyVisual_"+index).transform;
  pivot.SetParent(anchor,false);
  body.SetParent(pivot,true);
  arm.SetParent(pivot,true);
  return pivot;
 }
 static void Trigger(Transform t,float radius,float height){var q=t.gameObject.AddComponent<CapsuleCollider>(); q.isTrigger=true; q.radius=radius; q.height=height; q.center=new Vector3(0,height*.5f,0);}
 static void Mark(Transform t,WuguanTrainingPointKind kind,int index){var p=t.gameObject.AddComponent<WuguanTrainingPoint>(); p.kind=kind; p.index=index;}
 static Transform P(Transform r,string source,string runtime){var s=R(r,source); var g=new GameObject(runtime); g.transform.SetParent(r,true); g.transform.position=s.position; g.transform.rotation=r.rotation; return g.transform;}
 static Transform R(Transform r,string n){
  foreach(var t in r.GetComponentsInChildren<Transform>(true)) if(t.name==n)return t;
  throw new InvalidDataException("Missing "+n);
 }
}}












