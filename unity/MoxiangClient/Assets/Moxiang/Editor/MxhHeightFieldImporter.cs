using System;
using System.IO;
using System.Security.Cryptography;
using UnityEditor.AssetImporters;
using UnityEngine;
using UnityEngine.Rendering;

namespace Moxiang.Editor
{
    [ScriptedImporter(1, "mxhasset")]
    public sealed class MxhHeightFieldImporter : ScriptedImporter
    {
        public override void OnImportAsset(AssetImportContext context)
        {
            var descriptor = JsonUtility.FromJson<HeightFieldDescriptor>(File.ReadAllText(context.assetPath));
            if (descriptor == null || descriptor.schemaVersion != 1 || descriptor.kind != "heightfield" ||
                descriptor.profileId != "unity-remaster-v1" || descriptor.releaseReady ||
                string.IsNullOrWhiteSpace(descriptor.sourceId) || !IsHash(descriptor.sourceSha256))
                throw new InvalidDataException("Unsupported or incorrectly labelled remaster heightfield.");
            var coordinates = new MapCoordinates(descriptor.width, descriptor.depth);
            long count = (long)descriptor.heightCountX * descriptor.heightCountZ;
            if (descriptor.heightCountX < 2 || descriptor.heightCountZ < 2 || count > 2000000 ||
                float.IsNaN(descriptor.faceSize) || float.IsInfinity(descriptor.faceSize) || descriptor.faceSize <= 0)
                throw new InvalidDataException("Invalid height grid.");
            if (string.IsNullOrEmpty(descriptor.heightFile) ||
                Path.GetFileName(descriptor.heightFile) != descriptor.heightFile ||
                descriptor.heightFile.IndexOfAny(new[] { ':', '\\', '/' }) >= 0)
                throw new InvalidDataException("Height sidecar must be a sibling file.");
            var heightPath = Path.Combine(Path.GetDirectoryName(context.assetPath), descriptor.heightFile);
            context.DependsOnSourceAsset(heightPath);
            var bytes = File.ReadAllBytes(heightPath);
            if (bytes.LongLength != count * 4 || Hash(bytes) != descriptor.heightSha256)
                throw new InvalidDataException("Height sidecar length/SHA256 mismatch.");
            if (descriptor.tiles == null || (long)descriptor.tileCountX * descriptor.tileCountZ != descriptor.tiles.LongLength ||
                descriptor.textureNames == null)
                throw new InvalidDataException("Invalid tile descriptor.");
            foreach (int tile in descriptor.tiles)
                if (tile < 0 || tile > ushort.MaxValue || (tile & 0x3fff) >= descriptor.textureNames.Length)
                    throw new InvalidDataException("Terrain tile references an invalid palette slot.");

            var heights = new float[(int)count];
            var vertices = new Vector3[(int)count];
            var uv = new Vector2[(int)count];
            using (var reader = new BinaryReader(new MemoryStream(bytes)))
                for (int z = 0; z < descriptor.heightCountZ; ++z)
                    for (int x = 0; x < descriptor.heightCountX; ++x)
                    {
                        int index = z * descriptor.heightCountX + x;
                        heights[index] = reader.ReadSingle();
                        vertices[index] = coordinates.ToScene(new Vector3(x * descriptor.faceSize, heights[index], z * descriptor.faceSize));
                        uv[index] = new Vector2((float)x / (descriptor.heightCountX - 1), (float)z / (descriptor.heightCountZ - 1));
                    }
            var indices = new int[checked((descriptor.heightCountX - 1) * (descriptor.heightCountZ - 1) * 6)];
            int cursor = 0;
            for (int z = 0; z < descriptor.heightCountZ - 1; ++z)
                for (int x = 0; x < descriptor.heightCountX - 1; ++x)
                {
                    int a = z * descriptor.heightCountX + x, b = a + descriptor.heightCountX;
                    indices[cursor++] = a; indices[cursor++] = b; indices[cursor++] = a + 1;
                    indices[cursor++] = a + 1; indices[cursor++] = b; indices[cursor++] = b + 1;
                }
            var mesh = new Mesh { name = "HeightFieldInspection", indexFormat = IndexFormat.UInt32 };
            mesh.vertices = vertices; mesh.uv = uv; mesh.triangles = indices;
            mesh.RecalculateNormals(); mesh.RecalculateBounds();
            var asset = ScriptableObject.CreateInstance<ImportedHeightField>();
            asset.name = Path.GetFileNameWithoutExtension(context.assetPath);
            asset.descriptor = descriptor; asset.heights = heights; asset.inspectionMesh = mesh;
            context.AddObjectToAsset("heightfield", asset);
            context.AddObjectToAsset("inspectionMesh", mesh);
            context.SetMainObject(asset);
        }

        private static bool IsHash(string value)
        {
            if (value == null || value.Length != 64) return false;
            foreach (char c in value) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
            return true;
        }
        private static string Hash(byte[] bytes)
        {
            using (var hash = SHA256.Create())
                return BitConverter.ToString(hash.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant();
        }
    }
}
