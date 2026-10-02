// Copy into the existing ROG Unity test assembly, not the game's runtime assembly.
using NUnit.Framework;
using UnityEngine;

public sealed class NpcGroundMeasurementCandidateTests
{
    [Test]
    public void RejectsSelfDynamicBallAndExplicitCeilingEvenOnDefaultLayer()
    {
        var room=new GameObject("room");var npc=new GameObject("npc");
        try
        {
            npc.transform.SetParent(room.transform,false);
            var controller=npc.AddComponent<CharacterController>();controller.height=1.8f;controller.center=new Vector3(0,.9f,0);
            Collider Cube(string name,float y)
            {
                var go=GameObject.CreatePrimitive(PrimitiveType.Cube);go.name=name;
                go.transform.SetParent(room.transform,false);go.transform.position=new Vector3(0,y,0);
                go.transform.localScale=new Vector3(4,.2f,4);return go.GetComponent<Collider>();
            }
            var floor=Cube("reviewed-floor",-.1f);
            var ceiling=Cube("ceiling",1.4f);
            var ball=Cube("dynamic-probe",.8f);ball.gameObject.AddComponent<Rigidbody>().isKinematic=true;
            Physics.SyncTransforms();
            var sample=NpcGroundMeasurementCandidate.Measure(npc.transform,room.transform,
                new Collider[]{controller,ball,ceiling,floor},new[]{ceiling},null,null,2.5f,2.5f,.5f);
            Assert.That(sample.groundValid,Is.True);
            Assert.That(sample.groundY,Is.EqualTo(0).Within(.0001));
            Assert.That(sample.groundCollider,Does.StartWith("reviewed-floor#"));
            Assert.That(sample.footMarkerValid,Is.False);
            Assert.That(float.IsNaN(sample.footMarkerGap),Is.True);
            floor.enabled=false;
            sample=NpcGroundMeasurementCandidate.Measure(npc.transform,room.transform,
                new Collider[]{controller,ball,ceiling,floor},new[]{ceiling},null,null,2.5f,2.5f,.5f);
            Assert.That(sample.groundValid,Is.False);
            Assert.That(float.IsNaN(sample.groundY),Is.True);
            Assert.That(sample.reason,Is.EqualTo("no-reviewed-floor-hit"));
        }
        finally {Object.DestroyImmediate(room);if(npc!=null)Object.DestroyImmediate(npc);}
    }

    [Test]
    public void AggregatesBodyPartsButKeepsMarkerAndAabbDistinct()
    {
        var room=new GameObject("room");var npc=new GameObject("npc");
        var mesh=new Mesh();
        try
        {
            var floor=GameObject.CreatePrimitive(PrimitiveType.Cube);
            floor.transform.SetParent(room.transform,false);floor.transform.position=new Vector3(0,-.1f,0);
            floor.transform.localScale=new Vector3(4,.2f,4);
            mesh.vertices=new[]{Vector3.zero,Vector3.up,Vector3.right};mesh.triangles=new[]{0,1,2};
            SkinnedMeshRenderer Part(string name,float minimum)
            {
                var go=new GameObject(name);go.transform.SetParent(npc.transform,false);
                var part=go.AddComponent<SkinnedMeshRenderer>();part.sharedMesh=mesh;
                part.localBounds=new Bounds(new Vector3(0,minimum+.5f,0),Vector3.one);return part;
            }
            var first=Part("body-a",.2f);var second=Part("body-b",-.3f);
            var foot=new GameObject("reviewed-left-foot-marker");foot.transform.SetParent(npc.transform,false);
            foot.transform.localPosition=new Vector3(0,.1f,0);
            Physics.SyncTransforms();
            var sample=NpcGroundMeasurementCandidate.Measure(npc.transform,room.transform,
                new[]{floor.GetComponent<Collider>()},System.Array.Empty<Collider>(),
                new[]{first,second,second},foot.transform,.25f,.5f,.5f);
            Assert.That(sample.groundValid,Is.True);
            Assert.That(sample.bodyMeshCount,Is.EqualTo(2));
            Assert.That(sample.renderBoundsGap,Is.EqualTo(-.3f).Within(.0001));
            Assert.That(sample.footMarkerGap,Is.EqualTo(.1f).Within(.0001));
            npc.transform.position=new Vector3(10,0,0);Physics.SyncTransforms();
            sample=NpcGroundMeasurementCandidate.Measure(npc.transform,room.transform,
                new[]{floor.GetComponent<Collider>()},System.Array.Empty<Collider>(),new[]{first,second},
                foot.transform,.25f,.5f,.5f);
            Assert.That(sample.groundValid,Is.False);
            Assert.That(float.IsNaN(sample.renderBoundsGap),Is.True);
        }
        finally {Object.DestroyImmediate(room);Object.DestroyImmediate(npc);Object.DestroyImmediate(mesh);}
    }
}
