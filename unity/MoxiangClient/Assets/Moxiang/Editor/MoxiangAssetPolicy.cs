using System;
using System.IO;

namespace Moxiang.Editor
{
    public enum MoxiangAssetCategory
    {
        Unknown,
        Character,
        Npc,
        Monster,
        Weapon,
        Environment,
        Terrain,
        Vegetation,
        Vfx,
        Ui,
        Audio
    }

    public readonly struct MoxiangAssetBudget
    {
        public readonly int MaxTriangles;
        public readonly int MaxMaterials;
        public readonly int MaxTextureSize;

        public MoxiangAssetBudget(int maxTriangles, int maxMaterials, int maxTextureSize)
        {
            MaxTriangles = maxTriangles;
            MaxMaterials = maxMaterials;
            MaxTextureSize = maxTextureSize;
        }
    }

    public static class MoxiangAssetPolicy
    {
        public const string ManagedRoot = "Assets/Moxiang/ArtV2";
        public const string LegacyArtRoot = "Assets/Moxiang/Art";

        public static bool IsManagedAsset(string assetPath)
        {
            if (string.IsNullOrWhiteSpace(assetPath))
                return false;
            var normalized = assetPath.Replace('\\', '/').TrimEnd('/');
            return normalized.Equals(ManagedRoot, StringComparison.OrdinalIgnoreCase) ||
                   normalized.StartsWith(ManagedRoot + "/", StringComparison.OrdinalIgnoreCase);
        }

        public static MoxiangAssetCategory Classify(string assetPath)
        {
            if (!IsManagedAsset(assetPath))
                return MoxiangAssetCategory.Unknown;

            var relative = assetPath.Replace('\\', '/').Substring(ManagedRoot.Length).TrimStart('/');
            var slash = relative.IndexOf('/');
            var head = slash < 0 ? relative : relative.Substring(0, slash);

            if (head.Equals("Characters", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Character;
            if (head.Equals("NPCs", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Npc;
            if (head.Equals("Monsters", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Monster;
            if (head.Equals("Weapons", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Weapon;
            if (head.Equals("Environment", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Environment;
            if (head.Equals("Terrain", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Terrain;
            if (head.Equals("Vegetation", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Vegetation;
            if (head.Equals("VFX", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Vfx;
            if (head.Equals("UI", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Ui;
            if (head.Equals("Audio", StringComparison.OrdinalIgnoreCase)) return MoxiangAssetCategory.Audio;
            return MoxiangAssetCategory.Unknown;
        }

        public static MoxiangAssetBudget GetBudget(MoxiangAssetCategory category)
        {
            switch (category)
            {
                case MoxiangAssetCategory.Character: return new MoxiangAssetBudget(120000, 8, 4096);
                case MoxiangAssetCategory.Npc: return new MoxiangAssetBudget(60000, 6, 2048);
                case MoxiangAssetCategory.Monster: return new MoxiangAssetBudget(100000, 8, 4096);
                case MoxiangAssetCategory.Weapon: return new MoxiangAssetBudget(60000, 4, 4096);
                case MoxiangAssetCategory.Environment: return new MoxiangAssetBudget(100000, 8, 4096);
                case MoxiangAssetCategory.Terrain: return new MoxiangAssetBudget(250000, 8, 4096);
                case MoxiangAssetCategory.Vegetation: return new MoxiangAssetBudget(50000, 4, 2048);
                case MoxiangAssetCategory.Vfx: return new MoxiangAssetBudget(50000, 8, 2048);
                case MoxiangAssetCategory.Ui: return new MoxiangAssetBudget(0, 0, 4096);
                case MoxiangAssetCategory.Audio: return new MoxiangAssetBudget(0, 0, 0);
                default: return new MoxiangAssetBudget(0, 0, 0);
            }
        }

        public static bool IsNormalTexture(string assetPath)
        {
            var stem = Path.GetFileNameWithoutExtension(assetPath) ?? string.Empty;
            return EndsWithAny(stem, "_Normal", "_N", "_NOR");
        }

        public static bool IsLinearDataTexture(string assetPath)
        {
            if (IsNormalTexture(assetPath))
                return true;
            var stem = Path.GetFileNameWithoutExtension(assetPath) ?? string.Empty;
            return EndsWithAny(stem, "_AO", "_Mask", "_Metallic", "_Roughness", "_Smoothness", "_ORM", "_MetallicSmoothness");
        }

        public static bool IsRuntimeDccSource(string assetPath)
        {
            var ext = Path.GetExtension(assetPath);
            return ext.Equals(".blend", StringComparison.OrdinalIgnoreCase) ||
                   ext.Equals(".blend1", StringComparison.OrdinalIgnoreCase) ||
                   ext.Equals(".max", StringComparison.OrdinalIgnoreCase) ||
                   ext.Equals(".mb", StringComparison.OrdinalIgnoreCase) ||
                   ext.Equals(".ma", StringComparison.OrdinalIgnoreCase);
        }

        private static bool EndsWithAny(string value, params string[] suffixes)
        {
            foreach (var suffix in suffixes)
                if (value.EndsWith(suffix, StringComparison.OrdinalIgnoreCase))
                    return true;
            return false;
        }
    }
}
