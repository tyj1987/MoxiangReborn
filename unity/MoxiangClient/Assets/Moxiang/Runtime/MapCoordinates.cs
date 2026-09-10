using System;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Only the presentation boundary converts legacy world units.</summary>
    public readonly struct MapCoordinates
    {
        public const float SceneScale = 0.001f;
        public readonly float Width;
        public readonly float Depth;

        public MapCoordinates(float width, float depth)
        {
            if (!Finite(width) || !Finite(depth) || width <= 0 || depth <= 0)
                throw new ArgumentOutOfRangeException(nameof(width), "Map extents must be finite and positive.");
            Width = width;
            Depth = depth;
        }

        public Vector3 ToScene(Vector3 gamePosition)
        {
            Validate(gamePosition);
            return new Vector3((gamePosition.x - Width * 0.5f) * SceneScale,
                gamePosition.y * SceneScale, (gamePosition.z - Depth * 0.5f) * SceneScale);
        }

        public Vector3 ToGame(Vector3 scenePosition)
        {
            Validate(scenePosition);
            return new Vector3(scenePosition.x / SceneScale + Width * 0.5f,
                scenePosition.y / SceneScale, scenePosition.z / SceneScale + Depth * 0.5f);
        }

        private static bool Finite(float value) => !float.IsNaN(value) && !float.IsInfinity(value);
        private static void Validate(Vector3 value)
        {
            if (!Finite(value.x) || !Finite(value.y) || !Finite(value.z))
                throw new ArgumentOutOfRangeException(nameof(value), "Position must be finite.");
        }
    }
}
