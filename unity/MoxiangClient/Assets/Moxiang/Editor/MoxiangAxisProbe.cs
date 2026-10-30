using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;

namespace Moxiang.Editor
{
    public static class MoxiangAxisProbe
    {
        private const string Root = "Assets/Moxiang/Tests/Fixtures/AxisProbe";

        [Serializable]
        private sealed class Sample
        {
            public string asset;
            public bool bakeAxisConversion;
            public Vector3 boxSize;
            public Vector3 boxCenter;
            public Vector3 xSize;
            public Vector3 xCenter;
            public Vector3 ySize;
            public Vector3 yCenter;
            public Vector3 zSize;
            public Vector3 zCenter;
            public float score;
        }

        [Serializable]
        private sealed class Report
        {
            public string schema = "moxiang.axis-probe.v1";
            public string generatedUtc;
            public string unityVersion;
            public Sample[] samples;
            public string bestAsset;
            public bool bestBakeAxisConversion;
            public float bestScore;
        }

        [MenuItem("Moxiang/Art Reset V2/Run Axis Probe")]
        public static void Run()
        {
            var names = new[]
            {
                "MX_AXIS_A_NEGZ_Y_NOBK",
                "MX_AXIS_B_NEGZ_Y_BAKE",
                "MX_AXIS_C_NEGY_Z_NOBK",
                "MX_AXIS_D_NEGY_Z_BAKE",
                "MX_AXIS_E_PARENT_WITH_EMPTY",
                "MX_AXIS_F_PARENT_MESH_ONLY"
            };
            var samples = new List<Sample>();
            foreach (var name in names)
            {
                foreach (var bake in new[] { false, true })
                    samples.Add(Measure(name, bake));
            }

            var best = samples.OrderBy(x => x.score).First();
            var report = new Report
            {
                generatedUtc = DateTime.UtcNow.ToString("O"),
                unityVersion = Application.unityVersion,
                samples = samples.ToArray(),
                bestAsset = best.asset,
                bestBakeAxisConversion = best.bakeAxisConversion,
                bestScore = best.score
            };

            var repo = Path.GetFullPath(Path.Combine(Application.dataPath, "../../.."));
            var output = Path.Combine(repo, "modern/out/unity-remaster/asset-reset-v2/axis-probe.json");
            Directory.CreateDirectory(Path.GetDirectoryName(output));
            File.WriteAllText(output, JsonUtility.ToJson(report, true));
            Debug.Log("MXH_AXIS_PROBE_OK best=" + best.asset +
                      " bakeAxisConversion=" + best.bakeAxisConversion +
                      " score=" + best.score);
        }

        private static Sample Measure(string name, bool bakeAxisConversion)
        {
            var path = Root + "/" + name + ".fbx";
            AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceSynchronousImport);
            var importer = AssetImporter.GetAtPath(path) as ModelImporter;
            if (importer == null)
                throw new InvalidDataException("Axis probe importer missing: " + path);
            importer.bakeAxisConversion = bakeAxisConversion;
            importer.importCameras = false;
            importer.importLights = false;
            importer.materialImportMode = ModelImporterMaterialImportMode.None;
            importer.SaveAndReimport();

            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if (!prefab)
                throw new InvalidDataException("Axis probe model missing: " + path);
            var instance = UnityEngine.Object.Instantiate(prefab);
            try
            {
                var box = FindRenderer(instance, "MX_AXIS_PROBE_BOX");
                var x = FindRenderer(instance, "MX_AXIS_PROBE_X");
                var y = FindRenderer(instance, "MX_AXIS_PROBE_Y");
                var z = FindRenderer(instance, "MX_AXIS_PROBE_Z");

                var sample = new Sample
                {
                    asset = name,
                    bakeAxisConversion = bakeAxisConversion,
                    boxSize = box.bounds.size,
                    boxCenter = box.bounds.center,
                    xSize = x.bounds.size,
                    xCenter = x.bounds.center,
                    ySize = y.bounds.size,
                    yCenter = y.bounds.center,
                    zSize = z.bounds.size,
                    zCenter = z.bounds.center
                };

                // Expected Unity convention from Blender source: X -> +X, Z(up) -> +Y,
                // Blender +Y -> Unity +/-Z. Size is sign-independent.
                sample.score =
                    Distance(sample.boxSize, new Vector3(1f, 3f, 2f)) +
                    Distance(sample.xSize, new Vector3(2f, .2f, .15f)) +
                    Distance(sample.ySize, new Vector3(.15f, .3f, 3f)) +
                    Distance(sample.zSize, new Vector3(.2f, 5f, .2f)) +
                    Math.Abs(sample.zCenter.y - 2.5f) +
                    Math.Abs(sample.xCenter.x - 1f);
                return sample;
            }
            finally
            {
                UnityEngine.Object.DestroyImmediate(instance);
            }
        }

        private static Renderer FindRenderer(GameObject root, string exactName)
        {
            var renderer = root.GetComponentsInChildren<Renderer>(true)
                .FirstOrDefault(x => x.gameObject.name == exactName);
            if (!renderer)
                throw new InvalidDataException("Axis probe renderer missing: " + exactName);
            return renderer;
        }

        private static float Distance(Vector3 a, Vector3 b)
        {
            return Math.Abs(a.x-b.x) + Math.Abs(a.y-b.y) + Math.Abs(a.z-b.z);
        }
    }
}
