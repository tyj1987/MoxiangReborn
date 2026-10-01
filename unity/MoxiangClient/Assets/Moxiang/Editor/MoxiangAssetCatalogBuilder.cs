using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using UnityEditor;
using UnityEditor.Build;
using UnityEngine;

namespace Moxiang.Editor
{
    public static class MoxiangAssetCatalogBuilder
    {
        public const string SourceRepoPath = "artsource/Concept/asset-catalog-v1.json";
        public const string RegistryPath = "Assets/Moxiang/Data/MX_AssetRegistry.asset";
        private const string ExpectedSchema = "moxiang.art-v2.asset-catalog.v1";

        private static readonly Regex AssetIdPattern =
            new Regex("^MX_[A-Z0-9][A-Z0-9_]{2,95}$", RegexOptions.CultureInvariant);

        private static readonly HashSet<string> AllowedStages =
            new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                "prototype",
                "source_ready",
                "export_ready",
                "integration_ready",
                "production_ready"
            };

        [Serializable]
        private sealed class CatalogFile
        {
            public string schema;
            public CatalogEntry[] entries;
        }

        [Serializable]
        private sealed class CatalogEntry
        {
            public string assetId;
            public string legacyId;
            public string category;
            public string runtimePath;
            public string sourcePath;
            public string stage;
            public bool verticalSlice;
            public bool releaseReady;
        }

        [Serializable]
        public sealed class ValidationIssue
        {
            public string severity;
            public string assetId;
            public string message;
        }

        [Serializable]
        public sealed class ValidationReport
        {
            public string schema;
            public string generatedUtc;
            public string unityVersion;
            public string sourcePath;
            public string registryPath;
            public int entries;
            public int errors;
            public int warnings;
            public bool passed;
            public List<ValidationIssue> issues = new List<ValidationIssue>();
        }

