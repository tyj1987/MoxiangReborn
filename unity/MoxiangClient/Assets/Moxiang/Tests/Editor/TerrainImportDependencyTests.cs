using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using Moxiang.Editor;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Moxiang.Tests
{
    public sealed class TerrainImportDependencyTests
    {
        private string root;
        private string TerrainPath => root + "/A.mxhterrain";
        private string FieldPath => root + "/Z.mxhasset";
        private string TexturePath => root + "/Palette/Z.mxhdds";
        private string PalettePath => root + "/Palette/palette.json";

        private static string Hash(byte[] bytes)
        {
            using (var sha = SHA256.Create())
                return BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant();
        }

        [SetUp]
        public void WriteFreshSourcesWithoutImportingDependencies()
        {
            root = "Assets/__MxhTerrainImportTest_" + Guid.NewGuid().ToString("N");
            Directory.CreateDirectory(root + "/Palette");
            // Terrain sorts first and is written first. No original game assets are copied.
            File.WriteAllText(TerrainPath, "{\"schemaVersion\":1,\"heightfieldFile\":\"Z.mxhasset\",\"paletteFile\":\"Palette/palette.json\"}");
            var heights = new byte[16];
            File.WriteAllBytes(root + "/height.bytes", heights);
            var field = new HeightFieldDescriptor {
                schemaVersion = 1, kind = "heightfield", profileId = "unity-remaster-v1",
                sourceId = "synthetic-terrain-import-test", sourceSha256 = Hash(heights),
                heightFile = "height.bytes", heightSha256 = Hash(heights),
                heightCountX = 2, heightCountZ = 2, tileCountX = 1, tileCountZ = 1,
                facesPerTile = 1, width = 100, depth = 100, faceSize = 100,
                textureNames = new[] { "synthetic.tga" }, tiles = new[] { 0 }
            };
            File.WriteAllText(FieldPath, JsonUtility.ToJson(field));
            var dds = new byte[136]; // One 4x4 DXT1 block.
            void Put(int offset, uint value) => Array.Copy(BitConverter.GetBytes(value), 0, dds, offset, 4);
            Put(0, 0x20534444); Put(4, 124); Put(12, 4); Put(16, 4); Put(28, 1);
            Put(76, 32); Put(80, 4); Put(84, 0x31545844);
            File.WriteAllBytes(TexturePath, dds);
            File.WriteAllText(PalettePath, "{\"schemaVersion\":1,\"heightfieldSourceId\":\"synthetic-terrain-import-test\"," +
                "\"entries\":[{\"slot\":0,\"file\":\"Z.mxhdds\",\"authoringName\":\"synthetic.tga\",\"sha256\":\"" + Hash(dds) + "\"}]}");
        }

        [TearDown]
        public void RemoveOnlyOwnedFixture()
        {
            if (root == null) return;
            if (!AssetDatabase.DeleteAsset(root) && Directory.Exists(root)) Directory.Delete(root, true);
            if (File.Exists(root + ".meta")) File.Delete(root + ".meta");
        }

        private void AssertTerrain()
        {
            var terrain = AssetDatabase.LoadAssetAtPath<GameObject>(TerrainPath);
            Assert.That(terrain, Is.Not.Null);
            Assert.That(terrain.transform.childCount, Is.EqualTo(1));
            Assert.That(terrain.GetComponentInChildren<MeshFilter>().sharedMesh.vertexCount, Is.EqualTo(4));
            Assert.That(terrain.GetComponentInChildren<MeshRenderer>().sharedMaterial.mainTexture,
                Is.SameAs(AssetDatabase.LoadAssetAtPath<Texture2D>(TexturePath)));
            var dependencies = AssetDatabase.GetDependencies(TerrainPath);
            Assert.That(dependencies, Does.Contain(FieldPath));
            Assert.That(dependencies, Does.Contain(TexturePath));
        }

        [Test]
        public void ColdImportOrdersArtifactsWithoutManualDependencyImport()
        {
            CollectionAssert.AreEquivalent(new[] { FieldPath, TexturePath },
                MxhTerrainImporter.GatherDependenciesFromSourceFile(TerrainPath));
            AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);
            AssertTerrain();
        }

        [Test]
        public void ExplicitRecoveryImportsDependenciesInOnePass()
        {
            MxhTerrainImporter.ReimportWithDependencies(TerrainPath);
            AssertTerrain();
        }

        [TestCase(false)]
        [TestCase(true)]
        public void RecoveryStillRejectsWrongProvenanceOrTextureHash(bool wrongHash)
        {
            string palette = File.ReadAllText(PalettePath);
            palette = wrongHash ? palette.Replace(Hash(File.ReadAllBytes(TexturePath)), new string('0', 64))
                : palette.Replace("synthetic-terrain-import-test", "different-source");
            File.WriteAllText(PalettePath, palette);
            // Import failures log as well as returning DefaultAsset; require failure
            // explicitly instead of relying on Unity's version-specific log wording.
            bool previous = LogAssert.ignoreFailingMessages;
            try
            {
                LogAssert.ignoreFailingMessages = true;
                Assert.Throws<InvalidDataException>(() => MxhTerrainImporter.ReimportWithDependencies(TerrainPath));
                Assert.That(AssetDatabase.LoadAssetAtPath<ImportedHeightField>(FieldPath), Is.Not.Null);
                Assert.That(AssetDatabase.LoadAssetAtPath<Texture2D>(TexturePath), Is.Not.Null);
                Assert.That(AssetDatabase.LoadAssetAtPath<GameObject>(TerrainPath), Is.Null);
            }
            finally { LogAssert.ignoreFailingMessages = previous; }
        }

        [Test]
        public void Map10DeclaresHeightfieldAndAllThirteenTextureArtifacts()
        {
            var dependencies = MxhTerrainImporter.GatherDependenciesFromSourceFile("Assets/Moxiang/Derived/Map10/Map10.mxhterrain");
            Assert.That(dependencies.Length, Is.EqualTo(14));
            Assert.That(dependencies.Count(path => path.EndsWith(".mxhasset")), Is.EqualTo(1));
            Assert.That(dependencies.Count(path => path.EndsWith(".mxhdds")), Is.EqualTo(13));
        }
    }
}
