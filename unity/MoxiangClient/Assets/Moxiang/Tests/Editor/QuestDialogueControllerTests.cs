using NUnit.Framework;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace Moxiang.Tests
{
    public sealed class QuestDialogueControllerTests
    {
        private const string CatalogJson = "{\"schemaVersion\":1,\"textEncoding\":\"big5\",\"sources\":[{\"path\":\"q.bin\",\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}],\"summary\":{\"interactionCount\":2,\"unresolvedTextCount\":0},\"entries\":[{\"questId\":57,\"stage\":6,\"npcIndex\":554,\"mapId\":10,\"name\":\"奇怪的草\",\"title\":\"無形子的復活\",\"description\":\"原始說明\",\"dialogueText\":\"原始 NPC 台詞\",\"options\":[{\"textId\":678,\"text\":\"搜身找尋名牌\",\"linkType\":5,\"target\":4}],\"textResolved\":true},{\"questId\":60,\"stage\":6,\"npcIndex\":554,\"mapId\":10,\"name\":\"奇怪的草\",\"title\":\"第二任務\",\"description\":\"\",\"dialogueText\":\"第二台詞\",\"textResolved\":true}]}";

        [Test]
        public void TryOpenShowsOriginalTextAndAllActiveChoices()
        {
            var root = new GameObject("dialogue");
            var controller = root.AddComponent<QuestDialogueController>();
            controller.SetCatalogForTests(QuestInteractionCatalog.Load(CatalogJson));
            controller.heading = Child<TextMeshProUGUI>("heading");
            controller.body = Child<TextMeshProUGUI>("body");
            controller.feedback = Child<TextMeshProUGUI>("feedback");
            controller.continueQuest = Child<Button>("continue");
            controller.continueLabel = Child<TextMeshProUGUI>("continueLabel");
            controller.questChoices = Child<TMP_Dropdown>("choices");
            var npc = new GameObject("npc").AddComponent<TargetSelectable>();
            npc.objectId = 554; npc.isNpc = true;

            Assert.That(controller.TryOpen(npc, new uint[] { 57, 60 }), Is.True);
            Assert.That(controller.IsVisible, Is.True);
            Assert.That(controller.heading.text, Is.EqualTo("奇怪的草"));
            Assert.That(controller.body.text, Does.Contain("無形子的復活"));
            Assert.That(controller.body.text, Does.Contain("原始 NPC 台詞"));
            Assert.That(controller.continueLabel.text, Is.EqualTo("搜身找尋名牌"));
            Assert.That(controller.questChoices.options.Count, Is.EqualTo(2));
            controller.SelectQuestChoice(1);
            Assert.That(controller.body.text, Does.Contain("第二任務"));
            controller.Hide();
            Assert.That(controller.IsVisible, Is.False);
            Assert.That(root.activeSelf, Is.True, "Controller must stay enabled to receive target events while hidden.");

            Object.DestroyImmediate(npc.gameObject);
            Object.DestroyImmediate(root);
        }

        [Test]
        public void TryOpenRejectsInactiveNpcQuestPair()
        {
            var root = new GameObject("dialogue");
            var controller = root.AddComponent<QuestDialogueController>();
            controller.SetCatalogForTests(QuestInteractionCatalog.Load(CatalogJson));
            var npc = new GameObject("npc").AddComponent<TargetSelectable>();
            npc.objectId = 554; npc.isNpc = true;
            Assert.That(controller.TryOpen(npc, new uint[] { 173 }), Is.False);
            Object.DestroyImmediate(npc.gameObject);
            Object.DestroyImmediate(root);
        }

        private static T Child<T>(string name) where T : Component { return new GameObject(name).AddComponent<T>(); }
    }
}
