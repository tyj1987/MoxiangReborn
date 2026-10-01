using System.IO;
using NUnit.Framework;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class QuestInteractionCatalogTests
    {
        [Test]
        public void LoadsExportedBig5TextAndMap10NpcMappings()
        {
            var path = Path.Combine(Application.dataPath, "StreamingAssets", "Gameplay", "QuestInteractions.json");
            var catalog = QuestInteractionCatalog.Load(File.ReadAllText(path));
            Assert.That(catalog.Count, Is.EqualTo(1230));
            Assert.That(catalog.UnresolvedCount, Is.EqualTo(0));
            Assert.That(catalog.UnresolvedPresentationCount, Is.EqualTo(0));
            Assert.That(catalog.RecoveredPresentationCount, Is.EqualTo(3));
            Assert.That(catalog.Find(38), Has.Some.Matches<QuestInteractionEntry>(e => e.questId == 173 && e.title == "嵩山的殭屍"));
            Assert.That(catalog.Find(554), Has.Some.Matches<QuestInteractionEntry>(e => e.questId == 57 && e.mapId == 10));
            Assert.That(catalog.Find(572), Has.Some.Matches<QuestInteractionEntry>(e => e.questId == 180 && e.mapId == 10));
            var corpse = System.Linq.Enumerable.Single(catalog.Find(572), e => e.questId == 180 && e.stage == 1);
            Assert.That(corpse.dialogueIds, Is.EqualTo(new uint[] { 2230 }));
            Assert.That(corpse.dialogueText, Is.EqualTo("面目全非，幾乎無法辨識誰是誰。"));
            Assert.That(corpse.options[0].text, Is.EqualTo("搜身找尋名牌"));
            var malformedSourcePage = System.Linq.Enumerable.Single(catalog.Find(216), e => e.questId == 38 && e.stage == 1);
            Assert.That(malformedSourcePage.dialogueIds, Is.EqualTo(new uint[] { 12473 }));
            Assert.That(malformedSourcePage.options[0].text, Is.EqualTo("說來聽聽"));
            var hyperlinkTitle = System.Linq.Enumerable.First(catalog.Find(31), e => e.questId == 200);
            Assert.That(hyperlinkTitle.title, Is.EqualTo("玄武藍寶箱任務"));
            Assert.That(hyperlinkTitle.titleSource, Is.EqualTo("NpcHyperTextQuestLink"));
            var recovered = System.Linq.Enumerable.Single(catalog.Find(264), e => e.questId == 152 && e.stage == 0);
            Assert.That(recovered.dialogueIds, Is.EqualTo(new uint[] { 8363 }));
            Assert.That(recovered.options[0].text, Is.EqualTo("我去找回來"));
            Assert.That(recovered.pageSource, Is.EqualTo("RecoveredContiguousNpcMessageSequence"));
        }

        [Test]
        public void RejectsUnsupportedSchemaAndDuplicateIdentity()
        {
            var source = "\"sources\":[{\"path\":\"q\",\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}]";
            Assert.Throws<InvalidDataException>(() => QuestInteractionCatalog.Load("{\"schemaVersion\":2," + source + ",\"entries\":[]}"));
            var entry = "{\"questId\":1,\"stage\":0,\"npcIndex\":2,\"mapId\":3,\"x\":4,\"z\":5,\"textResolved\":true}";
            var json = "{\"schemaVersion\":1," + source + ",\"summary\":{\"interactionCount\":2,\"unresolvedTextCount\":0},\"entries\":[" + entry + "," + entry + "]}";
            Assert.Throws<InvalidDataException>(() => QuestInteractionCatalog.Load(json));
        }
    }
}
