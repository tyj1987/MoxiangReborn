using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;

namespace Moxiang.Editor
{
    public static class MoxiangWeaponV2Setup
    {
        // Backwards-compatible Sword 001 constants used by the existing production tests.
        public const string AssetId = "MX_WPN_SWORD_001";
        public const string Root = "Assets/Moxiang/ArtV2/Weapons/Sword";
        public const string ModelPath = Root + "/" + AssetId + ".fbx";
        public const string ReceiptPath = ModelPath + ".receipt.json";
        public const string MaterialsRoot = Root + "/Materials";
        public const string PrefabsRoot = Root + "/Prefabs";
        public const string PrefabPath = PrefabsRoot + "/" + AssetId + ".prefab";

        public const string BladeAssetId = "MX_WPN_BLADE_001";
        public const string BladeRoot = "Assets/Moxiang/ArtV2/Weapons/Blade";
        public const string BladeModelPath = BladeRoot + "/" + BladeAssetId + ".fbx";
        public const string BladeReceiptPath = BladeModelPath + ".receipt.json";
        public const string BladePrefabPath = BladeRoot + "/Prefabs/" + BladeAssetId + ".prefab";

        public const string SpearAssetId = "MX_WPN_SPEAR_001";
        public const string SpearRoot = "Assets/Moxiang/ArtV2/Weapons/Spear";
        public const string SpearModelPath = SpearRoot + "/" + SpearAssetId + ".fbx";
        public const string SpearReceiptPath = SpearModelPath + ".receipt.json";
        public const string SpearPrefabPath = SpearRoot + "/Prefabs/" + SpearAssetId + ".prefab";

        private const string GripSocket = "MX_SOCKET_GRIP";
        private const string TrailBaseSocket = "MX_SOCKET_TRAIL_BASE";
        private const string TrailTipSocket = "MX_SOCKET_TRAIL_TIP";

        private sealed class WeaponSpec
        {
            public readonly string AssetId;
            public readonly string Family;
            public readonly string Root;
            public readonly string PrimaryFxSocket;

            public WeaponSpec(string assetId, string family, string root, string primaryFxSocket)
            {
                AssetId = assetId;
                Family = family;
                Root = root;
                PrimaryFxSocket = primaryFxSocket;
            }

            public string ModelPath => Root + "/" + AssetId + ".fbx";
            public string ReceiptPath => ModelPath + ".receipt.json";
            public string MaterialsRoot => Root + "/Materials";
            public string PrefabsRoot => Root + "/Prefabs";
            public string PrefabPath => PrefabsRoot + "/" + AssetId + ".prefab";
        }

        [Serializable]
        private sealed class BuildReceipt
        {
            public string schema;
            public string generatedUtc;
            public string unityVersion;
            public string assetId;
            public string family;
            public string modelPath;
            public string prefabPath;
            public int renderers;
            public long triangles;
            public string[] sockets;
        }

        private readonly struct MaterialDefinition
        {
            public readonly Color Color;
            public readonly float Metallic;
            public readonly float Smoothness;

            public MaterialDefinition(Color color, float metallic, float smoothness)
            {
                Color = color;
                Metallic = metallic;
                Smoothness = smoothness;
            }
        }

        private static readonly WeaponSpec SwordSpec =
            new WeaponSpec(AssetId, "SWORD", Root, "MX_SOCKET_FX_GUARD");

        private static readonly WeaponSpec BladeSpec =
            new WeaponSpec(BladeAssetId, "BLADE", BladeRoot, "MX_SOCKET_FX_PRIMARY");

        private static readonly WeaponSpec SpearSpec =
            new WeaponSpec(SpearAssetId, "SPEAR", SpearRoot, "MX_SOCKET_FX_PRIMARY");

        [MenuItem("Moxiang/Art Reset V2/Weapons/Build All Vertical Slice Weapons")]
        public static void BuildAllVerticalSliceWeapons()
        {
            BuildWeapon(SwordSpec);
            BuildWeapon(BladeSpec);
            BuildWeapon(SpearSpec);
            Debug.Log("MXH_WPN_VERTICAL_SLICE_READY count=3");
        }

