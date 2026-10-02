// Standalone candidate for the ROG acceptance harness. Not a game fix and not
// applied to any alleged AcceptanceRuntime.cs path. Requires explicit bindings.
using System;
using System.Collections.Generic;
using UnityEngine;

public static class NpcGroundMeasurementCandidate
{
    public struct Sample
    {
        public bool groundValid, boundsValid, footMarkerValid;
        public float groundY, renderBoundsGap, footMarkerGap;
        public string reason, groundCollider;
        public int bodyMeshCount;
    }

    // approvedFloors MUST exclude roofs/ceilings/walls by source identity or
    // an explicit reviewed binding. Do not pass every room mesh automatically.
    // maxAbove/maxBelow are measurement windows, not movement/physics settings.
    public static Sample Measure(Transform npcRoot, Transform roomRoot,
        Collider[] approvedFloors, Collider[] excludedSurfaces, SkinnedMeshRenderer[] bodyParts,
        Transform footMarker, float maxAbove, float maxBelow, float minUpDot)
    {
        var result = new Sample { groundY=float.NaN,renderBoundsGap=float.NaN,
            footMarkerGap=float.NaN,reason="invalid-input",groundCollider="" };
        if(npcRoot==null || roomRoot==null || approvedFloors==null || excludedSurfaces==null ||
            !float.IsFinite(maxAbove)||!float.IsFinite(maxBelow)||maxAbove<0||maxBelow<=0||
            !float.IsFinite(minUpDot)||minUpDot<=0||minUpDot>1)return result;
        if(footMarker!=null && !footMarker.IsChildOf(npcRoot))
        { result.reason="foot-marker-outside-npc";return result; }
        var reference=footMarker!=null ? footMarker.position : npcRoot.position;
        if(!float.IsFinite(reference.x)||!float.IsFinite(reference.y)||!float.IsFinite(reference.z))return result;
        var ray=new Ray(reference+Vector3.up*maxAbove,Vector3.down);
        float distance=maxAbove+maxBelow;
        if(!float.IsFinite(distance))return result;
        RaycastHit best=default; Collider selected=null;
        foreach(var collider in approvedFloors)
        {
            if(collider==null || !collider.enabled || !collider.gameObject.activeInHierarchy || collider.isTrigger ||
                collider.attachedRigidbody!=null || collider is CharacterController ||
                Array.IndexOf(excludedSurfaces,collider)>=0 ||
                !collider.transform.IsChildOf(roomRoot) || collider.transform.IsChildOf(npcRoot))continue;
            // Query ONLY the reviewed static floor: self CharacterController,
            // moving test ball and arbitrary Default-layer objects cannot win.
            if(!collider.Raycast(ray,out var hit,distance) || Vector3.Dot(hit.normal,Vector3.up)<minUpDot)continue;
            if(selected==null || hit.distance<best.distance){selected=collider;best=hit;}
        }
        if(selected==null){result.reason="no-reviewed-floor-hit";return result;}
        result.groundValid=true;result.groundY=best.point.y;
        result.groundCollider=selected.name+"#"+selected.GetInstanceID();
        Bounds combined=default;
        var seen=new HashSet<SkinnedMeshRenderer>();
        foreach(var part in bodyParts ?? Array.Empty<SkinnedMeshRenderer>())
        {
            if(part==null || !seen.Add(part) || !part.enabled || !part.gameObject.activeInHierarchy || !part.transform.IsChildOf(npcRoot))continue;
            var bounds=part.bounds;
            if(!float.IsFinite(bounds.min.y)||!float.IsFinite(bounds.max.y))continue;
            if(result.bodyMeshCount==0)combined=bounds;else combined.Encapsulate(bounds);
            ++result.bodyMeshCount;
        }
        result.boundsValid=result.bodyMeshCount>0;
        if(result.boundsValid)result.renderBoundsGap=combined.min.y-best.point.y;
        result.footMarkerValid=footMarker!=null;
        if(result.footMarkerValid)result.footMarkerGap=footMarker.position.y-best.point.y;
        result.reason=result.footMarkerValid ? "foot-marker-and-render-bounds" : "render-bounds-only-not-anatomical-feet";
        return result;
    }
}
