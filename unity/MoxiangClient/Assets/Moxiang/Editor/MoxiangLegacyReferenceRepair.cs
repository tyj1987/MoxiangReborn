using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.AssetImporters;
using UnityEngine;

namespace Moxiang.Editor
{
    /// <summary>
    /// Repairs importer metadata for retained legacy-derived reference assets.
    /// This never promotes legacy geometry into ArtV2; it only keeps the
    /// semantic/model/motion reference layer loadable for comparison and tests.
    /// </summary>
    public static class MoxiangLegacyReferenceRepair
    {
        private const string ManRoot = "Assets/Moxiang/Derived/Man";

        [Serializable]
        private sealed class ImportState
        {
            public string path;
            public string importer;
            public bool mainObjectLoaded;
        }

        [Serializable]
        private sealed class RepairReport
        {
            public string schema;
            public string generatedUtc;
            public string unityVersion;
            public string root;
            public int assets;
            public int invalid;
            public List<ImportState> states = new List<ImportState>();
        }

        [MenuItem("Moxiang/Legacy Reference/Refresh Man Derived Assets")]
        public static void RefreshManDerivedAssets()
        {
            AssetDatabase.ImportAsset(
                ManRoot,
                ImportAssetOptions.ImportRecursive |
                ImportAssetOptions.ForceUpdate |
                ImportAssetOptions.ForceSynchronousImport);

            AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);

            var report = new RepairReport
            {
                schema = "moxiang.legacy-reference-import-health.v1",
                generatedUtc = DateTime.UtcNow.ToString("O"),
                unityVersion = Application.unityVersion,
                root = ManRoot
            };

            var projectRoot = Directory.GetParent(Application.dataPath)?.FullName;
            if (string.IsNullOrEmpty(projectRoot))
                throw new InvalidOperationException("Unity project root could not be resolved.");

            var absoluteRoot = Path.Combine(
                projectRoot,
                ManRoot.Replace('/', Path.DirectorySeparatorChar));

            var files = Directory.GetFiles(absoluteRoot, "*", SearchOption.AllDirectories)
                .Where(path =>
                    path.EndsWith(".mxhmodel", StringComparison.OrdinalIgnoreCase) ||
                    path.EndsWith(".mxhmotion", StringComparison.OrdinalIgnoreCase) ||
                    path.EndsWith(".mxhdds", StringComparison.OrdinalIgnoreCase))
                .OrderBy(path => path, StringComparer.OrdinalIgnoreCase)
                .ToArray();

            foreach (var absolutePath in files)
            {
                var relative = "Assets" + absolutePath
                    .Substring(Application.dataPath.Length)
                    .Replace('\\', '/');

                var importer = AssetImporter.GetAtPath(relative);
                var main = AssetDatabase.LoadMainAssetAtPath(relative);
                var expected = ExpectedImporter(relative);
                var actual = importer?.GetType().Name ?? string.Empty;
                var valid = importer != null &&
                            string.Equals(actual, expected, StringComparison.Ordinal) &&
                            main != null;

                report.states.Add(new ImportState
                {
                    path = relative,
                    importer = actual,
                    mainObjectLoaded = main != null
                });

                report.assets++;
                if (!valid)
                    report.invalid++;
            }

            var repoRoot = Path.GetFullPath(Path.Combine(Application.dataPath, "../../.."));
            var output = Path.Combine(
                repoRoot,
                "modern/out/unity-remaster/asset-reset-v2/legacy-man-import-health.json");
            Directory.CreateDirectory(Path.GetDirectoryName(output));
            File.WriteAllText(output, JsonUtility.ToJson(report, true));

            if (report.invalid != 0)
                throw new InvalidDataException(
                    "Legacy Man reference import health failed for " +
                    report.invalid + " asset(s). See " + output);

            Debug.Log(
                "MXH_LEGACY_MAN_IMPORT_OK assets=" + report.assets +
                " report=" + output);
        }

        private static string ExpectedImporter(string path)
        {
            if (path.EndsWith(".mxhmodel", StringComparison.OrdinalIgnoreCase))
                return nameof(MxhModelImporter);
            if (path.EndsWith(".mxhmotion", StringComparison.OrdinalIgnoreCase))
                return nameof(MxhMotionImporter);
            if (path.EndsWith(".mxhdds", StringComparison.OrdinalIgnoreCase))
                return nameof(MxhDdsImporter);
            return string.Empty;
        }
    }
}
