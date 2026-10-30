using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace Moxiang
{
    /// <summary>Visible server-authoritative quest choice for a selected NPC.</summary>
    [RequireComponent(typeof(CanvasGroup))]
    public sealed class QuestDialogueController : MonoBehaviour
    {
        public ConnectionPanel connection;
        public TargetSelectionController targetSelection;
        public QuestStateController questState;
        public TMP_Text heading;
        public TMP_Text body;
        public TMP_Text feedback;
        public TMP_Dropdown questChoices;
        public Button continueQuest;
        public TMP_Text continueLabel;
        public Button close;

        private ushort npcIndex;
        private ushort questId;
        private bool awaiting;
        private CanvasGroup visibility;
        private QuestInteractionCatalog catalog;
        private readonly System.Collections.Generic.List<QuestInteractionEntry> candidates = new();

        private void OnEnable()
        {
            visibility = GetComponent<CanvasGroup>();
            if (targetSelection != null) targetSelection.TargetSelected += OnTargetSelected;
            if (connection != null) connection.CoreEventReceived += OnCoreEvent;
            if (continueQuest != null) continueQuest.onClick.AddListener(Submit);
            if (close != null) close.onClick.AddListener(Hide);
            if (questChoices != null) questChoices.onValueChanged.AddListener(SelectQuestChoice);
            try { catalog ??= QuestInteractionCatalog.LoadDefault(); }
            catch (System.Exception ex) { Debug.LogError("Quest interaction catalog rejected: " + ex.Message); }
        }

        private void OnDisable()
        {
            if (targetSelection != null) targetSelection.TargetSelected -= OnTargetSelected;
            if (connection != null) connection.CoreEventReceived -= OnCoreEvent;
            if (continueQuest != null) continueQuest.onClick.RemoveListener(Submit);
            if (close != null) close.onClick.RemoveListener(Hide);
            if (questChoices != null) questChoices.onValueChanged.RemoveListener(SelectQuestChoice);
        }

        private void OnTargetSelected(uint objectId, Vector2 _)
        {
            var selected = targetSelection == null ? null : targetSelection.selected;
            if (!TryOpen(selected, questState))
            {
                Hide();
            }
        }

        public bool TryOpen(TargetSelectable selected, uint activeQuestId)
        {
            var ids = new System.Collections.Generic.HashSet<uint>();
            if (activeQuestId != 0) ids.Add(activeQuestId);
            return TryOpenInternal(selected, ids, null);
        }

        public bool TryOpen(TargetSelectable selected, QuestStateController state)
        {
            return TryOpenInternal(selected, state == null ? null : state.ActiveQuests.Keys, state == null ? null : state.ActiveQuests);
        }

        public bool TryOpen(TargetSelectable selected, System.Collections.Generic.IEnumerable<uint> activeQuestIds)
        {
            return TryOpenInternal(selected, activeQuestIds, null);
        }

        private bool TryOpenInternal(TargetSelectable selected, System.Collections.Generic.IEnumerable<uint> activeQuestIds,
            System.Collections.Generic.IReadOnlyDictionary<uint, uint> activeStates)
        {
            var objectId = selected == null ? 0u : selected.objectId;
            if (selected == null || !selected.isNpc || objectId == 0 || objectId > ushort.MaxValue || catalog == null || activeQuestIds == null) return false;
            var active = new System.Collections.Generic.HashSet<uint>(activeQuestIds);
            candidates.Clear();
            var seen = new System.Collections.Generic.HashSet<string>();
            foreach (var entry in catalog.Find(objectId))
                if (active.Contains(entry.questId) && entry.textResolved &&
                    (activeStates == null || activeStates.TryGetValue(entry.questId, out var stage) && stage == entry.stage) &&
                    seen.Add(entry.questId + ":" + entry.stage)) candidates.Add(entry);
            if (candidates.Count == 0) return false;
            npcIndex = (ushort)objectId;
            awaiting = false;
            if (questChoices != null)
            {
                questChoices.ClearOptions();
                questChoices.AddOptions(candidates.ConvertAll(e => e.title + " (#" + e.questId + ")"));
                questChoices.SetValueWithoutNotify(0);
                questChoices.gameObject.SetActive(candidates.Count > 1);
            }
            SelectQuestChoice(0);
            if (feedback != null) feedback.text = string.Empty;
            if (continueQuest != null) continueQuest.interactable = true;
            SetVisible(true);
            return true;
        }

        public void SetCatalogForTests(QuestInteractionCatalog value) { catalog = value; }

        public void SelectQuestChoice(int index)
        {
            if (index < 0 || index >= candidates.Count) return;
            var entry = candidates[index];
            questId = (ushort)entry.questId;
            if (heading != null) heading.text = string.IsNullOrWhiteSpace(entry.name) ? "NPC " + npcIndex : entry.name;
            var dialogue = string.IsNullOrWhiteSpace(entry.dialogueText) ? entry.description : entry.dialogueText;
            if (body != null) body.text = entry.title + (string.IsNullOrWhiteSpace(dialogue) ? string.Empty : "\n" + dialogue);
            QuestInteractionOption progressOption = null;
            if (entry.options != null)
                foreach (var option in entry.options)
                    if (option != null && option.linkType == 5 && !string.IsNullOrWhiteSpace(option.text)) { progressOption = option; break; }
            if (continueLabel != null) continueLabel.text = progressOption == null ? "继续任务" : progressOption.text;
        }

        public void Submit()
        {
            if (awaiting || connection == null || npcIndex == 0 || questId == 0) return;
            var result = connection.QuestNpcTalk(npcIndex, questId);
            awaiting = result == CoreResult.Ok;
            if (continueQuest != null) continueQuest.interactable = !awaiting;
            if (feedback != null) feedback.text = awaiting ? "正在等待服务器确认…" : "无法提交：" + result;
        }

        private void OnCoreEvent(CoreEvent e)
        {
            if (e.type == NativeClient.EventDisconnected || e.state != CoreState.InGame) { Hide(); return; }
            if (e.type != NativeClient.EventQuestNpcResponse || e.argument0 != npcIndex || e.argument1 != questId) return;
            awaiting = false;
            if (continueQuest != null) continueQuest.interactable = true;
            if (feedback != null) feedback.text = e.result == CoreResult.Ok
                ? "任务步骤已更新" : "该 NPC 当前不能推进此任务";
        }

        public void Hide()
        {
            awaiting = false;
            npcIndex = 0;
            questId = 0;
            candidates.Clear();
            SetVisible(false);
        }

        public bool IsVisible => visibility != null && visibility.alpha > 0.5f;

        private void SetVisible(bool value)
        {
            if (visibility == null) visibility = GetComponent<CanvasGroup>();
            visibility.alpha = value ? 1 : 0;
            visibility.interactable = value;
            visibility.blocksRaycasts = value;
        }
    }
}
