using System;
using System.Collections.Generic;
using UnityEngine;

namespace Moxiang
{
    public enum MoxiangRuntimeAssetCategory
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

    [Serializable]
    public sealed class MoxiangAssetEntry
    {
        [SerializeField] private string legacyId;
        [SerializeField] private string assetId;
        [SerializeField] private MoxiangRuntimeAssetCategory category;
        [SerializeField] private string runtimePath;

        public string LegacyId => legacyId;
        public string AssetId => assetId;
        public MoxiangRuntimeAssetCategory Category => category;
        public string RuntimePath => runtimePath;

        public MoxiangAssetEntry(
            string legacyId,
            string assetId,
            MoxiangRuntimeAssetCategory category,
            string runtimePath)
        {
            this.legacyId = legacyId ?? string.Empty;
            this.assetId = assetId ?? throw new ArgumentNullException(nameof(assetId));
            this.category = category;
            this.runtimePath = runtimePath ?? throw new ArgumentNullException(nameof(runtimePath));
        }
    }

    [CreateAssetMenu(menuName = "Moxiang/Art V2/Asset Registry", fileName = "MX_AssetRegistry")]
    public sealed class MoxiangAssetRegistry : ScriptableObject
    {
        [SerializeField] private List<MoxiangAssetEntry> entries = new List<MoxiangAssetEntry>();

        public IReadOnlyList<MoxiangAssetEntry> Entries => entries;

        public bool TryGetByAssetId(string assetId, out MoxiangAssetEntry entry)
        {
            return TryFind(assetId, false, out entry);
        }

        public bool TryGetByLegacyId(string legacyId, out MoxiangAssetEntry entry)
        {
            return TryFind(legacyId, true, out entry);
        }

        public void ReplaceEntries(IEnumerable<MoxiangAssetEntry> values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));

            var next = new List<MoxiangAssetEntry>();
            var assetIds = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var legacyIds = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            foreach (var value in values)
            {
                if (value == null)
                    throw new ArgumentException("Registry entries cannot contain null.", nameof(values));
                if (string.IsNullOrWhiteSpace(value.AssetId))
                    throw new ArgumentException("AssetId is required.", nameof(values));
                if (!assetIds.Add(value.AssetId))
                    throw new ArgumentException("Duplicate AssetId: " + value.AssetId, nameof(values));
                if (!string.IsNullOrWhiteSpace(value.LegacyId) && !legacyIds.Add(value.LegacyId))
                    throw new ArgumentException("Duplicate LegacyId: " + value.LegacyId, nameof(values));
                if (string.IsNullOrWhiteSpace(value.RuntimePath))
                    throw new ArgumentException("RuntimePath is required for " + value.AssetId, nameof(values));

                next.Add(value);
            }

            entries = next;
        }

        private bool TryFind(string id, bool legacy, out MoxiangAssetEntry entry)
        {
            if (!string.IsNullOrWhiteSpace(id))
            {
                foreach (var candidate in entries)
                {
                    if (candidate == null)
                        continue;
                    var value = legacy ? candidate.LegacyId : candidate.AssetId;
                    if (string.Equals(value, id, StringComparison.OrdinalIgnoreCase))
                    {
                        entry = candidate;
                        return true;
                    }
                }
            }

            entry = null;
            return false;
        }
    }
}
