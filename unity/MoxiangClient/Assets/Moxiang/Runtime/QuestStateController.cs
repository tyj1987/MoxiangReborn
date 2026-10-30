using System.Collections.Generic;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Stores only server-supplied quest identity and state.</summary>
    public sealed class QuestStateController : MonoBehaviour
    {
        public ConnectionPanel connection;
        public uint QuestId { get; private set; }
        public uint State { get; private set; }
        public IReadOnlyDictionary<uint, uint> ActiveQuests => activeQuests;
        private readonly Dictionary<uint, uint> activeQuests = new();
        private ulong generation;

        private void OnEnable() { if (connection != null) connection.CoreEventReceived += OnCoreEvent; }
        private void OnDisable() { if (connection != null) connection.CoreEventReceived -= OnCoreEvent; }

        private void OnCoreEvent(CoreEvent e)
        {
            if (e.type == NativeClient.EventDisconnected || e.state != CoreState.InGame)
            {
                Clear(); generation = 0; return;
            }
            if (generation != 0 && e.mapGeneration != generation) Clear();
            generation = e.mapGeneration;
            if (e.type == NativeClient.EventQuestUpdated) ApplyQuestUpdate(e.argument0, e.argument1);
        }

        public void ApplyQuestUpdate(uint questId, uint state)
        {
            if (questId == 0) return;
            QuestId = questId; State = state;
            activeQuests[questId] = state;
        }

        public void Clear()
        {
            QuestId = 0; State = 0; activeQuests.Clear();
        }
    }
}
