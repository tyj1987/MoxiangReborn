using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace Moxiang.Editor
{
    public static class WuguanArtV2Setup
    {
        public const string Root = "Assets/Moxiang/ArtV2/Environment/Wuguan";
        public const string MaterialsRoot = Root + "/Materials";
        public const string TexturesRoot = Root + "/Textures";
        public const string PrefabsRoot = Root + "/Prefabs";
        public const string AssemblyId = "MX_ENV_WUGUAN_ASSEMBLY_001";
        public const string AssemblyPath = PrefabsRoot + "/" + AssemblyId + ".prefab";
        public const string PreviewScenePath = "Assets/Moxiang/Scenes/ArtV2_WuguanPreview.unity";

        private static readonly string[] ModuleIds =
        {
            "MX_ENV_WUGUAN_FLOOR_4M_001",
            "MX_ENV_WUGUAN_POST_6M_001",
            "MX_ENV_WUGUAN_BEAM_4M_001",
            "MX_ENV_WUGUAN_BEAM_8M_001",
            "MX_ENV_WUGUAN_WALL_SOLID_4M_001",
            "MX_ENV_WUGUAN_WALL_DOOR_4M_001",
            "MX_ENV_WUGUAN_LATTICE_2M_001",
            "MX_ENV_WUGUAN_DOUGONG_S_001",
            "MX_ENV_WUGUAN_DAIS_4M_001",
            "MX_ENV_WUGUAN_DUMMY_001",
            "MX_ENV_WUGUAN_LANTERN_001",
            "MX_ENV_WUGUAN_WEAPON_RACK_001",
            "MX_ENV_WUGUAN_BRAZIER_001",
            "MX_ENV_WUGUAN_BENCH_001",
            "MX_ENV_WUGUAN_EAVE_4M_001"
        };

        [Serializable]
        private sealed class BuildReceipt
        {
            public string schema;
            public string generatedUtc;
            public string unityVersion;
            public string assemblyPath;
            public string previewScenePath;
            public int moduleModels;
            public int modulePrefabs;
            public int materials;
            public int assemblyInstances;
        }

        [Serializable]
        private sealed class BoundsAudit
        {
            public string schema;
            public string generatedUtc;
            public Vector3 size;
            public Vector3 min;
            public Vector3 max;
            public string minXRenderer;
            public string maxXRenderer;
            public string minZRenderer;
            public string maxZRenderer;
        }

        [MenuItem("Moxiang/Art Reset V2/Build Wuguan V2")]
        public static void BuildAll()
        {
            EnsureFolder(MaterialsRoot);
            EnsureFolder(PrefabsRoot);

            var materials = BuildMaterials();
            foreach (var id in ModuleIds)
                BuildModulePrefab(id, materials);

            var instanceCount = BuildAssembly();
            BuildPreviewScene();
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);
            WriteReceipt(instanceCount, materials.Count);
            Debug.Log("MXH_WUGUAN_ART_V2_READY modules=" + ModuleIds.Length +
                      " materials=" + materials.Count + " instances=" + instanceCount);
        }

        private static Dictionary<string, Material> BuildMaterials()
        {
            var shader = Shader.Find("Universal Render Pipeline/Lit");
            if (!shader || !shader.isSupported)
                throw new InvalidOperationException("URP Lit shader is unavailable.");

            var materials = new Dictionary<string, Material>(StringComparer.OrdinalIgnoreCase)
            {
                ["Wood"] = UpsertMaterial("MX_MAT_WUGUAN_WOOD_001", shader, Color.white, 0.0f, 1.0f),
                ["Lacquer"] = UpsertMaterial("MX_MAT_WUGUAN_LACQUER_001", shader, Color.white, 0.0f, 1.0f),
                ["Stone"] = UpsertMaterial("MX_MAT_WUGUAN_STONE_001", shader, Color.white, 0.0f, 1.0f),
                ["Plaster"] = UpsertMaterial("MX_MAT_WUGUAN_PLASTER_001", shader, Color.white, 0.0f, 1.0f),
                ["Paper"] = UpsertMaterial("MX_MAT_WUGUAN_PAPER_001", shader, Color.white, 0.0f, 1.0f),
                ["Bronze"] = UpsertMaterial("MX_MAT_WUGUAN_BRONZE_001", shader, Color.white, 1.0f, 1.0f)
            };

            BindPbrTextures(materials["Wood"], "WOOD", 0.75f);
            BindPbrTextures(materials["Lacquer"], "LACQUER", 0.35f);
            BindPbrTextures(materials["Stone"], "STONE", 1.0f);
            BindPbrTextures(materials["Plaster"], "PLASTER", 0.55f);
            BindPbrTextures(materials["Paper"], "PAPER", 0.30f);
            BindPbrTextures(materials["Bronze"], "BRONZE", 0.55f);
            return materials;
        }

        private static Material UpsertMaterial(string name, Shader shader, Color color, float metallic, float smoothness)
        {
            var path = MaterialsRoot + "/" + name + ".mat";
            var mat = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (!mat)
            {
                mat = new Material(shader) { name = name };
                AssetDatabase.CreateAsset(mat, path);
            }
            mat.shader = shader;
            mat.SetColor("_BaseColor", color);
            mat.SetFloat("_Metallic", metallic);
            mat.SetFloat("_Smoothness", smoothness);
            mat.SetFloat("_Surface", 0f);
            mat.SetFloat("_AlphaClip", 0f);
            mat.enableInstancing = true;
            EditorUtility.SetDirty(mat);
            return mat;
        }

        private static void BindPbrTextures(Material material, string key, float normalScale)
        {
            var basePath = TexturesRoot + "/MX_WUGUAN_" + key + "_BaseColor.png";
            var normalPath = TexturesRoot + "/MX_WUGUAN_" + key + "_Normal.png";
            var ormPath = TexturesRoot + "/MX_WUGUAN_" + key + "_ORM.png";
            foreach (var path in new[] { basePath, normalPath, ormPath })
                AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceSynchronousImport | ImportAssetOptions.ForceUpdate);

            var baseMap = AssetDatabase.LoadAssetAtPath<Texture2D>(basePath);
            var normalMap = AssetDatabase.LoadAssetAtPath<Texture2D>(normalPath);
            var ormMap = AssetDatabase.LoadAssetAtPath<Texture2D>(ormPath);
            if (!baseMap || !normalMap || !ormMap)
                throw new InvalidDataException("Missing generated PBR textures for " + key + ".");

            material.SetColor("_BaseColor", Color.white);
            material.SetTexture("_BaseMap", baseMap);
            material.SetTexture("_BumpMap", normalMap);
            material.SetFloat("_BumpScale", normalScale);
            material.SetTexture("_MetallicGlossMap", ormMap);
            material.SetTexture("_OcclusionMap", ormMap);
            material.SetFloat("_Metallic", 1.0f);
            material.SetFloat("_Smoothness", 1.0f);
            material.SetFloat("_OcclusionStrength", 1.0f);
            material.EnableKeyword("_NORMALMAP");
            material.EnableKeyword("_METALLICSPECGLOSSMAP");
            material.EnableKeyword("_OCCLUSIONMAP");
            EditorUtility.SetDirty(material);
        }

        private static Material ChooseMaterial(string objectName, IReadOnlyDictionary<string, Material> materials)
        {
            var upper = (objectName ?? string.Empty).ToUpperInvariant();
            if (upper.Contains("STONE") || upper.Contains("PLINTH") || upper.Contains("FOOT"))
                return materials["Stone"];
            if (upper.Contains("PLASTER"))
                return materials["Plaster"];
            if (upper.Contains("PAPER"))
                return materials["Paper"];
            if (upper.Contains("BRONZE") || upper.Contains("RING") || upper.Contains("PEG") || upper.Contains("BOWL"))
                return materials["Bronze"];
            if (upper.Contains("LACQUER") || upper.Contains("FRAME") || upper.Contains("TRIM"))
                return materials["Lacquer"];
            return materials["Wood"];
        }

        private static void BuildModulePrefab(string id, IReadOnlyDictionary<string, Material> materials)
        {
            var modelPath = Root + "/" + id + ".fbx";
            AssetDatabase.ImportAsset(modelPath, ImportAssetOptions.ForceSynchronousImport | ImportAssetOptions.ForceUpdate);
            var model = AssetDatabase.LoadAssetAtPath<GameObject>(modelPath);
            if (!model)
                throw new InvalidDataException("Missing Art V2 model: " + modelPath);

            var root = new GameObject(id);
            var visual = UnityEngine.Object.Instantiate(model);
            visual.name = id + "_Visual";
            // Preserve the imported FBX root transform. Unity stores the Blender Z-up ->
            // Unity Y-up conversion on this transform even when bakeAxisConversion is enabled.
            // Normalizing it here would re-expose Blender-local axes to runtime placement.
            visual.transform.SetParent(root.transform, false);

            foreach (var renderer in visual.GetComponentsInChildren<Renderer>(true))
            {
                var mat = ChooseMaterial(renderer.gameObject.name, materials);
                var count = Math.Max(1, renderer.sharedMaterials.Length);
                renderer.sharedMaterials = Enumerable.Repeat(mat, count).ToArray();
            }

            var bounds = CalculateBounds(visual);
            var collider = root.AddComponent<BoxCollider>();
            collider.center = root.transform.InverseTransformPoint(bounds.center);
            collider.size = bounds.size;

            var flags = StaticEditorFlags.BatchingStatic |
                        StaticEditorFlags.ContributeGI |
                        StaticEditorFlags.OccludeeStatic |
                        StaticEditorFlags.OccluderStatic;
            foreach (var transform in root.GetComponentsInChildren<Transform>(true))
                GameObjectUtility.SetStaticEditorFlags(transform.gameObject, flags);

            var prefabPath = PrefabsRoot + "/" + id + ".prefab";
            PrefabUtility.SaveAsPrefabAsset(root, prefabPath);
            UnityEngine.Object.DestroyImmediate(root);
        }

        private static Bounds CalculateBounds(GameObject root)
        {
            var renderers = root.GetComponentsInChildren<Renderer>(true);
            if (renderers.Length == 0)
                throw new InvalidDataException("Prefab has no renderers: " + root.name);
            var bounds = renderers[0].bounds;
            for (var i = 1; i < renderers.Length; i++)
                bounds.Encapsulate(renderers[i].bounds);
            return bounds;
        }

        private static int BuildAssembly()
        {
            var root = new GameObject(AssemblyId);
            var count = 0;

            // 16 x 12 m reusable floor field inside the original 18 x 14 m semantic bounds.
            foreach (var x in new[] {-6f,-2f,2f,6f})
                foreach (var z in new[] {-4f,0f,4f})
                    count += Place(root, "MX_ENV_WUGUAN_FLOOR_4M_001", new Vector3(x,0,z), Quaternion.identity);

            foreach (var x in new[] {-8f,-4f,0f,4f,8f})
            {
                foreach (var z in new[] {-6.1f,6.1f})
                {
                    count += Place(root, "MX_ENV_WUGUAN_POST_6M_001", new Vector3(x,0,z), Quaternion.identity);
                    count += Place(root, "MX_ENV_WUGUAN_DOUGONG_S_001", new Vector3(x,5.82f,z), Quaternion.identity);
                }
            }

            foreach (var x in new[] {-6f,-2f,2f,6f})
            {
                count += Place(root, "MX_ENV_WUGUAN_BEAM_4M_001", new Vector3(x,5.7f,-6.1f), Quaternion.identity);
                count += Place(root, "MX_ENV_WUGUAN_BEAM_4M_001", new Vector3(x,5.7f,6.1f), Quaternion.identity);
                count += Place(root, "MX_ENV_WUGUAN_EAVE_4M_001", new Vector3(x,5.95f,-6.25f), Quaternion.identity);
                count += Place(root, "MX_ENV_WUGUAN_EAVE_4M_001", new Vector3(x,5.95f,6.25f), Quaternion.Euler(0,180,0));
            }

            // North wall.
            foreach (var x in new[] {-6f,-2f,2f,6f})
                count += Place(root, "MX_ENV_WUGUAN_WALL_SOLID_4M_001", new Vector3(x,0,6.85f), Quaternion.identity);

            // South facade: central door, solid outer bays and lattice transition bays.
            count += Place(root, "MX_ENV_WUGUAN_WALL_DOOR_4M_001", new Vector3(0,0,-6.85f), Quaternion.identity);
            count += Place(root, "MX_ENV_WUGUAN_WALL_SOLID_4M_001", new Vector3(-6,0,-6.85f), Quaternion.identity);
            count += Place(root, "MX_ENV_WUGUAN_WALL_SOLID_4M_001", new Vector3(6,0,-6.85f), Quaternion.identity);
            count += Place(root, "MX_ENV_WUGUAN_LATTICE_2M_001", new Vector3(-3,2.0f,-6.72f), Quaternion.identity);
            count += Place(root, "MX_ENV_WUGUAN_LATTICE_2M_001", new Vector3(3,2.0f,-6.72f), Quaternion.identity);

            // Side walls.
            foreach (var z in new[] {-4f,0f,4f})
            {
                count += Place(root, "MX_ENV_WUGUAN_WALL_SOLID_4M_001", new Vector3(-8.85f,0,z), Quaternion.Euler(0,90,0));
                count += Place(root, "MX_ENV_WUGUAN_WALL_SOLID_4M_001", new Vector3(8.85f,0,z), Quaternion.Euler(0,90,0));
            }

            count += Place(root, "MX_ENV_WUGUAN_DAIS_4M_001", new Vector3(0,0,4.5f), Quaternion.identity);
            foreach (var x in new[] {-3f,0f,3f})
                count += Place(root, "MX_ENV_WUGUAN_DUMMY_001", new Vector3(x,0,-0.6f), Quaternion.identity);

            count += Place(root, "MX_ENV_WUGUAN_WEAPON_RACK_001", new Vector3(-7.1f,0,2.2f), Quaternion.Euler(0,90,0));
            count += Place(root, "MX_ENV_WUGUAN_WEAPON_RACK_001", new Vector3(7.1f,0,2.2f), Quaternion.Euler(0,-90,0));
            count += Place(root, "MX_ENV_WUGUAN_BENCH_001", new Vector3(-7.1f,0,-3.4f), Quaternion.Euler(0,90,0));
            count += Place(root, "MX_ENV_WUGUAN_BENCH_001", new Vector3(7.1f,0,-3.4f), Quaternion.Euler(0,-90,0));
            count += Place(root, "MX_ENV_WUGUAN_BRAZIER_001", new Vector3(-4.8f,0,4.8f), Quaternion.identity);
            count += Place(root, "MX_ENV_WUGUAN_BRAZIER_001", new Vector3(4.8f,0,4.8f), Quaternion.identity);
            foreach (var x in new[] {-4f,0f,4f})
                count += Place(root, "MX_ENV_WUGUAN_LANTERN_001", new Vector3(x,4.75f,0), Quaternion.identity);

            PrefabUtility.SaveAsPrefabAsset(root, AssemblyPath);
            UnityEngine.Object.DestroyImmediate(root);
            return count;
        }

        private static int Place(GameObject parent, string id, Vector3 position, Quaternion rotation)
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(PrefabsRoot + "/" + id + ".prefab");
            if (!prefab)
                throw new InvalidDataException("Missing module prefab: " + id);
            var instance = (GameObject)PrefabUtility.InstantiatePrefab(prefab);
            instance.transform.SetParent(parent.transform, false);
            instance.transform.localPosition = position;
            instance.transform.localRotation = rotation;
            instance.name = id;
            return 1;
        }

        private static void BuildPreviewScene()
        {
            EnsureFolder("Assets/Moxiang/Scenes");
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            var assembly = AssetDatabase.LoadAssetAtPath<GameObject>(AssemblyPath);
            if (!assembly)
                throw new InvalidDataException("Assembly prefab not found.");

            PrefabUtility.InstantiatePrefab(assembly);

            var sunObject = new GameObject("MX_PREVIEW_SUN");
            var sun = sunObject.AddComponent<Light>();
            sun.type = LightType.Directional;
            sun.intensity = 1.35f;
            sun.shadows = LightShadows.Soft;
            sunObject.transform.rotation = Quaternion.Euler(42f,-32f,0f);

            var warmA = CreatePoint("MX_PREVIEW_WARM_A", new Vector3(-4f,4.1f,1.5f), new Color(1f,0.55f,0.28f), 4.0f, 11f);
            var warmB = CreatePoint("MX_PREVIEW_WARM_B", new Vector3(4f,4.1f,1.5f), new Color(1f,0.55f,0.28f), 4.0f, 11f);
            warmA.transform.SetParent(null);
            warmB.transform.SetParent(null);

            var cameraObject = new GameObject("MX_PREVIEW_CAMERA");
            var camera = cameraObject.AddComponent<Camera>();
            camera.fieldOfView = 52f;
            camera.nearClipPlane = 0.1f;
            camera.farClipPlane = 100f;
            cameraObject.transform.position = new Vector3(0f,9.0f,-20.5f);
            cameraObject.transform.LookAt(new Vector3(0f,2.4f,0f));
            camera.tag = "MainCamera";

            RenderSettings.ambientLight = new Color(0.22f,0.24f,0.28f);
            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, PreviewScenePath);
        }

        private static GameObject CreatePoint(string name, Vector3 position, Color color, float intensity, float range)
        {
            var go = new GameObject(name);
            var light = go.AddComponent<Light>();
            light.type = LightType.Point;
            light.color = color;
            light.intensity = intensity;
            light.range = range;
            light.shadows = LightShadows.Soft;
            go.transform.position = position;
            return go;
        }

        private static void EnsureFolder(string path)
        {
            var normalized = path.Replace('\\','/').TrimEnd('/');
            var parts = normalized.Split('/');
            var current = parts[0];
            for (var i = 1; i < parts.Length; i++)
            {
                var next = current + "/" + parts[i];
                if (!AssetDatabase.IsValidFolder(next))
                    AssetDatabase.CreateFolder(current, parts[i]);
                current = next;
            }
        }

        [MenuItem("Moxiang/Art Reset V2/Capture Wuguan V2 Preview")]
        public static void CapturePreview()
        {
            EditorSceneManager.OpenScene(PreviewScenePath, OpenSceneMode.Single);
            var cameraObject = GameObject.Find("MX_PREVIEW_CAMERA");
            if (!cameraObject)
                throw new InvalidDataException("Preview camera is missing.");
            var camera = cameraObject.GetComponent<Camera>();
            if (!camera)
                throw new InvalidDataException("Preview Camera component is missing.");

            const int width = 1600;
            const int height = 900;
            var target = new RenderTexture(width, height, 24, RenderTextureFormat.ARGB32)
            {
                name = "MX_WUGUAN_PREVIEW_RT",
                antiAliasing = 1
            };
            var previousActive = RenderTexture.active;
            var previousTarget = camera.targetTexture;
            try
            {
                camera.targetTexture = target;
                RenderTexture.active = target;
                camera.Render();
                var texture = new Texture2D(width, height, TextureFormat.RGB24, false);
                texture.ReadPixels(new Rect(0,0,width,height), 0, 0);
                texture.Apply(false, false);

                var repo = Path.GetFullPath(Path.Combine(Application.dataPath, "../../.."));
                var output = Path.Combine(repo, "modern/out/unity-remaster/asset-reset-v2/wuguan-preview.png");
                Directory.CreateDirectory(Path.GetDirectoryName(output));
                File.WriteAllBytes(output, texture.EncodeToPNG());
                UnityEngine.Object.DestroyImmediate(texture);
                Debug.Log("MXH_WUGUAN_PREVIEW_OK " + output);
            }
            finally
            {
                camera.targetTexture = previousTarget;
                RenderTexture.active = previousActive;
                target.Release();
                UnityEngine.Object.DestroyImmediate(target);
            }
        }

        [MenuItem("Moxiang/Art Reset V2/Audit Wuguan V2 Bounds")]
        public static void AuditAssemblyBounds()
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(AssemblyPath);
            if (!prefab)
                throw new InvalidDataException("Assembly prefab not found.");
            var instance = UnityEngine.Object.Instantiate(prefab);
            try
            {
                var renderers = instance.GetComponentsInChildren<Renderer>(true);
                if (renderers.Length == 0)
                    throw new InvalidDataException("Assembly has no renderers.");
                var bounds = renderers[0].bounds;
                var minX = renderers[0]; var maxX = renderers[0];
                var minZ = renderers[0]; var maxZ = renderers[0];
                for (var i = 1; i < renderers.Length; i++)
                {
                    var r = renderers[i];
                    bounds.Encapsulate(r.bounds);
                    if (r.bounds.min.x < minX.bounds.min.x) minX = r;
                    if (r.bounds.max.x > maxX.bounds.max.x) maxX = r;
                    if (r.bounds.min.z < minZ.bounds.min.z) minZ = r;
                    if (r.bounds.max.z > maxZ.bounds.max.z) maxZ = r;
                }
                var audit = new BoundsAudit
                {
                    schema = "moxiang.art-v2.wuguan-bounds-audit.v1",
                    generatedUtc = DateTime.UtcNow.ToString("O"),
                    size = bounds.size,
                    min = bounds.min,
                    max = bounds.max,
                    minXRenderer = HierarchyPath(minX.transform),
                    maxXRenderer = HierarchyPath(maxX.transform),
                    minZRenderer = HierarchyPath(minZ.transform),
                    maxZRenderer = HierarchyPath(maxZ.transform)
                };
                var repo = Path.GetFullPath(Path.Combine(Application.dataPath, "../../.."));
                var output = Path.Combine(repo, "modern/out/unity-remaster/asset-reset-v2/wuguan-bounds-audit.json");
                File.WriteAllText(output, JsonUtility.ToJson(audit, true));
                Debug.Log("MXH_WUGUAN_BOUNDS size=" + bounds.size + " minX=" + audit.minXRenderer + " maxX=" + audit.maxXRenderer);
            }
            finally
            {
                UnityEngine.Object.DestroyImmediate(instance);
            }
        }

        private static string HierarchyPath(Transform transform)
        {
            var names = new List<string>();
            for (var current = transform; current != null; current = current.parent)
                names.Add(current.name);
            names.Reverse();
            return string.Join("/", names);
        }

        private static void WriteReceipt(int instances, int materials)
        {
            var repo = Path.GetFullPath(Path.Combine(Application.dataPath, "../../.."));
            var output = Path.Combine(repo, "modern/out/unity-remaster/asset-reset-v2/wuguan-unity-build.json");
            Directory.CreateDirectory(Path.GetDirectoryName(output));
            var receipt = new BuildReceipt
            {
                schema = "moxiang.art-v2.wuguan-unity-build.v1",
                generatedUtc = DateTime.UtcNow.ToString("O"),
                unityVersion = Application.unityVersion,
                assemblyPath = AssemblyPath,
                previewScenePath = PreviewScenePath,
                moduleModels = ModuleIds.Length,
                modulePrefabs = ModuleIds.Length,
                materials = materials,
                assemblyInstances = instances
            };
            File.WriteAllText(output, JsonUtility.ToJson(receipt, true));
        }
    }
}
