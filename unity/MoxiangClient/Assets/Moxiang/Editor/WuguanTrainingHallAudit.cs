using System;
using System.IO;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;
namespace Moxiang.Editor {
public static class WuguanTrainingHallAudit {
 [Serializable] sealed class Report { public int renderers,meshes,vertices,triangles,materials,colliders,triggers,lights,trainingPoints; public string prefabSha256Note="AssetDatabase prefab audit"; }
 public static void Run(){
  var prefab=AssetDatabase.LoadAssetAtPath<GameObject>(WuguanTrainingHallSetup.PrefabPath); if(!prefab) throw new InvalidDataException("Wuguan prefab missing");
  var report=new Report(); var mats=new HashSet<Material>();
  foreach(var r in prefab.GetComponentsInChildren<Renderer>(true)){report.renderers++; foreach(var m in r.sharedMaterials) if(m) mats.Add(m);}
  foreach(var f in prefab.GetComponentsInChildren<MeshFilter>(true)){if(!f.sharedMesh) continue; report.meshes++; report.vertices+=f.sharedMesh.vertexCount; report.triangles+=f.sharedMesh.triangles.Length/3;}
  report.materials=mats.Count;
  foreach(var c in prefab.GetComponentsInChildren<Collider>(true)){report.colliders++; if(c.isTrigger) report.triggers++;}
  report.lights=prefab.GetComponentsInChildren<Light>(true).Length;
  report.trainingPoints=prefab.GetComponentsInChildren<WuguanTrainingPoint>(true).Length;
  if(report.triangles>100000) throw new InvalidDataException("Wuguan triangle budget exceeded: "+report.triangles);
  if(report.renderers>128) throw new InvalidDataException("Wuguan renderer budget exceeded: "+report.renderers);
  if(report.materials>32) throw new InvalidDataException("Wuguan material budget exceeded: "+report.materials);
  if(report.lights>6) throw new InvalidDataException("Wuguan dynamic light budget exceeded: "+report.lights);
  if(report.trainingPoints!=10) throw new InvalidDataException("Wuguan semantic point count drifted: "+report.trainingPoints);
  var output=Path.GetFullPath(Path.Combine(Application.dataPath,"../../../modern/out/unity-remaster/wuguan-training-hall/audit.json")); Directory.CreateDirectory(Path.GetDirectoryName(output)); File.WriteAllText(output,JsonUtility.ToJson(report,true)); Debug.Log("MXH_WUGUAN_AUDIT_OK triangles="+report.triangles+" renderers="+report.renderers+" materials="+report.materials+" lights="+report.lights);
 }
}}
