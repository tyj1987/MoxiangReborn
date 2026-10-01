using System;
using UnityEngine;
namespace Moxiang {
public sealed class WuguanTrainingHallModule : MonoBehaviour {
 public Transform combatZone,instructorAnchor,questAnchor,entranceAnchor,backyardExitAnchor,weaponRackLeft,weaponRackRight;
 public Transform[] dummyAnchors=Array.Empty<Transform>();
 public Transform[] lanternAnchors=Array.Empty<Transform>();
 public bool IsConfigured=>combatZone&&instructorAnchor&&questAnchor&&entranceAnchor&&backyardExitAnchor&&weaponRackLeft&&weaponRackRight&&Complete(dummyAnchors)&&Complete(lanternAnchors);
 public Vector3 HallSize=>new Vector3(18f,6f,14f);
 public Bounds LocalTrainingBounds=>new Bounds(new Vector3(0f,1.25f,0f),new Vector3(7f,2.5f,7f));
 private static bool Complete(Transform[] anchors){if(anchors==null||anchors.Length!=4)return false; foreach(var anchor in anchors)if(!anchor)return false; return true;}
 public bool TryGetPoint(WuguanTrainingPointKind kind,int index,out WuguanTrainingPoint point){ foreach(var p in GetComponentsInChildren<WuguanTrainingPoint>(true)){ if(p.kind==kind&&p.index==index){point=p; return true;} } point=null; return false; }
}}



