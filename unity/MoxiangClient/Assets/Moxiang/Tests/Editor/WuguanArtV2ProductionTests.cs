using System.Collections.Generic;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;
using Moxiang.Editor;

namespace Moxiang.Tests
{
    public sealed class WuguanArtV2ProductionTests
    {
        private static readonly string[] Modules =
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

        [Test]
        public void SharedMaterialLibraryIsPresentAndUrpLit()
        {
            var names = new[]
            {
                "MX_MAT_WUGUAN_WOOD_001",
                "MX_MAT_WUGUAN_LACQUER_001",
                "MX_MAT_WUGUAN_STONE_001",
                "MX_MAT_WUGUAN_PLASTER_001",
                "MX_MAT_WUGUAN_PAPER_001",
                "MX_MAT_WUGUAN_BRONZE_001"
            };

            foreach (var name in names)
            {
                var path = WuguanArtV2Setup.MaterialsRoot + "/" + name + ".mat";
                var material = AssetDatabase.LoadAssetAtPath<Material>(path);
                Assert.That(material, Is.Not.Null, path);
                Assert.That(material.shader, Is.Not.Null, path);
                Assert.That(material.shader.name, Is.EqualTo("Universal Render Pipeline/Lit"), path);
                Assert.That(material.enableInstancing, Is.True, path);
                Assert.That(material.GetTexture("_BaseMap"), Is.Not.Null, path + " base map");
                Assert.That(material.GetTexture("_BumpMap"), Is.Not.Null, path + " normal map");
                Assert.That(material.GetTexture("_MetallicGlossMap"), Is.Not.Null, path + " metallic/smoothness map");
                Assert.That(material.GetTexture("_OcclusionMap"), Is.Not.Null, path + " occlusion map");
                Assert.That(material.IsKeywordEnabled("_NORMALMAP"), Is.True, path + " normal keyword");
            }
        }

        [Test]
        public void AllModuleModelsAndPrefabsAreProductionReady()
        {
            foreach (var id in Modules)
            {
                var modelPath = WuguanArtV2Setup.Root + "/" + id + ".fbx";
                var prefabPath = WuguanArtV2Setup.PrefabsRoot + "/" + id + ".prefab";
                var model = AssetDatabase.LoadAssetAtPath<GameObject>(modelPath);
                var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(prefabPath);
                Assert.That(model, Is.Not.Null, modelPath);
                Assert.That(prefab, Is.Not.Null, prefabPath);
                Assert.That(prefab.GetComponent<BoxCollider>(), Is.Not.Null, id + " collider");

                var renderers = prefab.GetComponentsInChildren<Renderer>(true);
                Assert.That(renderers.Length, Is.GreaterThan(0), id + " renderers");
                foreach (var renderer in renderers)
                {
                    Assert.That(renderer.sharedMaterials.Length, Is.GreaterThan(0), renderer.name);
                    foreach (var material in renderer.sharedMaterials)
                        Assert.That(material, Is.Not.Null, renderer.name + " material");
                }

                var importer = AssetImporter.GetAtPath(modelPath) as ModelImporter;
                Assert.That(importer, Is.Not.Null, modelPath);
                Assert.That(importer.importCameras, Is.False, modelPath);
                Assert.That(importer.importLights, Is.False, modelPath);
                Assert.That(importer.materialImportMode, Is.EqualTo(ModelImporterMaterialImportMode.None), modelPath);
                Assert.That(importer.bakeAxisConversion, Is.True, modelPath + " axis conversion");
                Assert.That(importer.isReadable, Is.False, modelPath);
            }
        }

        [Test]
        public void AssemblyUsesExpectedModulesAndPreservesSemanticScale()
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(WuguanArtV2Setup.AssemblyPath);
            Assert.That(prefab, Is.Not.Null);
            Assert.That(prefab.transform.childCount, Is.EqualTo(76));

            var instance = Object.Instantiate(prefab);
            try
            {
                var renderers = instance.GetComponentsInChildren<Renderer>(true);
                Assert.That(renderers.Length, Is.GreaterThan(100));

                var bounds = renderers[0].bounds;
                for (var i = 1; i < renderers.Length; i++)
                    bounds.Encapsulate(renderers[i].bounds);

                Assert.That(bounds.size.x, Is.InRange(17.0f, 20.5f), "width");
                Assert.That(bounds.size.z, Is.InRange(13.0f, 16.5f), "depth");
                Assert.That(bounds.size.y, Is.InRange(5.5f, 7.5f), "height");
            }
            finally
            {
                Object.DestroyImmediate(instance);
            }
        }

        [Test]
        public void PreviewSceneAndAssemblyAssetExist()
        {
            Assert.That(AssetDatabase.LoadAssetAtPath<SceneAsset>(WuguanArtV2Setup.PreviewScenePath), Is.Not.Null);
            Assert.That(AssetDatabase.LoadAssetAtPath<GameObject>(WuguanArtV2Setup.AssemblyPath), Is.Not.Null);
        }

        [Test]
        public void ManagedTreeStillHasZeroValidationErrors()
        {
            var report = MoxiangAssetValidator.ValidateAll(false);
            Assert.That(report.errors, Is.Zero,
                string.Join("\n", report.issues.ConvertAll(issue => issue.path + ": " + issue.message)));
        }
    }
}
