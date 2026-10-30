using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using UnityEditor;
using UnityEditor.AssetImporters;
using UnityEngine;

namespace Moxiang.Editor
{
    [ScriptedImporter(1, "mxhterrain")]
    public sealed class MxhTerrainImporter : ScriptedImporter
    {
        [Serializable] private sealed class Descriptor { public int schemaVersion; public bool releaseReady; public string heightfieldFile, paletteFile, visualOverrideFile; }
        [Serializable] private sealed class VisualOverride
        {
            public int schemaVersion, slot, baseSlot, tileIndex;
            public bool releaseReady;
            public string classification, sourceSha256, expectedAuthoringName;
        }
        [Serializable] private sealed class Entry { public int slot; public string file, sourceId, sha256, authoringName; }
        [Serializable] private sealed class Palette { public int schemaVersion; public bool releaseReady; public string heightfieldSourceId; public Entry[] entries; }
        private static string Child(string directory, string child)
        {
            if (string.IsNullOrEmpty(child) || Path.IsPathRooted(child) || child.Contains(":") || child.Contains(".."))
                throw new InvalidDataException("Terrain dependency must be a relative child asset.");
            return Path.Combine(directory, child).Replace('\\', '/');
        }
        public override void OnImportAsset(AssetImportContext context)
        {
            var descriptor = JsonUtility.FromJson<Descriptor>(File.ReadAllText(context.assetPath));
            if (descriptor == null || descriptor.schemaVersion != 1 || descriptor.releaseReady)
                throw new InvalidDataException("Unsupported terrain descriptor or premature release label.");
            string directory = Path.GetDirectoryName(context.assetPath);
            string heightPath = Child(directory, descriptor.heightfieldFile), palettePath = Child(directory, descriptor.paletteFile);
            context.DependsOnSourceAsset(heightPath); context.DependsOnSourceAsset(palettePath);
            var field = AssetDatabase.LoadAssetAtPath<ImportedHeightField>(heightPath);
            var palette = JsonUtility.FromJson<Palette>(File.ReadAllText(palettePath));
            if (field == null || palette == null || palette.schemaVersion != 1 || palette.releaseReady || palette.heightfieldSourceId != field.descriptor.sourceId)
                throw new InvalidDataException("Terrain and palette provenance do not match.");
            int overriddenSlot = -1;
            if (!string.IsNullOrEmpty(descriptor.visualOverrideFile))
            {
                string overridePath = Child(directory, descriptor.visualOverrideFile);
                context.DependsOnSourceAsset(overridePath);
                var art = JsonUtility.FromJson<VisualOverride>(File.ReadAllText(overridePath));
                if (art == null || art.schemaVersion != 1 || art.releaseReady || art.classification != "development-placeholder" ||
                    art.sourceSha256 != field.descriptor.sourceSha256 || art.slot < 0 || art.slot >= field.descriptor.textureNames.Length ||
                    art.expectedAuthoringName != field.descriptor.textureNames[art.slot] || art.tileIndex < 0 ||
                    art.tileIndex >= field.descriptor.tiles.Length || (field.descriptor.tiles[art.tileIndex] & 0x3fff) != art.slot ||
                    field.descriptor.tiles.Count(t => (t & 0x3fff) == art.slot) != 1 || palette.entries.Any(e => e.slot == art.slot))
                    throw new InvalidDataException("Visual override must identify one unresolved source tile and stay development-only.");
                var basis = palette.entries.SingleOrDefault(e => e.slot == art.baseSlot);
                if (basis == null) throw new InvalidDataException("Visual override base material is missing.");
                palette.entries = palette.entries.Concat(new[] { new Entry { slot = art.slot, authoringName = art.expectedAuthoringName,
                    file = basis.file, sourceId = basis.sourceId, sha256 = basis.sha256 } }).ToArray();
                overriddenSlot = art.slot;
            }
            int[] slots = palette.entries.Select(e => e.slot).ToArray();
            if (!field.descriptor.tiles.Select(t => t & 0x3fff).Distinct().OrderBy(t => t).SequenceEqual(slots.OrderBy(t => t)))
                throw new InvalidDataException("Palette must exactly cover every used slot.");
            var materials = new Material[slots.Length];
            for (int i = 0; i < slots.Length; ++i)
            {
                var entry = palette.entries[i];
                if (entry.slot < 0 || entry.slot >= field.descriptor.textureNames.Length || entry.authoringName != field.descriptor.textureNames[entry.slot])
                    throw new InvalidDataException("Invalid source texture mapping.");
                string path = Child(Path.GetDirectoryName(palettePath), entry.file);
                context.DependsOnSourceAsset(path);
                using (var sha = SHA256.Create())
                    if (BitConverter.ToString(sha.ComputeHash(File.ReadAllBytes(path))).Replace("-", "").ToLowerInvariant() != entry.sha256)
                        throw new InvalidDataException("Terrain texture source hash mismatch.");
                var texture = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
                if (texture == null) throw new InvalidDataException("Import the source DDS before building terrain.");
                var material = new Material(Shader.Find("Universal Render Pipeline/Lit")) { name = "TerrainSlot" + slots[i] };
                if (slots[i] == overriddenSlot) material.name += "_DevelopmentOverride_Unaccepted";
                material.SetTexture("_BaseMap", texture); material.SetColor("_BaseColor", Color.white); material.SetFloat("_Smoothness", 0);
                context.AddObjectToAsset(material.name, material); materials[i] = material;
            }
            var root = new GameObject(Path.GetFileNameWithoutExtension(context.assetPath) + "TexturedTerrain");
            for (int z = 0; z < field.descriptor.tileCountZ; z += 32)
                for (int x = 0; x < field.descriptor.tileCountX; x += 32)
                {
                    var mesh = TerrainTileMesh.Build(field, x, z, 32, slots); context.AddObjectToAsset(mesh.name, mesh);
                    var chunk = new GameObject(mesh.name, typeof(MeshFilter), typeof(MeshRenderer));
                    chunk.transform.SetParent(root.transform, false); chunk.GetComponent<MeshFilter>().sharedMesh = mesh;
                    chunk.GetComponent<MeshRenderer>().sharedMaterials = materials;
                }
            context.AddObjectToAsset("terrain", root); context.SetMainObject(root);
        }
    }
}
