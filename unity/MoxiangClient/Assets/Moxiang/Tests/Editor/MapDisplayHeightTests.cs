using NUnit.Framework;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class MapDisplayHeightTests
    {
        [Test]
        public void RectangularGridUsesXColumnsAndZRows()
        {
            var field=ScriptableObject.CreateInstance<ImportedHeightField>();
            try
            {
                field.descriptor=new HeightFieldDescriptor {heightCountX=3,heightCountZ=2,width=200,depth=100,faceSize=100};
                field.heights=new[]{0f,1000f,2000f,10000f,11000f,12000f};
                Assert.That(MapDisplayHeight.TrySample(field,150,25,out var height),Is.True);
                Assert.That(height,Is.EqualTo(4000));
                Assert.That(MapDisplayHeight.TrySample(field,200,100,out height),Is.True);
                Assert.That(height,Is.EqualTo(12000));
                Assert.That(MapDisplayHeight.TrySample(field,0,101,out _),Is.False);
                Assert.That(MapDisplayHeight.TrySample(field,0,-1,out _),Is.False);
            }
            finally {Object.DestroyImmediate(field);}
        }

        [Test]
        public void ActualMap10SampleCannotUseFormerZeroEntityHeight()
        {
            var field=UnityEditor.AssetDatabase.LoadAssetAtPath<ImportedHeightField>(
                "Assets/Moxiang/Derived/Map10/Map10.mxhasset");
            Assert.That(field,Is.Not.Null);
            Assert.That(field.descriptor.heightSha256,Is.EqualTo("2177825dd2057f4bbebfe8d7a520a4caa5847d9e6e2d4bc5ecc1b62c70c023f7"));
            Assert.That(MapDisplayHeight.TrySample(field,44653,7829,out var height),Is.True);
            Assert.That(height*.001f,Is.InRange(5.078f,5.089f));
        }

        [Test]
        public void SceneSurfaceUsesOnlyItsOwnedColliderAndRejectsMisses()
        {
            var host=new GameObject("RegistryTest");
            var floor=GameObject.CreatePrimitive(PrimitiveType.Cube);
            try
            {
                using var surface=new DisplaySurfaceFixture(host.AddComponent<ServerEntityRegistry>());
                floor.transform.position=new Vector3(0,1,0);floor.transform.localScale=new Vector3(10,2,10);
                var flags=System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic;
                typeof(MapVisualController).GetField("displayHeight",flags).SetValue(surface.Controller,null);
                typeof(MapVisualController).GetField("sceneGround",flags).SetValue(surface.Controller,floor.GetComponent<Collider>());
                Physics.SyncTransforms();
                Assert.That(surface.Controller.TryGetDisplayPosition(0,1,25600,25600,out var point),Is.True);
                Assert.That(point.y,Is.EqualTo(2).Within(.0001));
                Assert.That(surface.Controller.TryGetDisplayPosition(0,1,0,0,out _),Is.False);
                floor.GetComponent<Collider>().enabled=false;
                Assert.That(surface.Controller.TryGetDisplayPosition(0,1,25600,25600,out _),Is.False);
            }
            finally {Object.DestroyImmediate(host);Object.DestroyImmediate(floor);}
        }

        [Test]
        public void SurfaceRejectsStaleGenerationMissingDestinationAndTransformedOrigin()
        {
            var host=new GameObject("RegistryTest");
            try
            {
                var registry=host.AddComponent<ServerEntityRegistry>();
                using var surface=new DisplaySurfaceFixture(registry,3,7);
                Assert.That(surface.Controller.TryGetDisplayPosition(3,7,100,200,out var position),Is.True);
                Assert.That(position.y,Is.EqualTo(5));
                Assert.That(surface.Controller.TryGetDisplayPosition(2,7,100,200,out _),Is.False);
                Assert.That(surface.Controller.TryGetDisplayPosition(3,8,100,200,out _),Is.False);
                surface.Controller.ActiveRoot.transform.position=Vector3.right;
                Assert.That(surface.Controller.TryGetDisplayPosition(3,7,100,200,out _),Is.False);
                surface.Controller.ActiveRoot.transform.position=Vector3.zero;
                surface.Controller.Observe(new CoreSnapshot {state=CoreState.InGame,sessionGeneration=3,mapGeneration=8,
                    game=new CoreGame {mapNumber=17}});
                Assert.That(surface.Controller.IsReady,Is.False);
                Assert.That(surface.Controller.Failure,Is.Not.Empty);
                Assert.That(surface.Controller.TryGetDisplayPosition(3,7,100,200,out _),Is.False);
                surface.Controller.Observe(new CoreSnapshot {state=CoreState.InGame,sessionGeneration=3,mapGeneration=9,
                    game=new CoreGame {mapNumber=10}});
                Assert.That(surface.Controller.TryGetDisplayPosition(3,9,100,200,out position),Is.True);
                surface.Controller.Clear();
                Assert.That(surface.Controller.TryGetDisplayPosition(3,9,100,200,out _),Is.False);
            }
            finally {Object.DestroyImmediate(host);}
        }

        [Test]
        public void MissingSurfaceDoesNotMaterializeAtZeroHeight()
        {
            var host=new GameObject("RegistryTest");var prefab=new GameObject("NpcPrefab");
            try
            {
                var registry=host.AddComponent<ServerEntityRegistry>();registry.npcPrefab=prefab;
                var dispatch=typeof(ServerEntityRegistry).GetMethod("OnCoreEvent",
                    System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic);
                UnityEngine.TestTools.LogAssert.Expect(LogType.Error,
                    "Entity display height unavailable: no current surface, invalid coordinates or unsupported map transform.");
                dispatch.Invoke(registry,new object[]{new CoreEvent {type=NativeClient.EventNpcAdded,
                    state=CoreState.InGame,argument0=572,argument1=100u|(200u<<16),text=new byte[256]}});
                Assert.That(host.transform.childCount,Is.Zero);
                Assert.That(registry.PresentationFailure,Is.Not.Empty);
            }
            finally {Object.DestroyImmediate(host);Object.DestroyImmediate(prefab);}
        }

        [Test]
        public void SamplesVisibleTerrainDiagonalWithoutFlatteningOrBilinearInterpolation()
        {
            var field = ScriptableObject.CreateInstance<ImportedHeightField>();
            try
            {
                field.descriptor = new HeightFieldDescriptor { heightCountX=2,heightCountZ=2,
                    width=100,depth=100,faceSize=100 };
                field.heights = new[] { 1000f,2000f,3000f,9000f };
                Assert.That(MapDisplayHeight.TrySample(field,75,25,out var lower),Is.True);
                Assert.That(lower,Is.EqualTo(3500)); // 00,10,11 visible triangle
                Assert.That(MapDisplayHeight.TrySample(field,25,75,out var upper),Is.True);
                Assert.That(upper,Is.EqualTo(4000)); // 00,11,01 visible triangle
                Assert.That(MapDisplayHeight.TrySample(field,50,50,out var diagonal),Is.True);
                Assert.That(diagonal,Is.EqualTo(5000)); // Not bilinear 3750, nor inspection diagonal 2500.
                var scene = new MapCoordinates(100,100).ToScene(new Vector3(75,lower,25));
                Assert.That(scene.x,Is.EqualTo(.025f).Within(.000001));
                Assert.That(scene.y,Is.EqualTo(3.5f).Within(.000001));
                Assert.That(scene.z,Is.EqualTo(-.025f).Within(.000001));
                Assert.That(MapDisplayHeight.TrySample(field,100,100,out var edge),Is.True);
                Assert.That(edge,Is.EqualTo(9000));
                Assert.That(MapDisplayHeight.TrySample(field,0,0,out edge),Is.True);
                Assert.That(edge,Is.EqualTo(1000));
                foreach(var invalid in new[] {-1f,100.01f,float.NaN,float.PositiveInfinity})
                    Assert.That(MapDisplayHeight.TrySample(field,invalid,0,out _),Is.False);
                field.heights[3]=float.NaN;
                Assert.That(MapDisplayHeight.TrySample(field,50,50,out _),Is.False);
                field.heights=new float[3];
                Assert.That(MapDisplayHeight.TrySample(field,0,0,out _),Is.False);
                field.heights=new float[4]; field.descriptor.depth=200;
                Assert.That(MapDisplayHeight.TrySample(field,0,0,out _),Is.False);
                Assert.That(MapDisplayHeight.TrySample(null,0,0,out _),Is.False);
            }
            finally { Object.DestroyImmediate(field); }
        }
    }
}
