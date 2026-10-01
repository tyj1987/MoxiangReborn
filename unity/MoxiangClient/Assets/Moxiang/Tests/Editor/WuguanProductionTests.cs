using System;
using System.IO;
using System.Linq;
using System.Collections;
using NUnit.Framework;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.SceneManagement;
using UnityEngine.TestTools;
using Object=UnityEngine.Object;
namespace Moxiang.Tests
{
    public sealed class WuguanProductionTests
    {
        [Test] public void StandardMapSetupPreservesAndDeduplicatesReviewedMap44()
        {
            var first=new MapVisualController.Entry{mapNumber=10};var second=new MapVisualController.Entry{mapNumber=2};
            var entries=Moxiang.Editor.WuguanMap44Setup.AppendReviewedScene(new[]{first,second});
            Assert.That(entries.Length,Is.EqualTo(3));Assert.That(entries[0],Is.SameAs(first));Assert.That(entries[1],Is.SameAs(second));
            Assert.That(entries[2].mapNumber,Is.EqualTo(44));
            Assert.That(Moxiang.Editor.WuguanMap44Setup.AppendReviewedScene(entries).Length,Is.EqualTo(3));
            entries[2].scenePath="Unreviewed.unity";
            Assert.Throws<InvalidDataException>(()=>Moxiang.Editor.WuguanMap44Setup.AppendReviewedScene(entries));
        }
        [Test] public void Map44ServerSpawnCreatesSelectableWoodVisualWithServerIdentity()
        {
            var go=new GameObject("Map44ServerEntityTest");var fallback=new GameObject("OtherMapVisual");
            try{
                var connection=go.AddComponent<ConnectionPanel>();var snapshot=new CoreSnapshot{state=CoreState.InGame,sessionGeneration=11,mapGeneration=22};snapshot.game.mapNumber=44;
                var observed=typeof(ConnectionPanel).GetField("observed",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance);
                observed.SetValue(connection,snapshot);
                var registry=go.AddComponent<ServerEntityRegistry>();registry.connection=connection;registry.mapDepth=51200;
                using var surface=new DisplaySurfaceFixture(registry,11,22,0);
                registry.monsterPrefab=fallback;registry.map44TrainingPrefab=AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingDummyPrefab.Path);
                Assert.That(registry.map44TrainingPrefab,Is.Not.Null);
                var dispatch=typeof(ServerEntityRegistry).GetMethod("OnCoreEvent",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance);
                var added=Added();added.argument1=(12500u<<16)|10650u;added.reserved0=73;added.text=new byte[256];
                dispatch.Invoke(registry,new object[]{added});var target=go.GetComponentInChildren<WuguanServerHitReceiver>();
                Assert.That(target,Is.Not.Null);Assert.That(target.ObjectId,Is.EqualTo(9001));Assert.That(target.target.visual.GetComponentsInChildren<MeshRenderer>().Length,Is.EqualTo(2));
                var identity=target.GetComponent<TargetSelectable>();Assert.That(identity.visualKind,Is.EqualTo(73));
                Assert.That(Vector3.Distance(target.transform.position,new MapCoordinates(51200,51200).ToScene(new Vector3(10650,0,12500))),Is.LessThan(.01f));
                dispatch.Invoke(registry,new object[]{Hit()});Assert.That(target.AcceptedHitCount,Is.EqualTo(1));
                target.AdvancePresentation(.09f);Assert.That(target.target.visual.localScale.x,Is.LessThan(1));target.AdvancePresentation(.2f);
                Assert.That(target.target.visual.localScale,Is.EqualTo(Vector3.one));
                snapshot.game.mapNumber=10;observed.SetValue(connection,snapshot);
                var resolve=typeof(ServerEntityRegistry).GetMethod("ResolveMonsterPrefab",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance);
                object[] args={73u,null};Assert.That(resolve.Invoke(registry,args),Is.SameAs(fallback),"Map10 appearance must not be replaced.");
            }finally{Object.DestroyImmediate(go);Object.DestroyImmediate(fallback);}
        }
        static CoreEvent Added(uint id=9001)=>new CoreEvent {type=NativeClient.EventMonsterAdded,result=CoreResult.Ok,state=CoreState.InGame,argument0=id,sessionGeneration=11,mapGeneration=22,sequence=10};
        static CoreEvent Hit(uint id=9001)=>new CoreEvent {type=NativeClient.EventSkillHit,result=CoreResult.Ok,state=CoreState.InGame,argument0=id,argument1=37,reserved0=2,sessionGeneration=11,mapGeneration=22,sequence=11};
        [Test] public void HighLowHasRealPairedGeometryCagesAndRayBakes()
        {
            var e=Moxiang.Editor.WuguanHighLowSetup.Validate();
            Assert.That(e.refinedPairs,Is.EqualTo(21));Assert.That(e.highTriangles,Is.GreaterThan(e.lowTriangles*5));
            Assert.That(e.stats.normalChangedRatio,Is.GreaterThan(.005f));Assert.That(e.stats.occludedRatio,Is.InRange(.005f,.99f));
            var material=AssetDatabase.LoadAssetAtPath<Material>(Moxiang.Editor.WuguanTrainingHallPbr.MaterialPath);
            Assert.That(AssetDatabase.GetAssetPath(material.GetTexture("_BumpMap")),Does.EndWith("Hall_HighLow_Normal.png"));
            Assert.That(AssetDatabase.GetAssetPath(material.GetTexture("_OcclusionMap")),Does.EndWith("Hall_AO.png"));
        }
        [Test] public void ConfirmedHitRequiresRealIdentityGenerationAndMonotonicSequence()
        {
            var go=new GameObject("ServerTarget");var visual=new GameObject("Visual");visual.transform.SetParent(go.transform,false);
            try{
                var selectable=go.AddComponent<TargetSelectable>();selectable.objectId=9001;
                var target=go.AddComponent<WuguanTrainingDummyTarget>();target.visual=visual.transform;
                var receiver=go.AddComponent<WuguanServerHitReceiver>();receiver.target=target;
                Assert.That(receiver.AcceptServerHit(Hit()),Is.False);
                Assert.That(receiver.BindServerEntity(Added(),selectable),Is.True);
                int notifications=0;receiver.ServerHitConfirmed+=_=>notifications++;
                Assert.That(receiver.AcceptServerHit(Hit()),Is.True);Assert.That(receiver.LastDamage,Is.EqualTo(37));
                Assert.That(receiver.HasAuthoritativeDirection,Is.False);Assert.That(notifications,Is.EqualTo(1));
                receiver.AdvancePresentation(.09f);Assert.That(visual.transform.localScale.x,Is.LessThan(1));
                receiver.AdvancePresentation(.2f);Assert.That(visual.transform.localScale,Is.EqualTo(Vector3.one));
                Assert.That(receiver.AcceptServerHit(Hit()),Is.False);
                var wrong=Hit();wrong.sequence=12;wrong.mapGeneration=23;Assert.That(receiver.AcceptServerHit(wrong),Is.False);
                wrong=Hit(9002);wrong.sequence=12;Assert.That(receiver.AcceptServerHit(wrong),Is.False);
                wrong=Hit();wrong.sequence=12;wrong.argument1=0;Assert.That(receiver.AcceptServerHit(wrong),Is.False);
                receiver.ResetBinding();Assert.That(receiver.AcceptServerHit(Hit()),Is.False);
            }finally{Object.DestroyImmediate(go);}
        }
        [Test] public void RegistryDispatchesConfirmedHitWithoutChangingAuthoritativeLife()
        {
            var go=new GameObject("Registry");var source=new GameObject("AuditedTargetPrefab");
            try{
                source.AddComponent<WuguanServerHitReceiver>();
                var registry=go.AddComponent<ServerEntityRegistry>();registry.monsterPrefab=source;
                using var surface=new DisplaySurfaceFixture(registry,11,22,0);
                var dispatch=typeof(ServerEntityRegistry).GetMethod("OnCoreEvent",System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic);
                var added=Added();added.text=new byte[256];dispatch.Invoke(registry,new object[]{added});
                var receiver=go.GetComponentInChildren<WuguanServerHitReceiver>();Assert.That(receiver,Is.Not.Null);
                var health=receiver.GetComponent<ServerEntityHealth>();health.SetCurrentLife(900);
                dispatch.Invoke(registry,new object[]{Hit()});
                Assert.That(receiver.AcceptedHitCount,Is.EqualTo(1));Assert.That(receiver.ObjectId,Is.EqualTo(9001));
                Assert.That(health.CurrentLife,Is.EqualTo(900),"Only server LifeNotify changes health, not hit presentation.");
            }finally{Object.DestroyImmediate(go);Object.DestroyImmediate(source);}
        }
        [Test] public void StaticDummiesDoNotInventServerIds()
        {
            var p=AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingHallSetup.PrefabPath);
            var receivers=p.GetComponentsInChildren<WuguanServerHitReceiver>(true);Assert.That(receivers.Length,Is.EqualTo(4));
            foreach(var receiver in receivers){Assert.That(receiver.ObjectId,Is.Zero);Assert.That(receiver.AcceptServerHit(Hit()),Is.False);}
        }
        [Test] public void OfficialMap44BindingPreservesSourceNavigationAndMap2Map10()
        {
            var proof=Moxiang.Editor.WuguanMap44Setup.ValidateSource();Assert.That(proof.walkableCells,Is.EqualTo(3617));
            var scene=OpenOwned(Moxiang.Editor.WuguanLightingBake.ScenePath,out var owns);
            try{
                var content=scene.GetRootGameObjects().SelectMany(x=>x.GetComponentsInChildren<WuguanMap44Content>()).Single();content.Validate();
                Assert.That(content.navigation.sharedMesh.triangles.Length/3,Is.EqualTo(3617*2));
                Assert.That(content.IsWalkable(content.SpawnGame),Is.True);Assert.That(content.IsWalkable(Vector3.zero),Is.False);
                Physics.SyncTransforms();var point=new MapCoordinates(51200,51200).ToScene(content.SpawnGame);
                Assert.That(content.navigation.Raycast(new Ray(point+Vector3.up*5,Vector3.down),out var hit,10),Is.True);
                Assert.That(Vector3.Distance(hit.point,point),Is.LessThan(.01f));
                foreach(var floor in content.hall.GetComponentsInChildren<MeshRenderer>().Where(x=>x.name=="COL_Floor"||x.name=="Combat_Mat"))
                    Assert.That(floor.bounds.max.y,Is.EqualTo(hit.point.y).Within(.001f),"Map44 art floor must match source navigation height.");
                Assert.That(content.hall.GetComponentsInChildren<Collider>().Where(x=>!x.isTrigger).All(x=>!x.enabled),Is.True);
            }finally{if(owns)EditorSceneManager.CloseScene(scene,true);}
            scene=OpenOwned(Moxiang.Editor.ConnectionSceneBuilder.ScenePath,out owns);
            try{
                var controller=scene.GetRootGameObjects().SelectMany(x=>x.GetComponentsInChildren<MapVisualController>()).Single();
                Assert.That(controller.maps.Count(x=>x.mapNumber==2),Is.EqualTo(1));Assert.That(controller.maps.Count(x=>x.mapNumber==10),Is.EqualTo(1));
                Assert.That(controller.maps.Single(x=>x.mapNumber==44).scenePath,Is.EqualTo(WuguanMap44SceneLoader.ScenePath));
                Assert.That(controller.entities.map44TrainingPrefab,Is.EqualTo(AssetDatabase.LoadAssetAtPath<GameObject>(Moxiang.Editor.WuguanTrainingDummyPrefab.Path)));
            }finally{if(owns)EditorSceneManager.CloseScene(scene,true);}
        }
        [Test] public void LightingIsPersistedAndMovableDummiesRemainProbeLit()
        {
            var path=Path.Combine(Moxiang.Editor.WuguanTrainingHallPbr.Repository,"modern/out/unity-remaster/wuguan-training-hall/lighting-bake.json");
            var evidence=JsonUtility.FromJson<Moxiang.Editor.WuguanLightingBake.Evidence>(File.ReadAllText(path));
            Assert.That(evidence.lightmaps,Is.GreaterThan(0));Assert.That(evidence.staticRenderers,Is.EqualTo(58));Assert.That(evidence.dynamicRenderers,Is.EqualTo(8));
            for(int i=0;i<evidence.colorPaths.Length;i++)Assert.That(Moxiang.Editor.WuguanTrainingHallPbr.Digest(evidence.colorPaths[i]),Is.EqualTo(evidence.colorSha256[i]));
            Assert.That(AssetDatabase.LoadAssetAtPath<LightingDataAsset>(evidence.lightingDataPath),Is.Not.Null);
            var scene=OpenOwned(Moxiang.Editor.WuguanLightingBake.ScenePath,out var owns);
            try{
                var content=scene.GetRootGameObjects().SelectMany(x=>x.GetComponentsInChildren<WuguanMap44Content>()).Single();
                var renderers=content.GetComponentsInChildren<MeshRenderer>();
                Assert.That(renderers.Count(x=>!x.GetComponentInParent<WuguanTrainingDummyTarget>()&&x.lightmapIndex>=0&&x.lightmapIndex<LightmapSettings.lightmaps.Length),Is.EqualTo(58));
                foreach(var r in renderers.Where(x=>x.GetComponentInParent<WuguanTrainingDummyTarget>()))Assert.That(r.receiveGI,Is.EqualTo(ReceiveGI.LightProbes));
                Assert.That(content.GetComponentInChildren<LightProbeGroup>().probePositions.Length,Is.EqualTo(45));
            }finally{if(owns)EditorSceneManager.CloseScene(scene,true);}
        }
        [UnityTest] public IEnumerator RegisteredMap44LoadsBakedSceneAndCancelsStaleGeneration()
        {
            yield return new EnterPlayMode();
            var go=new GameObject("Map44RuntimeLoadTest");var controller=go.AddComponent<MapVisualController>();
            controller.maps=new[]{new MapVisualController.Entry{mapNumber=44,scenePath=WuguanMap44SceneLoader.ScenePath}};
            var snapshot=new CoreSnapshot{state=CoreState.InGame,sessionGeneration=77,mapGeneration=9};snapshot.game.mapNumber=44;
            controller.Observe(snapshot);float deadline=Time.realtimeSinceStartup+20;
            while(!controller.IsReady&&Time.realtimeSinceStartup<deadline){controller.Observe(snapshot);yield return null;}
            Assert.That(controller.IsReady,Is.True,controller.Failure);Assert.That(controller.ActiveRoot.GetComponent<WuguanMap44Content>(),Is.Not.Null);
            Assert.That(LightmapSettings.lightmaps.Length,Is.GreaterThan(0));
            controller.Clear();deadline=Time.realtimeSinceStartup+10;
            while(SceneManager.GetSceneByPath(WuguanMap44SceneLoader.ScenePath).isLoaded&&Time.realtimeSinceStartup<deadline)yield return null;
            Assert.That(SceneManager.GetSceneByPath(WuguanMap44SceneLoader.ScenePath).isLoaded,Is.False);
            snapshot.mapGeneration=10;controller.Observe(snapshot);controller.Clear();
            yield return new WaitForSecondsRealtime(2);
            Assert.That(controller.IsReady,Is.False);Assert.That(SceneManager.GetSceneByPath(WuguanMap44SceneLoader.ScenePath).isLoaded,Is.False);
            Object.Destroy(go);yield return new ExitPlayMode();
        }
        static Scene OpenOwned(string path,out bool owned){var scene=SceneManager.GetSceneByPath(path);owned=!scene.IsValid()||!scene.isLoaded;return owned?EditorSceneManager.OpenScene(path,OpenSceneMode.Additive):scene;}
    }
}
