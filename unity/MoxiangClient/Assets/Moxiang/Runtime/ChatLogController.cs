using System.Collections.Generic;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Bounded view model for server-supplied public chat messages.</summary>
    public sealed class ChatLogController : MonoBehaviour
    {
        public ConnectionPanel connection;
        public int capacity = 100;
        public IReadOnlyList<string> Messages => messages;
        private readonly List<string> messages = new List<string>();
        private ulong generation;

        private void OnEnable() { if (connection != null) connection.CoreEventReceived += OnCoreEvent; }
        private void OnDisable() { if (connection != null) connection.CoreEventReceived -= OnCoreEvent; }

        private void OnCoreEvent(CoreEvent e)
        {
            if (e.type == NativeClient.EventDisconnected || e.state != CoreState.InGame)
            { messages.Clear(); generation = 0; return; }
            if (generation != 0 && e.mapGeneration != generation) messages.Clear();
            generation = e.mapGeneration;
            if (e.type != NativeClient.EventChatMessage || string.IsNullOrEmpty(e.Text)) return;
            messages.Add(e.Text);
            var limit = Mathf.Clamp(capacity, 1, 500);
            if (messages.Count > limit) messages.RemoveRange(0, messages.Count - limit);
        }
    }
}
