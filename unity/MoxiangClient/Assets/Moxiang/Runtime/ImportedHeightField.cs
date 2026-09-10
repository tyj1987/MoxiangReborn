using System;
using UnityEngine;

namespace Moxiang
{
    [Serializable]
    public sealed class HeightFieldDescriptor
    {
        public int schemaVersion;
        public string kind, profileId, converterVersion, sourceId, sourceSha256;
        public string heightFile, heightSha256;
        public bool releaseReady;
        public int heightCountX, heightCountZ, tileCountX, tileCountZ, facesPerTile;
        public float width, depth, faceSize;
        public string[] textureNames;
        public int[] tiles;
    }

    public sealed class ImportedHeightField : ScriptableObject
    {
        public HeightFieldDescriptor descriptor;
        public float[] heights;
        // Geometry inspection only. Material, alpha and collision acceptance are separate.
        public Mesh inspectionMesh;
        public bool IsReleaseReady => false;
    }
}
