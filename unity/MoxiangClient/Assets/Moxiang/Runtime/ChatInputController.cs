using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace Moxiang
{
    /// <summary>Sends public chat through the authoritative native core.</summary>
    public sealed class ChatInputController : MonoBehaviour
    {
        public ConnectionPanel connection;
        public TMP_InputField input;
        public Button send;

        private void OnEnable()
        {
            if (send != null) send.onClick.AddListener(SendMessage);
        }

        private void OnDisable()
        {
            if (send != null) send.onClick.RemoveListener(SendMessage);
        }

        public void SendMessage()
        {
            if (connection == null || input == null || string.IsNullOrWhiteSpace(input.text)) return;
            if (connection.Chat(input.text.Trim()) == CoreResult.Ok) input.text = string.Empty;
        }
    }
}
