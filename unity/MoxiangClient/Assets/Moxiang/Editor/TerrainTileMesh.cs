using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace Moxiang.Editor
{
    public static class TerrainTileMesh
    {
        // Matches TerrainScene::rotatedUv and HFL's high two rotation bits.
        public static Vector2 RotateUv(float u, float v, int rotation)
        {
            switch (rotation & 3)
            {
                case 1: return new Vector2(1 - v, u);
                case 2: return new Vector2(1 - u, 1 - v);
                case 3: return new Vector2(v, 1 - u);
                default: return new Vector2(u, v);
            }
        }

        public static Mesh Build(ImportedHeightField field, int startX, int startZ, int count, int[] paletteSlots)
        {
            var d = field.descriptor;
            if (d.facesPerTile <= 0 || d.facesPerTile > 16 || count < 1 || count > 32 ||
                startX < 0 || startZ < 0 || startX >= d.tileCountX || startZ >= d.tileCountZ ||
                d.heightCountX != d.tileCountX * d.facesPerTile + 1 ||
                d.heightCountZ != d.tileCountZ * d.facesPerTile + 1 ||
                field.heights.Length != d.heightCountX * d.heightCountZ)
                throw new InvalidDataException("Tile and height grid do not describe the same terrain.");
            var groups = new List<int>[paletteSlots.Length];
            var slots = new Dictionary<int, int>();
            for (int i = 0; i < groups.Length; ++i) { groups[i] = new List<int>(); slots.Add(paletteSlots[i], i); }
            var vertices = new List<Vector3>(); var normals = new List<Vector3>(); var uv = new List<Vector2>();
            var coordinates = new MapCoordinates(d.width, d.depth);
            float Height(int x, int z) => field.heights[Mathf.Clamp(z, 0, d.heightCountZ - 1) * d.heightCountX + Mathf.Clamp(x, 0, d.heightCountX - 1)];
            for (int tz = startZ; tz < Math.Min(startZ + count, d.tileCountZ); ++tz)
                for (int tx = startX; tx < Math.Min(startX + count, d.tileCountX); ++tx)
                {
                    int tile = d.tiles[tz * d.tileCountX + tx];
                    if (!slots.TryGetValue(tile & 0x3fff, out int group)) throw new InvalidDataException("Unresolved used palette slot.");
                    for (int z = 0; z < d.facesPerTile; ++z)
                        for (int x = 0; x < d.facesPerTile; ++x)
                        {
                            int hx = tx * d.facesPerTile + x, hz = tz * d.facesPerTile + z, first = vertices.Count;
                            var corners = new[] { new Vector2Int(hx, hz), new Vector2Int(hx + 1, hz), new Vector2Int(hx + 1, hz + 1), new Vector2Int(hx, hz + 1) };
                            foreach (var c in corners)
                            {
                                vertices.Add(coordinates.ToScene(new Vector3(c.x * d.faceSize, Height(c.x, c.y), c.y * d.faceSize)));
                                normals.Add(new Vector3(Height(c.x - 1, c.y) - Height(c.x + 1, c.y), 2 * d.faceSize, Height(c.x, c.y - 1) - Height(c.x, c.y + 1)).normalized);
                                uv.Add(RotateUv((float)(c.x - tx * d.facesPerTile) / d.facesPerTile, (float)(c.y - tz * d.facesPerTile) / d.facesPerTile, tile >> 14));
                            }
                            groups[group].AddRange(new[] { first, first + 2, first + 1, first, first + 3, first + 2 });
                        }
                }
            var mesh = new Mesh { name = $"Terrain_{startX}_{startZ}", indexFormat = UnityEngine.Rendering.IndexFormat.UInt32 };
            mesh.SetVertices(vertices); mesh.SetNormals(normals); mesh.SetUVs(0, uv); mesh.subMeshCount = groups.Length;
            for (int i = 0; i < groups.Length; ++i) mesh.SetTriangles(groups[i], i);
            mesh.RecalculateBounds(); return mesh;
        }
    }
}
