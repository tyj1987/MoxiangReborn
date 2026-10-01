using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
namespace Moxiang.Editor
{
    public static class WuguanMap44Setup
    {
        public const string Derived="Assets/Moxiang/Derived/Map44";
        public const string MeshPath=Derived+"/Map44AuthoritativeWalk.asset";
        [Serializable] public sealed class Source {public string path,sha256;}
        [Serializable] public sealed class Proof {public int schema,mapNumber,tileSize,width,height,collisionBit,walkableCells;public string tileSha256,scenePath;public Source[] sourceRefs;}
        public static Proof ValidateSource()
        {
            var p=JsonUtility.FromJson<Proof>(File.ReadAllText(Derived+"/Map44.binding.json"));
            if(p==null||p.schema!=1||p.mapNumber!=44||p.tileSize!=50||p.width!=1024||p.height!=1024||p.collisionBit!=1||p.walkableCells!=3617||p.scenePath!=WuguanLightingBake.ScenePath)
                throw new InvalidDataException("Map44 provenance contract changed.");
            if(p.sourceRefs==null||p.sourceRefs.Length!=5)throw new InvalidDataException("Map44 source evidence missing.");
            foreach(var source in p.sourceRefs)WuguanTrainingHallPbr.VerifyHash(source.path,source.sha256);
            WuguanTrainingHallPbr.VerifyHash("unity/MoxiangClient/"+Derived+"/44.ttb.bytes",p.tileSha256);
            return p;
        }
        public static Mesh BuildNavigation()
        {
            var proof=ValidateSource();var data=File.ReadAllBytes(Derived+"/44.ttb.bytes");
            var vertices=new List<Vector3>();var triangles=new List<int>();var coords=new MapCoordinates(51200,51200);int cells=0;
            for(int z=0;z<1024;z++)for(int x=0;x<1024;x++)
            {
                if((data[8+(z*1024+x)*2]&1)!=0)continue;
                int a=vertices.Count;float gx=x*50,gz=z*50;
                vertices.Add(coords.ToScene(new Vector3(gx,0,gz)));vertices.Add(coords.ToScene(new Vector3(gx,0,gz+50)));
                vertices.Add(coords.ToScene(new Vector3(gx+50,0,gz+50)));vertices.Add(coords.ToScene(new Vector3(gx+50,0,gz)));
                triangles.AddRange(new[]{a,a+1,a+2,a,a+2,a+3});cells++;
            }
            if(cells!=proof.walkableCells)throw new InvalidDataException("Source navigation cell count mismatch.");
            var mesh=AssetDatabase.LoadAssetAtPath<Mesh>(MeshPath);
            if(!mesh){mesh=new Mesh();AssetDatabase.CreateAsset(mesh,MeshPath);}else mesh.Clear();
            mesh.name="Map44_TTB_Walkable";mesh.SetVertices(vertices);mesh.SetTriangles(triangles,0);mesh.RecalculateNormals();mesh.RecalculateBounds();
            EditorUtility.SetDirty(mesh);AssetDatabase.SaveAssets();return mesh;
        }
        public static MapVisualController.Entry[] AppendReviewedScene(MapVisualController.Entry[] original)
        {
            var entries=(original??Array.Empty<MapVisualController.Entry>()).ToList();
            var existing=entries.Where(x=>x!=null&&x.mapNumber==44).ToArray();
            bool exists=AssetDatabase.LoadAssetAtPath<SceneAsset>(WuguanLightingBake.ScenePath)!=null;
            if(!exists){if(existing.Length>0)throw new InvalidDataException("Registered Map44 scene is missing.");return entries.ToArray();}
            ValidateSource();
            if(!AssetDatabase.LoadAssetAtPath<LightingDataAsset>("Assets/Moxiang/Scenes/Map44_Wuguan/LightingData.asset"))
                throw new InvalidDataException("Map44 must be light-baked before registration.");
            if(existing.Length>1||(existing.Length==1&&existing[0].scenePath!=WuguanLightingBake.ScenePath))
                throw new InvalidDataException("Conflicting Map44 entry.");
            if(existing.Length==0)entries.Add(new MapVisualController.Entry{mapNumber=44,scenePath=WuguanLightingBake.ScenePath});
            return entries.ToArray();
        }
        public static void EnsureBuildRegistrationIfPresent()
        {
            if(AppendReviewedScene(Array.Empty<MapVisualController.Entry>()).Length==0)return;
            var scenes=EditorBuildSettings.scenes.ToList();var index=scenes.FindIndex(x=>x.path==WuguanLightingBake.ScenePath);
            if(index<0)scenes.Add(new EditorBuildSettingsScene(WuguanLightingBake.ScenePath,true));else scenes[index].enabled=true;
            EditorBuildSettings.scenes=scenes.ToArray();
        }
        public static void Bind()
        {
            if(!Application.isBatchMode)throw new InvalidOperationException("Use the repository batch workflow.");
            for(int i=0;i<SceneManager.sceneCount;i++)if(SceneManager.GetSceneAt(i).isDirty)throw new InvalidOperationException("Unsaved scene blocks binding.");
            WuguanMapDependencyCheck.Ensure();
            var mesh=BuildNavigation();
            var scene=EditorSceneManager.OpenScene(WuguanLightingBake.ScenePath,OpenSceneMode.Single);
            var roots=scene.GetRootGameObjects();
            var container=roots.SingleOrDefault(x=>x.name=="Map44Content");
            if(!container){container=new GameObject("Map44Content");foreach(var root in roots)root.transform.SetParent(container.transform,true);}
            var content=container.GetComponent<WuguanMap44Content>();if(!content)content=container.AddComponent<WuguanMap44Content>();
            content.hall=container.GetComponentInChildren<WuguanTrainingHallModule>();
            content.tileSource=AssetDatabase.LoadAssetAtPath<TextAsset>(Derived+"/44.ttb.bytes");
            // Preserve original TTB walkability, not a new hall-shaped gameplay boundary.
            foreach(var collider in content.hall.GetComponentsInChildren<Collider>(true))if(!collider.isTrigger)collider.enabled=false;
            var nav=container.transform.Find("AuthoritativeWalkSurface");
            if(!nav){nav=new GameObject("AuthoritativeWalkSurface").transform;nav.SetParent(container.transform,false);}
            content.navigation=nav.GetComponent<MeshCollider>();if(!content.navigation)content.navigation=nav.gameObject.AddComponent<MeshCollider>();
            content.navigation.sharedMesh=mesh;content.Validate();
            EditorSceneManager.MarkSceneDirty(scene);EditorSceneManager.SaveScene(scene);AssetDatabase.SaveAssets();
            if(LightmapSettings.lightmaps.Length==0||!Lightmapping.lightingDataAsset)throw new InvalidDataException("Map binding lost baked lighting.");
            scene=EditorSceneManager.OpenScene(ConnectionSceneBuilder.ScenePath,OpenSceneMode.Single);
            var controllers=scene.GetRootGameObjects().SelectMany(x=>x.GetComponentsInChildren<MapVisualController>(true)).ToArray();
            if(controllers.Length!=1)throw new InvalidDataException("Expected one existing map presentation controller.");
            var entries=(controllers[0].maps??Array.Empty<MapVisualController.Entry>()).ToList();
            var prior=entries.Where(x=>x!=null&&x.mapNumber==44).ToArray();
            if(prior.Length>1||(prior.Length==1&&prior[0].scenePath!=WuguanLightingBake.ScenePath))throw new InvalidDataException("Conflicting Map44 binding.");
            if(prior.Length==0)entries.Add(new MapVisualController.Entry {mapNumber=44,scenePath=WuguanLightingBake.ScenePath});
            if(!controllers[0].entities)throw new InvalidDataException("Missing existing server entity registry.");
            controllers[0].entities.map44TrainingPrefab=WuguanTrainingDummyPrefab.Build();
            controllers[0].maps=entries.ToArray();EditorSceneManager.MarkSceneDirty(scene);EditorSceneManager.SaveScene(scene);
            var build=EditorBuildSettings.scenes.ToList();var index=build.FindIndex(x=>x.path==WuguanLightingBake.ScenePath);
            if(index<0)build.Add(new EditorBuildSettingsScene(WuguanLightingBake.ScenePath,true));else build[index].enabled=true;
            EditorBuildSettings.scenes=build.ToArray();AssetDatabase.SaveAssets();
            File.WriteAllText(Path.Combine(WuguanTrainingHallPbr.Repository,"modern/out/unity-remaster/wuguan-training-hall/map44-binding-result.txt"),
                "map=44 spawn=10650,12500 input=source-ttb free_cells=3617 baked_scene="+WuguanLightingBake.ScenePath+"\ntravel_end_to_end=not_verified\n");
            Debug.Log("MXH_MAP44_BOUND source_ttb=true maps_preserved=true live_travel_not_verified=true");
        }
    }
}
