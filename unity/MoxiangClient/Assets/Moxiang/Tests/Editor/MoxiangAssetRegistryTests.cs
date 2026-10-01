using System;
using NUnit.Framework;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class MoxiangAssetRegistryTests
    {
        private MoxiangAssetRegistry registry;

        [SetUp]
        public void SetUp()
        {
            registry = ScriptableObject.CreateInstance<MoxiangAssetRegistry>();
        }

        [TearDown]
        public void TearDown()
        {
            UnityEngine.Object.DestroyImmediate(registry);
        }

        [Test]
        public void ResolvesLegacyAndV2IdentityWithoutGameplayPathCoupling()
        {
            registry.ReplaceEntries(new[]
            {
                new MoxiangAssetEntry(
                    "MAP44_WUGUAN_GATE",
                    "MX_ENV_WUGUAN_GATE_001",
                    MoxiangRuntimeAssetCategory.Environment,
                    "Assets/Moxiang/ArtV2/Environment/Wuguan/MX_ENV_WUGUAN_GATE_001.prefab")
            });

            Assert.That(registry.TryGetByLegacyId("map44_wuguan_gate", out var legacy), Is.True);
            Assert.That(legacy.AssetId, Is.EqualTo("MX_ENV_WUGUAN_GATE_001"));
            Assert.That(registry.TryGetByAssetId("mx_env_wuguan_gate_001", out var v2), Is.True);
            Assert.That(v2.RuntimePath, Does.StartWith("Assets/Moxiang/ArtV2/"));
        }

        [Test]
        public void RejectsDuplicateAssetIds()
        {
            var entries = new[]
            {
                new MoxiangAssetEntry("", "MX_TEST_001", MoxiangRuntimeAssetCategory.Environment, "A"),
                new MoxiangAssetEntry("", "MX_TEST_001", MoxiangRuntimeAssetCategory.Environment, "B")
            };
            Assert.Throws<ArgumentException>(() => registry.ReplaceEntries(entries));
        }

        [Test]
        public void RejectsDuplicateNonEmptyLegacyIds()
        {
            var entries = new[]
            {
                new MoxiangAssetEntry("LEGACY_1", "MX_TEST_001", MoxiangRuntimeAssetCategory.Environment, "A"),
                new MoxiangAssetEntry("LEGACY_1", "MX_TEST_002", MoxiangRuntimeAssetCategory.Environment, "B")
            };
            Assert.Throws<ArgumentException>(() => registry.ReplaceEntries(entries));
        }
    }
}
