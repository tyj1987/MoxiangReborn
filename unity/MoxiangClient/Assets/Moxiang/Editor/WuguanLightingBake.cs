using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.SceneManagement;
namespace Moxiang.Editor
{
    public static class WuguanLightingBake
    {
        public const string ScenePath="Assets/Moxiang/Scenes/Map44_Wuguan.unity";
        public const string SettingsPath="Assets/Moxiang/Art/WuguanTrainingHall/Map44_Wuguan.lighting";
        [Serializable] public sealed class Evidence
        {
            public string completedUtc,scenePath,lightingDataPath,sceneSha256;
            public int staticRenderers,dynamicRenderers,lightmaps,probeCount;
            public string[] colorPaths,colorSha256;
            public int mapNumber=44;
            public Vector3 canonicalSpawn=new Vector3(10650,0,12500);
            public string scope="Baked game content; map binding, navigation and live combat require separate tests.";
        }
        public static void Bake()
        {
            if(!Application.isBatchMode)throw new InvalidOperationException("Run this bake through the project batch pipeline.");
            if(Lightmapping.isRunning)throw new InvalidOperationException("Another light bake is running.");
            foreach(var scene in Enumerable.Range(0,SceneManager.sceneCount).Select(SceneManager.GetSceneAt))
                if(scene.isDirty)throw new InvalidOperationException("Refusing to close an unsaved scene.");
            var importer=AssetImporter.GetAtPath(WuguanTrainingHallPbr.ModelPath) as ModelImporter;
            if(!importer)throw new InvalidDataException("PBR FBX importer missing.");
            importer.generateSecondaryUV=true;importer.SaveAndReimport();
            WuguanTrainingHallSetup.Build();
            var sceneResult=EditorSceneManager.NewScene(NewSceneSetup.EmptyScene,NewSceneMode.Single);
            var prefab=AssetDatabase.LoadAssetAtPath<GameObject>(WuguanTrainingHallSetup.PrefabPath);
            var root=(GameObject)PrefabUtility.InstantiatePrefab(prefab,sceneResult);
            var m=root.GetComponent<WuguanTrainingHallModule>();
            // The legacy Suryun origin is authoritative; do not invent a portal route.
            var spawn=new MapCoordinates(51200,51200).ToScene(new Vector3(10650,0,12500));
            root.transform.position=spawn-m.combatZone.position;
            // Match the legacy fixed-height navigation plane; the asset preview used raised floor inserts.
            foreach(var floorName in new[]{"COL_Floor","Combat_Mat"})
            {
                var floor=root.GetComponentsInChildren<MeshRenderer>(true).Single(x=>x.name==floorName);
                floor.transform.position+=Vector3.up*(spawn.y-floor.bounds.max.y);
            }
            int statics=0,dynamics=0;
            foreach(var renderer in root.GetComponentsInChildren<MeshRenderer>(true))
            {
                bool moving=renderer.GetComponentInParent<WuguanTrainingDummyTarget>()!=null;
                GameObjectUtility.SetStaticEditorFlags(renderer.gameObject,moving?0:StaticEditorFlags.ContributeGI);
                renderer.receiveGI=moving?ReceiveGI.LightProbes:ReceiveGI.Lightmaps;
                renderer.lightProbeUsage=LightProbeUsage.BlendProbes;
                if(moving)dynamics++;else statics++;
            }
            foreach(var light in root.GetComponentsInChildren<Light>(true))light.lightmapBakeType=LightmapBakeType.Mixed;
            var sun=new GameObject("Daylight").AddComponent<Light>();sun.type=LightType.Directional;
            sun.color=new Color(1,.89f,.73f);sun.intensity=1.4f;sun.transform.rotation=Quaternion.Euler(42,-35,0);
            sun.lightmapBakeType=LightmapBakeType.Baked;sun.shadows=LightShadows.Soft;
            RenderSettings.ambientMode=AmbientMode.Trilight;
            RenderSettings.ambientSkyColor=new Color(.28f,.33f,.42f);
            RenderSettings.ambientEquatorColor=new Color(.15f,.14f,.12f);
            RenderSettings.ambientGroundColor=new Color(.045f,.04f,.035f);
            var settings=AssetDatabase.LoadAssetAtPath<LightingSettings>(SettingsPath);
            if(!settings){settings=new LightingSettings();AssetDatabase.CreateAsset(settings,SettingsPath);}
            settings.bakedGI=true;settings.realtimeGI=false;settings.lightmapper=LightingSettings.Lightmapper.ProgressiveCPU;
            settings.lightmapResolution=24;settings.lightmapMaxSize=2048;settings.lightmapPadding=4;
            settings.directSampleCount=64;settings.indirectSampleCount=256;settings.environmentSampleCount=64;
            settings.minBounces=2;settings.maxBounces=4;settings.ao=true;settings.aoMaxDistance=.6f;
            settings.aoExponentIndirect=1;settings.aoExponentDirect=0;
            settings.directionalityMode=LightmapsMode.CombinedDirectional;
            Lightmapping.lightingSettings=settings;EditorUtility.SetDirty(settings);
            var probes=new GameObject("HallLightProbes").AddComponent<LightProbeGroup>();
            probes.transform.position=spawn;
            var positions=new System.Collections.Generic.List<Vector3>();
            foreach(float y in new[]{.35f,1.5f,3f})foreach(float x in new[]{-6f,-3f,0f,3f,6f})foreach(float z in new[]{-4.5f,0f,4.5f})positions.Add(new Vector3(x,y,z));
            probes.probePositions=positions.ToArray();
            Directory.CreateDirectory(Path.GetDirectoryName(ScenePath));
            if(!EditorSceneManager.SaveScene(sceneResult,ScenePath))throw new IOException("Cannot save lighting scene.");
            AssetDatabase.SaveAssets();
            Debug.Log("MXH_WUGUAN_LIGHTBAKE_START static="+statics+" dynamic="+dynamics);
            if(!Lightmapping.Bake())throw new InvalidOperationException("Unity lightmapper failed.");
            var maps=LightmapSettings.lightmaps;
            if(maps==null||maps.Length==0||!Lightmapping.lightingDataAsset)throw new InvalidDataException("No baked lightmaps produced.");
            var paths=maps.Select(x=>AssetDatabase.GetAssetPath(x.lightmapColor)).ToArray();
            if(paths.Any(string.IsNullOrEmpty))throw new InvalidDataException("Unpersisted lightmap output.");
            if(root.GetComponentsInChildren<MeshRenderer>(true).Where(x=>!x.GetComponentInParent<WuguanTrainingDummyTarget>()).Any(x=>x.lightmapIndex<0||x.lightmapIndex>=maps.Length))
                throw new InvalidDataException("Static renderer did not receive a lightmap.");
            EditorSceneManager.SaveScene(sceneResult);AssetDatabase.SaveAssets();
            var evidence=new Evidence {completedUtc=DateTime.UtcNow.ToString("O"),scenePath=ScenePath,
                lightingDataPath=AssetDatabase.GetAssetPath(Lightmapping.lightingDataAsset),sceneSha256=WuguanTrainingHallPbr.Digest(ScenePath),
                staticRenderers=statics,dynamicRenderers=dynamics,lightmaps=maps.Length,probeCount=positions.Count,
                colorPaths=paths,colorSha256=paths.Select(WuguanTrainingHallPbr.Digest).ToArray()};
            var report=Path.Combine(WuguanTrainingHallPbr.Repository,"modern/out/unity-remaster/wuguan-training-hall/lighting-bake.json");
            File.WriteAllText(report,JsonUtility.ToJson(evidence,true));
            Debug.Log("MXH_WUGUAN_LIGHTBAKE_OK lightmaps="+maps.Length+" probes="+positions.Count);
        }
    }
}
