using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;
using Moxiang.Editor;

namespace Moxiang.Tests
{
    public sealed class MoxiangWeaponFamilyV2ProductionTests
    {
        [Serializable]
        private sealed class BlenderReceipt
        {
            public string schema;
            public string asset_id;
            public string category;
            public string source_sha256;
            public string export_file;
            public string export_sha256;
            public string blender_version;
            public float scale_meters;
            public string forward_axis;
            public string up_axis;
            public string[] objects;
        }

        [TestCase(
            MoxiangWeaponV2Setup.BladeAssetId,
            MoxiangWeaponV2Setup.BladeModelPath,
            MoxiangWeaponV2Setup.BladeReceiptPath,
            MoxiangWeaponV2Setup.BladePrefabPath,
            "artsource/Blender/Weapons/Blade/MX_WPN_BLADE_001.blend",
            "MX_SOCKET_FX_PRIMARY")]
        [TestCase(
            MoxiangWeaponV2Setup.SpearAssetId,
            MoxiangWeaponV2Setup.SpearModelPath,
            MoxiangWeaponV2Setup.SpearReceiptPath,
            MoxiangWeaponV2Setup.SpearPrefabPath,
            "artsource/Blender/Weapons/Spear/MX_WPN_SPEAR_001.blend",
            "MX_SOCKET_FX_PRIMARY")]
        public void WeaponFamilyAssetsMeetProductionContract(
            string assetId,
            string modelPath,
            string receiptPath,
            string prefabPath,
            string sourceRepoPath,
            string expectedPrimaryFxSocket)
        {
            ValidateBlenderReceipt(
                assetId,
                modelPath,
                receiptPath,
                sourceRepoPath,
                expectedPrimaryFxSocket);
            ValidateImporter(modelPath);
            ValidatePrefab(prefabPath, expectedPrimaryFxSocket);
        }

        private static void ValidateBlenderReceipt(
            string assetId,
            string modelPath,
            string receiptPath,
            string sourceRepoPath,
            string expectedPrimaryFxSocket)
        {
            var receiptAsset = AssetDatabase.LoadAssetAtPath<TextAsset>(receiptPath);
            Assert.That(receiptAsset, Is.Not.Null, receiptPath);

            var receipt = JsonUtility.FromJson<BlenderReceipt>(receiptAsset.text);
            Assert.That(receipt, Is.Not.Null);
            Assert.That(receipt.schema, Is.EqualTo("moxiang.blender-export-receipt.v1"));
            Assert.That(receipt.asset_id, Is.EqualTo(assetId));
            Assert.That(receipt.category, Is.EqualTo("Weapons"));
            Assert.That(receipt.blender_version, Does.StartWith("5.2."));
            Assert.That(receipt.scale_meters, Is.EqualTo(1f).Within(0.000001f));
            Assert.That(receipt.forward_axis, Is.EqualTo("-Z"));
            Assert.That(receipt.up_axis, Is.EqualTo("Y"));
            Assert.That(receipt.export_file, Is.EqualTo(Path.GetFileName(modelPath)));

            var projectRoot = Directory.GetParent(Application.dataPath).FullName;
            var repoRoot = Path.GetFullPath(Path.Combine(Application.dataPath, "../../.."));
            var model = Path.Combine(
                projectRoot,
                modelPath.Replace('/', Path.DirectorySeparatorChar));
            var source = Path.Combine(
                repoRoot,
                sourceRepoPath.Replace('/', Path.DirectorySeparatorChar));

            Assert.That(File.Exists(model), Is.True, model);
            Assert.That(File.Exists(source), Is.True, source);
            Assert.That(Sha256(model), Is.EqualTo(receipt.export_sha256));
            Assert.That(Sha256(source), Is.EqualTo(receipt.source_sha256));

            foreach (var socket in new[]
            {
                "MX_SOCKET_GRIP",
                "MX_SOCKET_TRAIL_BASE",
                "MX_SOCKET_TRAIL_TIP",
                expectedPrimaryFxSocket
            })
            {
                Assert.That(receipt.objects, Does.Contain(socket), socket);
            }
        }

        private static void ValidateImporter(string modelPath)
        {
            var importer = AssetImporter.GetAtPath(modelPath) as ModelImporter;
            Assert.That(importer, Is.Not.Null, modelPath);
            Assert.That(importer.importCameras, Is.False);
            Assert.That(importer.importLights, Is.False);
            Assert.That(importer.importVisibility, Is.False);
            Assert.That(importer.materialImportMode, Is.EqualTo(ModelImporterMaterialImportMode.None));
            Assert.That(importer.bakeAxisConversion, Is.True);
            Assert.That(importer.isReadable, Is.False);

            long triangles = 0;
            foreach (var mesh in AssetDatabase.LoadAllAssetsAtPath(modelPath).OfType<Mesh>())
            {
                for (var subMesh = 0; subMesh < mesh.subMeshCount; subMesh++)
                    triangles += (long)mesh.GetIndexCount(subMesh) / 3L;
            }

            var budget = MoxiangAssetPolicy.GetBudget(MoxiangAssetCategory.Weapon);
            Assert.That(triangles, Is.GreaterThan(0), modelPath);
            Assert.That(triangles, Is.LessThanOrEqualTo(budget.MaxTriangles), modelPath);
        }

        private static void ValidatePrefab(string prefabPath, string expectedPrimaryFxSocket)
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(prefabPath);
            Assert.That(prefab, Is.Not.Null, prefabPath);

            var rig = prefab.GetComponent<WeaponSocketRig>();
            Assert.That(rig, Is.Not.Null, prefabPath);
            Assert.That(rig.ValidateContract(out var error), Is.True, error);
            Assert.That(rig.Grip.name, Is.EqualTo("MX_SOCKET_GRIP"));
            Assert.That(rig.TrailBase.name, Is.EqualTo("MX_SOCKET_TRAIL_BASE"));
            Assert.That(rig.TrailTip.name, Is.EqualTo("MX_SOCKET_TRAIL_TIP"));
            Assert.That(rig.PrimaryFx.name, Is.EqualTo(expectedPrimaryFxSocket));

            var renderers = prefab.GetComponentsInChildren<Renderer>(true);
            Assert.That(renderers.Length, Is.GreaterThan(0), prefabPath);
            foreach (var renderer in renderers)
            {
                Assert.That(renderer.sharedMaterials.Length, Is.GreaterThan(0), renderer.name);
                foreach (var material in renderer.sharedMaterials)
                {
                    Assert.That(material, Is.Not.Null, renderer.name);
                    Assert.That(
                        material.shader.name,
                        Is.EqualTo("Universal Render Pipeline/Lit"),
                        material.name);
                }
            }
        }

        private static string Sha256(string path)
        {
            using (var stream = File.OpenRead(path))
            using (var sha = SHA256.Create())
                return string.Concat(
                    sha.ComputeHash(stream).Select(value => value.ToString("x2")));
        }
    }
}
