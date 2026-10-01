using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace Moxiang.Editor
{
    public static class MoxiangAssetValidator
    {
        [Serializable]
        public sealed class ValidationIssue
        {
            public string severity;
            public string path;
            public string message;
        }

        [Serializable]
        public sealed class ValidationReport
        {
            public string generatedUtc;
            public string unityVersion;
            public string managedRoot;
            public bool rootExists;
            public bool passed;
            public int assetsScanned;
            public int errors;
            public int warnings;
            public List<ValidationIssue> issues = new List<ValidationIssue>();
        }

        [Serializable]
        private sealed class BlenderExportReceipt
        {
            public string schema;
            public string asset_id;
            public string category;
            public string source_file;
            public string source_sha256;
            public string export_file;
            public string export_sha256;
            public string blender_version;
            public float scale_meters;
            public string forward_axis;
            public string up_axis;
            public string[] objects;
        }

        public static string ReportPath => Path.GetFullPath(Path.Combine(
            Application.dataPath, "../../../modern/out/unity-remaster/asset-reset-v2/asset-validation.json"));

        [MenuItem("Moxiang/Art Reset V2/Validate Managed Assets")]
        public static void RunMenu()
        {
            var report = ValidateAll(true);
            if (!report.passed)
                throw new BuildFailedException("Asset Reset V2 validation failed. See " + ReportPath);
            Debug.Log("MXH_ART_V2_VALIDATION_OK assets=" + report.assetsScanned + " warnings=" + report.warnings);
        }

        public static ValidationReport ValidateAll(bool writeReport)
        {
            var report = new ValidationReport
            {
                generatedUtc = DateTime.UtcNow.ToString("O"),
                unityVersion = Application.unityVersion,
                managedRoot = MoxiangAssetPolicy.ManagedRoot,
                rootExists = AssetDatabase.IsValidFolder(MoxiangAssetPolicy.ManagedRoot)
            };

            if (!report.rootExists)
            {
                Add(report, "error", MoxiangAssetPolicy.ManagedRoot, "Managed Art V2 root is missing.");
                return Finish(report, writeReport);
            }

            foreach (var guid in AssetDatabase.FindAssets(string.Empty, new[] { MoxiangAssetPolicy.ManagedRoot }))
            {
                var path = AssetDatabase.GUIDToAssetPath(guid);
                if (string.IsNullOrEmpty(path) || AssetDatabase.IsValidFolder(path))
                    continue;

                report.assetsScanned++;
                ValidatePath(report, path);
            }

            return Finish(report, writeReport);
        }

        public static void ValidateOrThrow()
        {
            var report = ValidateAll(true);
            if (!report.passed)
                throw new BuildFailedException("Asset Reset V2 validation failed with " + report.errors +
                                               " error(s). Report: " + ReportPath);
        }

        private static void ValidatePath(ValidationReport report, string path)
        {
            var category = MoxiangAssetPolicy.Classify(path);
            if (category == MoxiangAssetCategory.Unknown)
            {
                Add(report, "error", path, "Asset is inside ArtV2 but not inside an approved category directory.");
                return;
            }

            if (MoxiangAssetPolicy.IsRuntimeDccSource(path))
                Add(report, "error", path, "Editable DCC source must stay under artsource/, not inside Unity Assets.");

            var fileName = Path.GetFileName(path);
            if (fileName.Contains(" "))
                Add(report, "warning", path, "New runtime asset names should not contain spaces.");
            if (!fileName.StartsWith("MX_", StringComparison.OrdinalIgnoreCase) &&
                !fileName.EndsWith(".meta", StringComparison.OrdinalIgnoreCase))
                Add(report, "warning", path, "Primary Asset V2 names should use the MX_ prefix.");

            var extension = Path.GetExtension(path).ToLowerInvariant();
            if (extension == ".fbx" || extension == ".obj" || extension == ".dae")
                ValidateModel(report, path, category);
            else if (extension == ".png" || extension == ".tga" || extension == ".jpg" ||
                     extension == ".jpeg" || extension == ".psd" || extension == ".exr")
                ValidateTexture(report, path, category);
            else if (extension == ".json" &&
                     path.EndsWith(".fbx.receipt.json", StringComparison.OrdinalIgnoreCase))
                ValidateExportReceipt(report, path, category);
        }

        private static void ValidateModel(ValidationReport report, string path, MoxiangAssetCategory category)
        {
            var budget = MoxiangAssetPolicy.GetBudget(category);
            long triangles = 0;
            foreach (var mesh in AssetDatabase.LoadAllAssetsAtPath(path).OfType<Mesh>())
                for (var subMesh = 0; subMesh < mesh.subMeshCount; subMesh++)
                    triangles += (long)mesh.GetIndexCount(subMesh) / 3L;

            if (budget.MaxTriangles > 0 && triangles > budget.MaxTriangles)
                Add(report, "error", path, "Triangle budget exceeded: " + triangles + " > " + budget.MaxTriangles + ".");

            var root = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if (root != null && budget.MaxMaterials > 0)
            {
                var materials = new HashSet<Material>();
                foreach (var renderer in root.GetComponentsInChildren<Renderer>(true))
                    foreach (var material in renderer.sharedMaterials)
                        if (material != null)
                            materials.Add(material);

                if (materials.Count > budget.MaxMaterials)
                    Add(report, "error", path, "Material budget exceeded: " + materials.Count + " > " + budget.MaxMaterials + ".");
            }

            var importer = AssetImporter.GetAtPath(path) as ModelImporter;
            if (importer == null)
                return;

            if (importer.importCameras || importer.importLights)
                Add(report, "error", path, "Production model imports must not import cameras or lights.");
            if (importer.materialImportMode != ModelImporterMaterialImportMode.None)
                Add(report, "error", path, "FBX embedded material import must remain disabled for Art V2.");
            if (!importer.bakeAxisConversion)
                Add(report, "error", path, "Art V2 FBX imports must bake axis conversion so Blender Z-up becomes Unity Y-up without a hidden root rotation.");
            if (importer.isReadable)
                Add(report, "warning", path, "Mesh Read/Write is enabled; disable unless a reviewed runtime system requires it.");
        }

        private static void ValidateTexture(ValidationReport report, string path, MoxiangAssetCategory category)
        {
            var importer = AssetImporter.GetAtPath(path) as TextureImporter;
            if (importer == null)
                return;

            var budget = MoxiangAssetPolicy.GetBudget(category);
            if (budget.MaxTextureSize > 0 && importer.maxTextureSize > budget.MaxTextureSize)
                Add(report, "error", path, "Texture max size exceeds category budget.");

            if (category == MoxiangAssetCategory.Ui && importer.mipmapEnabled)
                Add(report, "error", path, "UI textures must not use mipmaps by default.");

            if (MoxiangAssetPolicy.IsNormalTexture(path))
            {
                if (importer.textureType != TextureImporterType.NormalMap)
                    Add(report, "error", path, "Normal suffix requires TextureImporterType.NormalMap.");
                if (importer.sRGBTexture)
                    Add(report, "error", path, "Normal maps must be linear.");
            }
            else if (MoxiangAssetPolicy.IsLinearDataTexture(path) && importer.sRGBTexture)
            {
                Add(report, "error", path, "Mask/metallic/AO/roughness/smoothness data must be linear.");
            }
        }

        private static void ValidateExportReceipt(
            ValidationReport report,
            string receiptPath,
            MoxiangAssetCategory category)
        {
            var text = AssetDatabase.LoadAssetAtPath<TextAsset>(receiptPath);
            if (!text)
            {
                Add(report, "error", receiptPath, "Blender export receipt could not be loaded as text.");
                return;
            }

            BlenderExportReceipt receipt;
            try
            {
                receipt = JsonUtility.FromJson<BlenderExportReceipt>(text.text);
            }
            catch (Exception exception)
            {
                Add(report, "error", receiptPath, "Blender export receipt JSON is invalid: " + exception.Message);
                return;
            }

            if (receipt == null || receipt.schema != "moxiang.blender-export-receipt.v1")
            {
                Add(report, "error", receiptPath, "Unexpected Blender export receipt schema.");
                return;
            }

            var modelPath = receiptPath.Substring(
                0,
                receiptPath.Length - ".receipt.json".Length);
            var expectedAssetId = Path.GetFileNameWithoutExtension(modelPath);

            if (!string.Equals(receipt.asset_id, expectedAssetId, StringComparison.Ordinal))
                Add(report, "error", receiptPath, "Receipt AssetId does not match the FBX file name.");
            if (!string.Equals(receipt.export_file, Path.GetFileName(modelPath), StringComparison.Ordinal))
                Add(report, "error", receiptPath, "Receipt export_file does not match the FBX file name.");
            if (!ReceiptCategoryMatches(category, receipt.category))
                Add(report, "error", receiptPath, "Receipt category does not match the managed asset path.");
            if (Math.Abs(receipt.scale_meters - 1.0f) > 0.000001f)
                Add(report, "error", receiptPath, "Receipt scale must remain 1 meter per Blender unit.");
            if (!string.Equals(receipt.forward_axis, "-Z", StringComparison.Ordinal) ||
                !string.Equals(receipt.up_axis, "Y", StringComparison.Ordinal))
                Add(report, "error", receiptPath, "Receipt axis contract must be -Z forward / Y up.");
            if (receipt.objects == null || receipt.objects.Length == 0)
                Add(report, "error", receiptPath, "Receipt must list exported objects.");
            if (string.IsNullOrWhiteSpace(receipt.source_file) ||
                string.IsNullOrWhiteSpace(receipt.source_sha256))
                Add(report, "error", receiptPath, "Receipt must bind to a Blender source file hash.");
            if (string.IsNullOrWhiteSpace(receipt.blender_version))
                Add(report, "error", receiptPath, "Receipt must record the Blender version.");

            var projectRoot = Directory.GetParent(Application.dataPath)?.FullName;
            if (string.IsNullOrEmpty(projectRoot))
            {
                Add(report, "error", receiptPath, "Unity project root could not be resolved.");
                return;
            }

            var absoluteModelPath = Path.GetFullPath(Path.Combine(projectRoot, modelPath));
            if (!File.Exists(absoluteModelPath))
            {
                Add(report, "error", receiptPath, "Receipt references a missing FBX: " + modelPath);
                return;
            }

            var actualHash = Sha256(absoluteModelPath);
            if (!string.Equals(actualHash, receipt.export_sha256, StringComparison.OrdinalIgnoreCase))
                Add(report, "error", receiptPath, "FBX SHA256 no longer matches its Blender export receipt.");
        }

        private static bool ReceiptCategoryMatches(
            MoxiangAssetCategory category,
            string receiptCategory)
        {
            switch (category)
            {
                case MoxiangAssetCategory.Character:
                    return string.Equals(receiptCategory, "Characters", StringComparison.OrdinalIgnoreCase);
                case MoxiangAssetCategory.Npc:
                    return string.Equals(receiptCategory, "NPCs", StringComparison.OrdinalIgnoreCase);
                case MoxiangAssetCategory.Monster:
                    return string.Equals(receiptCategory, "Monsters", StringComparison.OrdinalIgnoreCase);
                case MoxiangAssetCategory.Weapon:
                    return string.Equals(receiptCategory, "Weapons", StringComparison.OrdinalIgnoreCase);
                case MoxiangAssetCategory.Environment:
                    return string.Equals(receiptCategory, "Environment", StringComparison.OrdinalIgnoreCase);
                case MoxiangAssetCategory.Terrain:
                    return string.Equals(receiptCategory, "Terrain", StringComparison.OrdinalIgnoreCase);
                case MoxiangAssetCategory.Vegetation:
                    return string.Equals(receiptCategory, "Vegetation", StringComparison.OrdinalIgnoreCase);
                case MoxiangAssetCategory.Vfx:
                    return string.Equals(receiptCategory, "VFX", StringComparison.OrdinalIgnoreCase);
                case MoxiangAssetCategory.Ui:
                    return string.Equals(receiptCategory, "UI", StringComparison.OrdinalIgnoreCase);
                case MoxiangAssetCategory.Audio:
                    return string.Equals(receiptCategory, "Audio", StringComparison.OrdinalIgnoreCase);
                default:
                    return false;
            }
        }

        private static string Sha256(string path)
        {
            using (var stream = File.OpenRead(path))
            using (var sha = SHA256.Create())
                return string.Concat(sha.ComputeHash(stream).Select(value => value.ToString("x2")));
        }

        private static ValidationReport Finish(ValidationReport report, bool writeReport)
        {
            report.passed = report.errors == 0;
            if (writeReport)
            {
                Directory.CreateDirectory(Path.GetDirectoryName(ReportPath));
                File.WriteAllText(ReportPath, JsonUtility.ToJson(report, true));
            }
            return report;
        }

        private static void Add(ValidationReport report, string severity, string path, string message)
        {
            report.issues.Add(new ValidationIssue { severity = severity, path = path, message = message });
            if (severity == "error") report.errors++;
            else report.warnings++;
        }
    }

    public sealed class ArtResetV2BuildGuard : IPreprocessBuildWithReport
    {
        public int callbackOrder => -900;

        public void OnPreprocessBuild(BuildReport report)
        {
            MoxiangAssetValidator.ValidateOrThrow();
        }
    }
}