        [MenuItem("Moxiang/Art Reset V2/Weapons/Build Sword 001")]
        public static void BuildSword001()
        {
            BuildWeapon(SwordSpec);
        }

        [MenuItem("Moxiang/Art Reset V2/Weapons/Build Blade 001")]
        public static void BuildBlade001()
        {
            BuildWeapon(BladeSpec);
        }

        [MenuItem("Moxiang/Art Reset V2/Weapons/Build Spear 001")]
        public static void BuildSpear001()
        {
            BuildWeapon(SpearSpec);
        }

        private static void BuildWeapon(WeaponSpec spec)
        {
            AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);
            EnsureFolder(spec.MaterialsRoot);
            EnsureFolder(spec.PrefabsRoot);

            AssetDatabase.ImportAsset(
                spec.ModelPath,
                ImportAssetOptions.ForceSynchronousImport | ImportAssetOptions.ForceUpdate);

            var model = AssetDatabase.LoadAssetAtPath<GameObject>(spec.ModelPath);
            if (!model)
                throw new InvalidDataException("Weapon model is missing: " + spec.ModelPath);

            var shader = Shader.Find("Universal Render Pipeline/Lit");
            if (!shader || !shader.isSupported)
                throw new InvalidOperationException("URP Lit shader is unavailable.");

