using NUnit.Framework;
using UnityEditor;
using Moxiang.Editor;

namespace Moxiang.Tests
{
    public sealed class MoxiangAssetPipelineTests
    {
        [TestCase("Assets/Moxiang/ArtV2/Characters/MX_Hero.fbx", MoxiangAssetCategory.Character)]
        [TestCase("Assets/Moxiang/ArtV2/NPCs/MX_Merchant.fbx", MoxiangAssetCategory.Npc)]
        [TestCase("Assets/Moxiang/ArtV2/Monsters/MX_Wolf.fbx", MoxiangAssetCategory.Monster)]
        [TestCase("Assets/Moxiang/ArtV2/Weapons/MX_Sword.fbx", MoxiangAssetCategory.Weapon)]
        [TestCase("Assets/Moxiang/ArtV2/Environment/MX_Wall.fbx", MoxiangAssetCategory.Environment)]
        [TestCase("Assets/Moxiang/ArtV2/Terrain/MX_MapTile.fbx", MoxiangAssetCategory.Terrain)]
        [TestCase("Assets/Moxiang/ArtV2/Vegetation/MX_Pine.fbx", MoxiangAssetCategory.Vegetation)]
        [TestCase("Assets/Moxiang/ArtV2/VFX/MX_SwordTrail.png", MoxiangAssetCategory.Vfx)]
        [TestCase("Assets/Moxiang/ArtV2/UI/MX_Inventory.png", MoxiangAssetCategory.Ui)]
        [TestCase("Assets/Moxiang/ArtV2/Audio/MX_SwordHit.wav", MoxiangAssetCategory.Audio)]
        public void ManagedPathClassificationIsDeterministic(string path, MoxiangAssetCategory expected)
        {
            Assert.That(MoxiangAssetPolicy.IsManagedAsset(path), Is.True);
            Assert.That(MoxiangAssetPolicy.Classify(path), Is.EqualTo(expected));
        }

        [Test]
        public void LegacyArtIsNotSilentlyReprocessedByV2()
        {
            const string legacy = "Assets/Moxiang/Art/WuguanTrainingHall/Source/WuguanTrainingHall_PBR.fbx";
            Assert.That(MoxiangAssetPolicy.IsManagedAsset(legacy), Is.False);
            Assert.That(MoxiangAssetPolicy.Classify(legacy), Is.EqualTo(MoxiangAssetCategory.Unknown));
        }

        [Test]
        public void RuntimeDccSourceIsRejectedByPolicy()
        {
            Assert.That(MoxiangAssetPolicy.IsRuntimeDccSource("Assets/Moxiang/ArtV2/Characters/MX_Hero.blend"), Is.True);
            Assert.That(MoxiangAssetPolicy.IsRuntimeDccSource("Assets/Moxiang/ArtV2/Characters/MX_Hero.fbx"), Is.False);
        }

        [Test]
        public void TextureRoleRulesSeparateColorAndLinearData()
        {
            Assert.That(MoxiangAssetPolicy.IsNormalTexture("MX_Sword_Normal.png"), Is.True);
            Assert.That(MoxiangAssetPolicy.IsLinearDataTexture("MX_Sword_Normal.png"), Is.True);
            Assert.That(MoxiangAssetPolicy.IsLinearDataTexture("MX_Sword_MetallicSmoothness.png"), Is.True);
            Assert.That(MoxiangAssetPolicy.IsLinearDataTexture("MX_Sword_AO.png"), Is.True);
            Assert.That(MoxiangAssetPolicy.IsLinearDataTexture("MX_Sword_BaseColor.png"), Is.False);
        }

        [Test]
        public void PhaseZeroBudgetsRemainPinned()
        {
            var character = MoxiangAssetPolicy.GetBudget(MoxiangAssetCategory.Character);
            var npc = MoxiangAssetPolicy.GetBudget(MoxiangAssetCategory.Npc);
            var terrain = MoxiangAssetPolicy.GetBudget(MoxiangAssetCategory.Terrain);
            Assert.That(character.MaxTriangles, Is.EqualTo(120000));
            Assert.That(character.MaxTextureSize, Is.EqualTo(4096));
            Assert.That(npc.MaxTriangles, Is.EqualTo(60000));
            Assert.That(npc.MaxTextureSize, Is.EqualTo(2048));
            Assert.That(terrain.MaxTriangles, Is.EqualTo(250000));
        }

        [Test]
        public void ManagedCategoryFoldersExist()
        {
            var folders = new[]
            {
                "Characters", "NPCs", "Monsters", "Weapons", "Environment",
                "Terrain", "Vegetation", "VFX", "UI", "Audio"
            };
            Assert.That(AssetDatabase.IsValidFolder(MoxiangAssetPolicy.ManagedRoot), Is.True);
            foreach (var folder in folders)
                Assert.That(AssetDatabase.IsValidFolder(MoxiangAssetPolicy.ManagedRoot + "/" + folder), Is.True, folder);
        }

        [Test]
        public void CurrentManagedTreePassesAssetValidation()
        {
            var report = MoxiangAssetValidator.ValidateAll(false);
            Assert.That(report.rootExists, Is.True);
            Assert.That(report.errors, Is.Zero,
                string.Join("\n", report.issues.ConvertAll(issue => issue.path + ": " + issue.message)));
        }
    }
}
