using UnityEngine;

namespace Moxiang
{
    /// <summary>Stores only server-supplied quest identity and state.</summary>
    public sealed class QuestStateController : MonoBehaviour
    {
        public ConnectionPanel connection;
        public uint QuestId { get; private set; }
        public uint State { get; private set; }
        private ulong generation;

        private void OnEnable() { if (connection != null) connection.CoreEventReceived += OnCoreEvent; }
        private void OnDisable() { if (connection != null) connection.CoreEventReceived -= OnCoreEvent; }

        private void OnCoreEvent(CoreEvent e)
        {
            if (e.type == NativeClient.EventDisconnected || e.state != CoreState.InGame)
            {
                QuestId = 0; State = 0; generation = 0; return;
            }
            if (generation != 0 && e.mapGeneration != generation) { QuestId = 0; State = 0; }
            generation = e.mapGeneration;
            if (e.type == NativeClient.EventQuestUpdated) { QuestId = e.argument0; State = e.argument1; }
        }
    }
}