            var materialCache = new Dictionary<string, Material>(StringComparer.OrdinalIgnoreCase);
            var root = new GameObject(spec.AssetId);
            try
            {
                var visual = UnityEngine.Object.Instantiate(model);
                visual.name = spec.AssetId + "_Visual";
                visual.transform.SetParent(root.transform, false);

                foreach (var renderer in visual.GetComponentsInChildren<Renderer>(true))
                {
                    var role = ChooseMaterialRole(renderer.gameObject.name);
                    var material = GetOrCreateMaterial(spec, role, shader, materialCache);
                    var count = Math.Max(1, renderer.sharedMaterials.Length);
                    renderer.sharedMaterials = Enumerable.Repeat(material, count).ToArray();
                }

                var grip = FindRequired(root.transform, GripSocket);
                var trailBase = FindRequired(root.transform, TrailBaseSocket);
                var trailTip = FindRequired(root.transform, TrailTipSocket);
                var primaryFx = FindRequired(root.transform, spec.PrimaryFxSocket);

                var rig = root.AddComponent<WeaponSocketRig>();
                rig.Configure(grip, trailBase, trailTip, primaryFx);
                if (!rig.ValidateContract(out var contractError))
                    throw new InvalidDataException(contractError);

                var prefab = PrefabUtility.SaveAsPrefabAsset(root, spec.PrefabPath);
                if (!prefab)
                    throw new InvalidOperationException("Failed to save weapon prefab: " + spec.PrefabPath);

                AssetDatabase.SaveAssets();
                AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);
                WriteBuildReceipt(spec, prefab);
                Debug.Log("MXH_WPN_READY asset=" + spec.AssetId + " prefab=" + spec.PrefabPath);
            }
            finally
            {
                UnityEngine.Object.DestroyImmediate(root);
            }
        }

        private static string ChooseMaterialRole(string objectName)
        {
            var upper = (objectName ?? string.Empty).ToUpperInvariant();
            if (upper.Contains("TASSEL"))
                return "ACCENT";
            if (upper.Contains("SHAFT"))
                return "WOOD";
            if (upper.Contains("GRIP_CORE"))
                return "LEATHER";
            if (upper.Contains("GRIP_WRAP") ||
                upper.Contains("WRAP") ||
                upper.Contains("RICASSO") ||
                upper.Contains("SPINE"))
                return "BLACKENED";
            if (upper.Contains("GUARD") ||
                upper.Contains("POMMEL") ||
                upper.Contains("COLLAR"))
                return "BRONZE";
            return "STEEL";
        }

        private static Material GetOrCreateMaterial(
            WeaponSpec spec,
            string role,
            Shader shader,
            IDictionary<string, Material> cache)
        {
            if (cache.TryGetValue(role, out var material))
                return material;

            var definition = GetMaterialDefinition(role);
            var name = "MX_MAT_" + spec.Family + "_" + role + "_001";
            var path = spec.MaterialsRoot + "/" + name + ".mat";
            material = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (!material)
            {
                material = new Material(shader) { name = name };
                AssetDatabase.CreateAsset(material, path);
            }

            material.shader = shader;
            material.SetColor("_BaseColor", definition.Color);
            material.SetFloat("_Metallic", definition.Metallic);
            material.SetFloat("_Smoothness", definition.Smoothness);
            material.enableInstancing = true;
            EditorUtility.SetDirty(material);
            cache[role] = material;
            return material;
        }

        private static MaterialDefinition GetMaterialDefinition(string role)
        {
            switch (role)
            {
                case "BRONZE":
                    return new MaterialDefinition(
                        new Color(0.32f, 0.16f, 0.045f), 0.82f, 0.70f);
                case "LEATHER":
                    return new MaterialDefinition(
                        new Color(0.075f, 0.025f, 0.016f), 0.0f, 0.38f);
                case "BLACKENED":
                    return new MaterialDefinition(
                        new Color(0.025f, 0.028f, 0.03f), 0.72f, 0.68f);
                case "WOOD":
                    return new MaterialDefinition(
                        new Color(0.10f, 0.032f, 0.015f), 0.0f, 0.46f);
                case "ACCENT":
                    return new MaterialDefinition(
                        new Color(0.36f, 0.012f, 0.008f), 0.0f, 0.40f);
                default:
                    return new MaterialDefinition(
                        new Color(0.28f, 0.32f, 0.36f), 0.95f, 0.82f);
            }
        }

        private static Transform FindRequired(Transform root, string name)
        {
            foreach (var candidate in root.GetComponentsInChildren<Transform>(true))
                if (string.Equals(candidate.name, name, StringComparison.Ordinal))
                    return candidate;
            throw new InvalidDataException("Required weapon socket is missing: " + name);
        }

        private static void WriteBuildReceipt(WeaponSpec spec, GameObject prefab)
        {
            var meshes = prefab.GetComponentsInChildren<MeshFilter>(true)
                .Select(item => item.sharedMesh)
                .Where(item => item)
                .Distinct()
                .ToArray();

            long triangles = 0;
            foreach (var mesh in meshes)
                for (var subMesh = 0; subMesh < mesh.subMeshCount; subMesh++)
                    triangles += (long)mesh.GetIndexCount(subMesh) / 3L;

            var repo = Path.GetFullPath(Path.Combine(Application.dataPath, "../../.."));
            var shortName = spec.AssetId
                .Replace("MX_WPN_", string.Empty)
                .ToLowerInvariant()
                .Replace('_', '-');
            var output = Path.Combine(
                repo,
                "modern/out/unity-remaster/asset-reset-v2/" + shortName + "-unity-build.json");
            Directory.CreateDirectory(Path.GetDirectoryName(output));

            var receipt = new BuildReceipt
            {
                schema = "moxiang.art-v2.weapon-unity-build.v1",
                generatedUtc = DateTime.UtcNow.ToString("O"),
                unityVersion = Application.unityVersion,
                assetId = spec.AssetId,
                family = spec.Family,
                modelPath = spec.ModelPath,
                prefabPath = spec.PrefabPath,
                renderers = prefab.GetComponentsInChildren<Renderer>(true).Length,
                triangles = triangles,
                sockets = new[]
                {
                    GripSocket,
                    TrailBaseSocket,
                    TrailTipSocket,
                    spec.PrimaryFxSocket
                }
            };
            File.WriteAllText(output, JsonUtility.ToJson(receipt, true));
        }

        private static void EnsureFolder(string path)
        {
            var normalized = path.Replace('\\', '/').TrimEnd('/');
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
    }
}
