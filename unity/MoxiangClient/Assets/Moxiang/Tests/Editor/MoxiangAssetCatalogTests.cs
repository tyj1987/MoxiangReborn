using NUnit.Framework;
using UnityEditor;
using Moxiang.Editor;

namespace Moxiang.Tests
{
    public sealed class MoxiangAssetCatalogTests
    {
        [Test]
        public void CurrentCatalogHasNoValidationErrors()
        {
            var report = MoxiangAssetCatalogBuilder.ValidateCurrentCatalog(false);
            Assert.That(report.entries, Is.GreaterThanOrEqualTo(2));
            Assert.That(report.errors, Is.Zero,
                string.Join("\n", report.issues.ConvertAll(
                    issue => issue.assetId + ": " + issue.message)));
        }

        [Test]
        public void GeneratedRegistryContainsCurrentVerticalSliceSeeds()
        {
            var registry = AssetDatabase.LoadAssetAtPath<MoxiangAssetRegistry>(
                MoxiangAssetCatalogBuilder.RegistryPath);
            Assert.That(registry, Is.Not.Null, MoxiangAssetCatalogBuilder.RegistryPath);

            Assert.That(registry.TryGetByAssetId(
                "MX_ENV_WUGUAN_ASSEMBLY_001", out var environment), Is.True);
            Assert.That(environment.Category, Is.EqualTo(MoxiangRuntimeAssetCategory.Environment));
            Assert.That(environment.RuntimePath, Does.EndWith("MX_ENV_WUGUAN_ASSEMBLY_001.prefab"));

            Assert.That(registry.TryGetByAssetId(
                "MX_WPN_SWORD_001", out var sword), Is.True);
            Assert.That(sword.Category, Is.EqualTo(MoxiangRuntimeAssetCategory.Weapon));
            Assert.That(sword.RuntimePath, Does.EndWith("MX_WPN_SWORD_001.prefab"));
        }

        [Test]
        public void UnverifiedLegacyIdsAreNotInvented()
        {
            var registry = AssetDatabase.LoadAssetAtPath<MoxiangAssetRegistry>(
                MoxiangAssetCatalogBuilder.RegistryPath);
            Assert.That(registry, Is.Not.Null);
            Assert.That(registry.TryGetByAssetId(
                "MX_ENV_WUGUAN_ASSEMBLY_001", out var environment), Is.True);
            Assert.That(environment.LegacyId, Is.Empty);
            Assert.That(registry.TryGetByAssetId(
                "MX_WPN_SWORD_001", out var sword), Is.True);
            Assert.That(sword.LegacyId, Is.Empty);
        }
    }
}
