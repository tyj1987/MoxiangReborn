using System;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace Moxiang.Editor
{
    public static class MapVisualSetup
    {
        public static void CaptureMap2()
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Derived/Map2/Map2.mxhterrain");
            if (prefab == null) throw new InvalidDataException("Missing Map2 terrain.");
            UnityEngine.Object.Instantiate(prefab);
            var sun = new GameObject("PreviewSun").AddComponent<Light>();
            sun.type = LightType.Directional; sun.intensity = 1.3f;
            sun.transform.rotation = Quaternion.Euler(50, -30, 0);
            var camera = new GameObject("PreviewCamera").AddComponent<Camera>();
            camera.transform.position = new Vector3(38, 48, -48);
            camera.transform.LookAt(Vector3.zero);
            camera.clearFlags = CameraClearFlags.SolidColor;
            camera.backgroundColor = new Color(0.08f, 0.1f, 0.13f);
            var target = new RenderTexture(1280, 720, 24);
            var pixels = new Texture2D(1280, 720, TextureFormat.RGB24, false);
            try
            {
                camera.targetTexture = target; camera.Render(); RenderTexture.active = target;
                pixels.ReadPixels(new Rect(0, 0, 1280, 720), 0, 0); pixels.Apply();
                string output = Path.GetFullPath(Path.Combine(Application.dataPath, "../../../modern/out/unity-remaster/map2-development-preview.png"));
                File.WriteAllBytes(output, pixels.EncodeToPNG());
                Debug.Log("MXH_MAP2_ASSET_PREVIEW_SAVED not_gameplay=true override_unaccepted=true");
            }
            finally
            {
                RenderTexture.active = null; camera.targetTexture = null;
                target.Release(); UnityEngine.Object.DestroyImmediate(target); UnityEngine.Object.DestroyImmediate(pixels);
            }
        }

        public static void Apply()
        {
            if (UnityEngine.SceneManagement.SceneManager.GetActiveScene().isDirty)
                throw new InvalidOperationException("Save the active scene before map presentation setup.");
            var scene = EditorSceneManager.OpenScene(ConnectionSceneBuilder.ScenePath, OpenSceneMode.Single);
            var connection = UnityEngine.Object.FindFirstObjectByType<ConnectionPanel>();
            var input = UnityEngine.Object.FindFirstObjectByType<Map10InputController>();
            var registry = UnityEngine.Object.FindFirstObjectByType<ServerEntityRegistry>();
            var anchor = GameObject.Find("Map10GeometryInspection");
            if (connection == null || anchor == null) throw new InvalidDataException("Missing connection scene or map anchor.");
            if (input == null) input = anchor.AddComponent<Map10InputController>();
            if (registry == null) registry = anchor.AddComponent<ServerEntityRegistry>();
            input.connection = connection; input.worldCamera = Camera.main;
            registry.connection = connection;
            const string npc54Path = "Assets/Moxiang/Derived/Npc/N054/dead_m2.mxhmodel";
            AssetDatabase.ImportAsset(npc54Path, ImportAssetOptions.ForceUpdate);
            var npc54 = AssetDatabase.LoadAssetAtPath<GameObject>(npc54Path);
            if (npc54 == null) throw new InvalidDataException("Missing audited N054 model and texture derivative.");
            registry.npcPrefab = null;
            registry.npcVisuals = new[] {
                new ServerEntityRegistry.VisualKindPrefab { visualKind = 54, prefab = npc54 }
            };
            uint[] monsterKinds = { 73, 102, 103, 104, 105, 219 };
            registry.monsterPrefab = null;
            registry.animationDriver = anchor.GetComponent<LegacyAnimationDriver>() ?? anchor.AddComponent<LegacyAnimationDriver>();
            registry.monsterVisuals = new ServerEntityRegistry.MonsterVisualPrefab[monsterKinds.Length];
            for (int i = 0; i < monsterKinds.Length; ++i)
            {
                uint kind = monsterKinds[i];
                string path = $"Assets/Moxiang/Derived/Monster/M{kind:000}/l{kind:000}_lod2.mxhmodel";
                AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceUpdate);
                var visual = AssetDatabase.LoadAssetAtPath<GameObject>(path);
                if (visual == null) throw new InvalidDataException("Missing audited monster visual kind " + kind + ".");
                var motions = new ImportedMotion[12];
                for (int motion = 1; motion <= 12; ++motion)
                {
                    string motionPath = $"Assets/Moxiang/Derived/Monster/M{kind:000}/Motions/l{kind:000}_{motion:00}.mxhmotion";
                    motions[motion - 1] = AssetDatabase.LoadAssetAtPath<ImportedMotion>(motionPath);
                    if (motions[motion - 1] == null) throw new InvalidDataException($"Missing monster {kind} motion {motion}.");
                }
                registry.monsterVisuals[i] = new ServerEntityRegistry.MonsterVisualPrefab { visualKind = kind, prefab = visual, motions = motions };
            }
            if (input.worldCamera == null) throw new InvalidDataException("Missing main camera.");
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Derived/Map10/Map10.mxhterrain");
            var field = AssetDatabase.LoadAssetAtPath<ImportedHeightField>("Assets/Moxiang/Derived/Map10/Map10.mxhasset");
            if (prefab == null || field == null) throw new InvalidDataException("Missing Map10 presentation assets.");
            var visuals = connection.mapVisuals;
            if (visuals == null) visuals = new GameObject("MapPresentation").AddComponent<MapVisualController>();
            visuals.movement = input; visuals.entities = registry;
            var map2 = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Derived/Map2/Map2.mxhterrain");
            var field2 = AssetDatabase.LoadAssetAtPath<ImportedHeightField>("Assets/Moxiang/Derived/Map2/Map2.mxhasset");
            if (map2 == null || field2 == null) throw new InvalidDataException("Missing Map2 development presentation assets.");
            visuals.maps = new[] {
                new MapVisualController.Entry { mapNumber = 10, prefab = prefab, heightField = field },
                new MapVisualController.Entry { mapNumber = 2, prefab = map2, heightField = field2 }
            };
            visuals.maps = WuguanMap44Setup.AppendReviewedScene(visuals.maps);
            WuguanMap44Setup.EnsureBuildRegistrationIfPresent();
            if(System.Array.Exists(visuals.maps,x=>x.mapNumber==44))registry.map44TrainingPrefab=WuguanTrainingDummyPrefab.Build();
            connection.mapVisuals = visuals;
            // Imported main assets use their file name; instance names can also be edited.
            // Match the source asset instead of a display name to retire fixed terrain.
            foreach (var root in scene.GetRootGameObjects())
            {
                var source = PrefabUtility.GetCorrespondingObjectFromSource(root);
                if (source == prefab || source == map2)
                {
                    root.SetActive(false);
                    PrefabUtility.RecordPrefabInstancePropertyModifications(root);
                }
            }
            foreach (var renderer in input.GetComponents<Renderer>()) renderer.enabled = false;
            if (input.mapCollider != null) input.mapCollider.enabled = false;
            input.mapCollider = null;
            foreach (var label in UnityEngine.Object.FindObjectsByType<TMPro.TMP_Text>(FindObjectsSortMode.None))
                if (label.text.StartsWith("MAP 10")) label.text = "地图资源验证\n资源未就绪时暂停场景交互；美术质量尚未验收。";
            EditorSceneManager.MarkSceneDirty(scene);
            if (!EditorSceneManager.SaveScene(scene)) throw new IOException("Could not save map presentation bindings.");
            AssetDatabase.SaveAssets();
            Debug.Log("MXH_MAP_VISUAL_BINDINGS_READY maps=10,2 npc_visual_kinds=54 monster_visual_kinds=73,102,103,104,105,219 map2_override=unaccepted");
        }
    }
}
