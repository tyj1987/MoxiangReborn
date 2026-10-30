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
    public sealed class MoxiangWeaponV2ProductionTests
    {
        [Serializable]
        private sealed class BlenderReceipt
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

        [Test]
        public void SwordBlenderReceiptBindsSourceAndExportBytes()
        {
            var receiptAsset = AssetDatabase.LoadAssetAtPath<TextAsset>(MoxiangWeaponV2Setup.ReceiptPath);
            Assert.That(receiptAsset, Is.Not.Null, MoxiangWeaponV2Setup.ReceiptPath);
            var receipt = JsonUtility.FromJson<BlenderReceipt>(receiptAsset.text);

            Assert.That(receipt.schema, Is.EqualTo("moxiang.blender-export-receipt.v1"));
            Assert.That(receipt.asset_id, Is.EqualTo(MoxiangWeaponV2Setup.AssetId));
            Assert.That(receipt.category, Is.EqualTo("Weapons"));
            Assert.That(receipt.blender_version, Does.StartWith("5.2."));
            Assert.That(receipt.scale_meters, Is.EqualTo(1f).Within(0.000001f));
            Assert.That(receipt.forward_axis, Is.EqualTo("-Z"));
            Assert.That(receipt.up_axis, Is.EqualTo("Y"));

            var repo = Path.GetFullPath(Path.Combine(Application.dataPath, "../../.."));
            var model = Path.Combine(
                Path.GetDirectoryName(Application.dataPath),
                MoxiangWeaponV2Setup.ModelPath.Replace('/', Path.DirectorySeparatorChar));
            var source = Path.Combine(
                repo,
                "artsource/Blender/Weapons/Sword/MX_WPN_SWORD_001.blend");

            Assert.That(File.Exists(model), Is.True, model);
            Assert.That(File.Exists(source), Is.True, source);
            Assert.That(Sha256(model), Is.EqualTo(receipt.export_sha256));
            Assert.That(Sha256(source), Is.EqualTo(receipt.source_sha256));

            var required = new[]
            {
                "MX_SOCKET_GRIP",
                "MX_SOCKET_TRAIL_BASE",
                "MX_SOCKET_TRAIL_TIP",
                "MX_SOCKET_FX_GUARD"
            };
            foreach (var socket in required)
                Assert.That(receipt.objects, Does.Contain(socket), socket);
        }

        [Test]
        public void SwordImporterMeetsManagedWeaponPolicy()
        {
            var importer = AssetImporter.GetAtPath(MoxiangWeaponV2Setup.ModelPath) as ModelImporter;
            Assert.That(importer, Is.Not.Null);
            Assert.That(importer.importCameras, Is.False);
            Assert.That(importer.importLights, Is.False);
            Assert.That(importer.importVisibility, Is.False);
            Assert.That(importer.materialImportMode, Is.EqualTo(ModelImporterMaterialImportMode.None));
            Assert.That(importer.bakeAxisConversion, Is.True);
            Assert.That(importer.isReadable, Is.False);

            long triangles = 0;
            foreach (var mesh in AssetDatabase.LoadAllAssetsAtPath(MoxiangWeaponV2Setup.ModelPath).OfType<Mesh>())
                for (var subMesh = 0; subMesh < mesh.subMeshCount; subMesh++)
                    triangles += (long)mesh.GetIndexCount(subMesh) / 3L;

            var budget = MoxiangAssetPolicy.GetBudget(MoxiangAssetCategory.Weapon);
            Assert.That(triangles, Is.GreaterThan(0));
            Assert.That(triangles, Is.LessThanOrEqualTo(budget.MaxTriangles));
        }

        [Test]
        public void SwordPrefabPublishesStableRuntimeSocketContract()
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(MoxiangWeaponV2Setup.PrefabPath);
            Assert.That(prefab, Is.Not.Null, MoxiangWeaponV2Setup.PrefabPath);

            var rig = prefab.GetComponent<WeaponSocketRig>();
            Assert.That(rig, Is.Not.Null);
            Assert.That(rig.ValidateContract(out var error), Is.True, error);
            Assert.That(rig.Grip.name, Is.EqualTo("MX_SOCKET_GRIP"));
            Assert.That(rig.TrailBase.name, Is.EqualTo("MX_SOCKET_TRAIL_BASE"));
            Assert.That(rig.TrailTip.name, Is.EqualTo("MX_SOCKET_TRAIL_TIP"));
            Assert.That(rig.PrimaryFx.name, Is.EqualTo("MX_SOCKET_FX_GUARD"));

            var renderers = prefab.GetComponentsInChildren<Renderer>(true);
            Assert.That(renderers.Length, Is.GreaterThan(0));
            foreach (var renderer in renderers)
            {
                Assert.That(renderer.sharedMaterials.Length, Is.GreaterThan(0), renderer.name);
                foreach (var material in renderer.sharedMaterials)
                {
                    Assert.That(material, Is.Not.Null, renderer.name);
                    Assert.That(material.shader.name, Is.EqualTo("Universal Render Pipeline/Lit"), material.name);
                }
            }
        }

        private static string Sha256(string path)
        {
            using (var stream = File.OpenRead(path))
            using (var sha = SHA256.Create())
                return string.Concat(sha.ComputeHash(stream).Select(value => value.ToString("x2")));
        }
    }
}