        [MenuItem("Moxiang/Art Reset V2/Build Asset Registry")]
        public static void BuildRegistry()
        {
            var catalog = LoadCatalog();
            var report = Validate(catalog, true);
            if (!report.passed)
                throw new BuildFailedException(
                    "Art V2 asset catalog validation failed with " + report.errors + " error(s).");

            EnsureFolder("Assets/Moxiang/Data");
            var registry = AssetDatabase.LoadAssetAtPath<MoxiangAssetRegistry>(RegistryPath);
            if (!registry)
            {
                registry = ScriptableObject.CreateInstance<MoxiangAssetRegistry>();
                AssetDatabase.CreateAsset(registry, RegistryPath);
            }

            var entries = new List<MoxiangAssetEntry>();
            foreach (var item in catalog.entries)
            {
                if (!TryParseRuntimeCategory(item.category, out var category))
                    throw new BuildFailedException("Unsupported catalog category: " + item.category);

                entries.Add(new MoxiangAssetEntry(
                    item.legacyId ?? string.Empty,
                    item.assetId,
                    category,
                    item.runtimePath));
            }

            registry.ReplaceEntries(entries);
            EditorUtility.SetDirty(registry);
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);
            Debug.Log("MXH_ART_V2_REGISTRY_OK entries=" + entries.Count + " path=" + RegistryPath);
        }

        public static ValidationReport ValidateCurrentCatalog(bool writeReport)
        {
            return Validate(LoadCatalog(), writeReport);
        }

        private static CatalogFile LoadCatalog()
        {
            var path = AbsoluteSourcePath();
            if (!File.Exists(path))
                throw new FileNotFoundException("Art V2 asset catalog is missing.", path);

            var catalog = JsonUtility.FromJson<CatalogFile>(File.ReadAllText(path));
            if (catalog == null)
                throw new InvalidDataException("Art V2 asset catalog JSON could not be parsed.");
            return catalog;
        }

        private static ValidationReport Validate(CatalogFile catalog, bool writeReport)
        {
            var report = new ValidationReport
            {
                schema = "moxiang.art-v2.asset-catalog-validation.v1",
                generatedUtc = DateTime.UtcNow.ToString("O"),
                unityVersion = Application.unityVersion,
                sourcePath = SourceRepoPath,
                registryPath = RegistryPath,
                entries = catalog.entries?.Length ?? 0
            };

            if (!string.Equals(catalog.schema, ExpectedSchema, StringComparison.Ordinal))
                Add(report, "error", string.Empty, "Unexpected catalog schema: " + catalog.schema);

            if (catalog.entries == null || catalog.entries.Length == 0)
            {
                Add(report, "error", string.Empty, "Catalog must contain at least one entry.");
                return Finish(report, writeReport);
            }

            var assetIds = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var legacyIds = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            foreach (var item in catalog.entries)
            {
                if (item == null)
                {
                    Add(report, "error", string.Empty, "Catalog contains a null entry.");
                    continue;
                }

                var id = item.assetId ?? string.Empty;
                if (!AssetIdPattern.IsMatch(id))
                    Add(report, "error", id, "AssetId does not match the MX_ production naming contract.");
                else if (!assetIds.Add(id))
                    Add(report, "error", id, "Duplicate AssetId.");

                if (!string.IsNullOrWhiteSpace(item.legacyId) && !legacyIds.Add(item.legacyId))
                    Add(report, "error", id, "Duplicate non-empty LegacyId: " + item.legacyId);

                if (!TryParseRuntimeCategory(item.category, out var runtimeCategory))
                {
                    Add(report, "error", id, "Unsupported runtime category: " + item.category);
                }
                else
                {
                    var editorCategory = MoxiangAssetPolicy.Classify(item.runtimePath);
                    if (!CategoryMatches(runtimeCategory, editorCategory))
                        Add(report, "error", id, "Runtime path category does not match catalog category.");
                }

                if (string.IsNullOrWhiteSpace(item.runtimePath) ||
                    !MoxiangAssetPolicy.IsManagedAsset(item.runtimePath))
                {
                    Add(report, "error", id, "RuntimePath must be inside Assets/Moxiang/ArtV2.");
                }
                else if (!AssetDatabase.LoadMainAssetAtPath(item.runtimePath))
                {
                    Add(report, "error", id, "Runtime asset does not exist: " + item.runtimePath);
                }

                if (string.IsNullOrWhiteSpace(item.sourcePath))
                {
                    Add(report, "error", id, "SourcePath is required.");
                }
                else
                {
                    var absoluteSource = Path.Combine(
                        RepoRoot(),
                        item.sourcePath.Replace('/', Path.DirectorySeparatorChar));
                    if (!File.Exists(absoluteSource))
                        Add(report, "error", id, "Source asset does not exist: " + item.sourcePath);
                }

                if (string.IsNullOrWhiteSpace(item.stage) || !AllowedStages.Contains(item.stage))
                    Add(report, "error", id, "Unsupported production stage: " + item.stage);

                if (item.releaseReady &&
                    !string.Equals(item.stage, "production_ready", StringComparison.OrdinalIgnoreCase))
                {
                    Add(report, "error", id, "releaseReady requires production_ready stage.");
                }

                if (string.IsNullOrWhiteSpace(item.legacyId))
                    Add(report, "warning", id, "LegacyId is not mapped yet.");
            }

            return Finish(report, writeReport);
        }

        private static bool TryParseRuntimeCategory(
            string value,
            out MoxiangRuntimeAssetCategory category)
        {
            return Enum.TryParse(value, true, out category) &&
                   category != MoxiangRuntimeAssetCategory.Unknown;
        }

        private static bool CategoryMatches(
            MoxiangRuntimeAssetCategory runtimeCategory,
            MoxiangAssetCategory editorCategory)
        {
            switch (runtimeCategory)
            {
                case MoxiangRuntimeAssetCategory.Character:
                    return editorCategory == MoxiangAssetCategory.Character;
                case MoxiangRuntimeAssetCategory.Npc:
                    return editorCategory == MoxiangAssetCategory.Npc;
                case MoxiangRuntimeAssetCategory.Monster:
                    return editorCategory == MoxiangAssetCategory.Monster;
                case MoxiangRuntimeAssetCategory.Weapon:
                    return editorCategory == MoxiangAssetCategory.Weapon;
                case MoxiangRuntimeAssetCategory.Environment:
                    return editorCategory == MoxiangAssetCategory.Environment;
                case MoxiangRuntimeAssetCategory.Terrain:
                    return editorCategory == MoxiangAssetCategory.Terrain;
                case MoxiangRuntimeAssetCategory.Vegetation:
                    return editorCategory == MoxiangAssetCategory.Vegetation;
                case MoxiangRuntimeAssetCategory.Vfx:
                    return editorCategory == MoxiangAssetCategory.Vfx;
                case MoxiangRuntimeAssetCategory.Ui:
                    return editorCategory == MoxiangAssetCategory.Ui;
                case MoxiangRuntimeAssetCategory.Audio:
                    return editorCategory == MoxiangAssetCategory.Audio;
                default:
                    return false;
            }
        }

        private static ValidationReport Finish(ValidationReport report, bool writeReport)
        {
            report.passed = report.errors == 0;
            if (writeReport)
            {
                var output = Path.Combine(
                    RepoRoot(),
                    "modern/out/unity-remaster/asset-reset-v2/asset-catalog-validation.json");
                Directory.CreateDirectory(Path.GetDirectoryName(output));
                File.WriteAllText(output, JsonUtility.ToJson(report, true));
            }
            return report;
        }

        private static void Add(
            ValidationReport report,
            string severity,
            string assetId,
            string message)
        {
            report.issues.Add(new ValidationIssue
            {
                severity = severity,
                assetId = assetId,
                message = message
            });
            if (severity == "error")
                report.errors++;
            else
                report.warnings++;
        }

        private static string AbsoluteSourcePath()
        {
            return Path.Combine(
                RepoRoot(),
                SourceRepoPath.Replace('/', Path.DirectorySeparatorChar));
        }

        private static string RepoRoot()
        {
            return Path.GetFullPath(Path.Combine(Application.dataPath, "../../.."));
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
