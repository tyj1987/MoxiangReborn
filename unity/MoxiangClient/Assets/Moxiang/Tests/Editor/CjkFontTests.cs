using NUnit.Framework;
using TMPro;
using UnityEditor;

namespace Moxiang.Tests
{
    public sealed class CjkFontTests
    {
        [Test]
        public void BundledFallbackCoversChineseAfterDynamicAtlasReset()
        {
            var primary = AssetDatabase.LoadAssetAtPath<TMP_FontAsset>("Assets/TextMesh Pro/Resources/Fonts & Materials/LiberationSans SDF.asset");
            var fallback = AssetDatabase.LoadAssetAtPath<TMP_FontAsset>("Assets/Moxiang/Fonts/MoxiangCjkFallback.asset");
            Assert.That(primary, Is.Not.Null);
            Assert.That(fallback, Is.Not.Null);
            Assert.That(primary.fallbackFontAssetTable, Does.Contain(fallback));
            Assert.That(fallback.sourceFontFile, Is.Not.Null);
            Assert.That(fallback.atlasPopulationMode, Is.EqualTo(AtlasPopulationMode.Dynamic));
            Assert.That(new SerializedObject(fallback).FindProperty("m_ClearDynamicDataOnBuild").boolValue, Is.True);
            fallback.ClearFontAssetData();
            const string sample = "墨香 地图资源验证 资源未就绪时暂停场景交互；美术质量尚未验收。角色装备背包任务聊天换图失败 HP 100/100";
            Assert.That(primary.HasCharacters(sample, out uint[] missing, true, true), Is.True,
                "Bundled font must repopulate Chinese glyphs after build-time cleanup.");
            Assert.That(missing, Is.Null.Or.Empty);
            Assert.That(fallback.atlasWidth, Is.EqualTo(1024));
            Assert.That(fallback.atlasHeight, Is.EqualTo(1024));
        }
    }
}
