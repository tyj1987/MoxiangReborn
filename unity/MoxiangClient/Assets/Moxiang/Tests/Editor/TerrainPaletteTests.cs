using System;
using System.IO;
using System.Linq;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;
using Moxiang.Editor;

namespace Moxiang.Tests
{
    public sealed class TerrainPaletteTests
    {
        [Test]
        public void LegacyUvRotationPreservesAllFourCorners()
        {
            Assert.That(TerrainTileMesh.RotateUv(0.25f, 0.75f, 0), Is.EqualTo(new Vector2(0.25f, 0.75f)));
            Assert.That(TerrainTileMesh.RotateUv(0.25f, 0.75f, 1), Is.EqualTo(new Vector2(0.25f, 0.25f)));
            Assert.That(TerrainTileMesh.RotateUv(0.25f, 0.75f, 2), Is.EqualTo(new Vector2(0.75f, 0.25f)));
            Assert.That(TerrainTileMesh.RotateUv(0.25f, 0.75f, 3), Is.EqualTo(new Vector2(0.75f, 0.75f)));
        }

        [Test]
        public void TileGroupsKeepUpwardWindingAndExactEdges()
        {
            var field = ScriptableObject.CreateInstance<ImportedHeightField>();
            field.descriptor = new HeightFieldDescriptor { heightCountX = 3, heightCountZ = 3, tileCountX = 2, tileCountZ = 2,
                facesPerTile = 1, faceSize = 100, width = 200, depth = 200, tiles = new[] { 0, 1 | (1 << 14), 2 << 14, 1 | (3 << 14) } };
            field.heights = new float[9];
            Mesh mesh = null;
            try
            {
                mesh = TerrainTileMesh.Build(field, 0, 0, 2, new[] { 0, 1 });
                Assert.That(mesh.vertexCount, Is.EqualTo(16));
                Assert.That(mesh.GetTriangles(0).Length, Is.EqualTo(12));
                Assert.That(mesh.GetTriangles(1).Length, Is.EqualTo(12));
                Assert.That(mesh.bounds.min.x, Is.EqualTo(-0.1f).Within(0.00001));
                Assert.That(mesh.bounds.max.z, Is.EqualTo(0.1f).Within(0.00001));
                var v = mesh.vertices; var t = mesh.triangles;
                for (int i = 0; i < t.Length; i += 3)
                    Assert.That(Vector3.Cross(v[t[i + 1]] - v[t[i]], v[t[i + 2]] - v[t[i]]).y, Is.GreaterThan(0));
                Assert.Throws<InvalidDataException>(() => TerrainTileMesh.Build(field, 0, 0, 2, new[] { 0 }));
            }
            finally { if (mesh != null) UnityEngine.Object.DestroyImmediate(mesh); UnityEngine.Object.DestroyImmediate(field); }
        }

        private static byte[] PartialDds()
        {
            var bytes = new byte[128 + 2048 + 512 + 128 + 32 + 8];
            void Put(int offset, uint value) => Array.Copy(BitConverter.GetBytes(value), 0, bytes, offset, 4);
            Put(0, 0x20534444); Put(4, 124); Put(12, 64); Put(16, 64); Put(28, 5);
            Put(76, 32); Put(80, 4); Put(84, 0x31545844);
            return bytes;
        }
        [Test]
        public void PartialDdsKeepsDeclaredMipsAndRejectsTruncation()
        {
            byte[] bytes = PartialDds(); var texture = MxhDdsImporter.Read(bytes);
            try { Assert.That(texture.width, Is.EqualTo(64)); Assert.That(texture.mipmapCount, Is.EqualTo(5)); Assert.That(texture.format, Is.EqualTo(TextureFormat.DXT1)); }
            finally { UnityEngine.Object.DestroyImmediate(texture); }
            Array.Resize(ref bytes, bytes.Length - 1);
            Assert.Throws<InvalidDataException>(() => MxhDdsImporter.Read(bytes));
        }
        [Test]
        public void ActualMap10PaletteAndChunkAssetsExist()
        {
            string root = "Assets/Moxiang/Derived/Map10/";
            Assert.That(AssetDatabase.FindAssets("t:Texture2D", new[] { root + "Palette" }).Length, Is.EqualTo(13));
            Assert.That(AssetDatabase.LoadAllAssetsAtPath(root + "Map10.mxhterrain").OfType<Mesh>().Count(), Is.EqualTo(64));
            foreach (string guid in AssetDatabase.FindAssets("t:Texture2D", new[] { root + "Palette" }))
                Assert.That(AssetDatabase.LoadAssetAtPath<Texture2D>(AssetDatabase.GUIDToAssetPath(guid)).mipmapCount, Is.EqualTo(5));
        }

        [Test]
        public void Map2RecoveredTexturesImportButUnresolvedTilePreventsCompleteTerrain()
        {
            const string root = "Assets/Moxiang/Derived/Map2/";
            var textures = AssetDatabase.FindAssets("t:Texture2D", new[] { root + "Palette" });
            Assert.That(textures.Length, Is.EqualTo(19));
            foreach (var guid in textures)
            {
                var texture = AssetDatabase.LoadAssetAtPath<Texture2D>(AssetDatabase.GUIDToAssetPath(guid));
                Assert.That(texture.width, Is.EqualTo(64));
                Assert.That(texture.height, Is.EqualTo(64));
                Assert.That(texture.mipmapCount, Is.EqualTo(5));
            }
            var field = AssetDatabase.LoadAssetAtPath<ImportedHeightField>(root + "Map2.mxhasset");
            Assert.That(field, Is.Not.Null);
            int unresolved = Array.FindIndex(field.descriptor.tiles, t => (t & 0x3fff) == 13);
            Assert.That(unresolved, Is.GreaterThanOrEqualTo(0));
            Assert.That(field.descriptor.textureNames[13], Is.EqualTo("1"));
            var slots = field.descriptor.tiles.Select(t => t & 0x3fff).Distinct().Where(t => t != 13).ToArray();
            int x = unresolved % field.descriptor.tileCountX, z = unresolved / field.descriptor.tileCountX;
            Assert.Throws<InvalidDataException>(() => TerrainTileMesh.Build(field, x, z, 1, slots));
        }

        [Test]
        public void Map2DevelopmentTerrainKeepsOverrideExplicitAndAllChunksPresent()
        {
            const string path = "Assets/Moxiang/Derived/Map2/Map2.mxhterrain";
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            Assert.That(prefab, Is.Not.Null);
            Assert.That(prefab.name, Is.EqualTo("Map2"));
            Assert.That(prefab.transform.childCount, Is.EqualTo(64));
            var materials = AssetDatabase.LoadAllAssetsAtPath(path).OfType<Material>().ToArray();
            Assert.That(materials.Length, Is.EqualTo(20));
            Assert.That(materials.Count(m => m.name.Contains("DevelopmentOverride_Unaccepted")), Is.EqualTo(1));
            foreach (var renderer in prefab.GetComponentsInChildren<MeshRenderer>())
                foreach (var material in renderer.sharedMaterials) Assert.That(material.mainTexture, Is.Not.Null);
        }
    }
}
