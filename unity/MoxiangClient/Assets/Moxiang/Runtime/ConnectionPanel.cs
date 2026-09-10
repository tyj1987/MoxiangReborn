using System;
using TMPro;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Development connection UI backed exclusively by native server snapshots.</summary>
    public sealed class ConnectionPanel : MonoBehaviour
    {
        public TMP_InputField host, port, user, password;
        public TMP_Text status;
        public UnityEngine.UI.Button connect, disconnect;
        public UnityEngine.UI.Button[] characters;
        public TMP_Text[] characterLabels;
        private NativeClient client;
        private CoreSnapshot observed;
        private ulong sequence;
        private string failure;
        public CoreSnapshot Observed => observed;
        public event Action<CoreEvent> CoreEventReceived;
        public CoreResult Move(ushort x, ushort z, bool stop)
        {
            return client == null ? CoreResult.NotReady : client.Move(x, z, stop, observed);
        }
        public CoreResult CreateCharacterFromUi(string name, byte sex, byte hair, byte face, byte cloth, byte boots, byte weapon)
        {
            if (client == null) return CoreResult.NotReady;
            try
            {
                var result = client.CreateCharacter(name, sex, hair, face, cloth, boots, weapon, observed);
                failure = result == CoreResult.Ok ? null : "Create character: " + result;
                return result;
            }
            catch (Exception exception) { failure = exception.Message; return CoreResult.InvalidArgument; }
        }
        public void BeginDevelopmentProbe(string loginHost, string loginPort, string account, string secret)
        {
            if (!Debug.isDebugBuild) throw new InvalidOperationException("Development probe is disabled.");
            host.text = loginHost; port.text = loginPort; user.text = account; password.text = secret;
            Connect();
        }
        public void SelectFirstForDevelopmentProbe()
        {
            if (!Debug.isDebugBuild || observed.characters == null) return;
            for (int i = 0; i < observed.characters.Length; ++i)
                if (observed.characters[i].valid != 0) { Select(i); return; }
        }

        private void OnEnable()
        {
            connect.onClick.AddListener(Connect);
            disconnect.onClick.AddListener(Disconnect);
            for (int i = 0; i < characters.Length; ++i)
            {
                int slot = i;
                characters[i].onClick.AddListener(() => Select(slot));
            }
            try { client = new NativeClient(); }
            catch (Exception exception) { failure = exception.GetType().Name + ": " + exception.Message; }
            Refresh();
        }

        private void Update()
        {
            if (client == null) return;
            try
            {
                client.Tick();
                observed = client.Snapshot();
                for (int i = 0; i < 256 && client.PollEvent(out CoreEvent item); ++i)
                {
                    if (item.sessionGeneration != observed.sessionGeneration || item.mapGeneration != observed.mapGeneration) continue;
                    if (item.sequence <= sequence) continue;
                    sequence = item.sequence;
                    if (item.result != CoreResult.Ok) failure = item.Text;
                    CoreEventReceived?.Invoke(item);
                }
                Refresh();
            }
            catch (Exception exception)
            {
                failure = exception.GetType().Name + ": " + exception.Message;
                Release();
                Refresh();
            }
        }

        private void Connect()
        {
            if (client == null) return;
            if (!ushort.TryParse(port.text, out ushort number) || number == 0) { failure = "Enter a valid login port."; Refresh(); return; }
            try
            {
                var result = client.Connect(host.text, number, user.text, password.text);
                failure = result == CoreResult.Ok ? null : "Connect: " + result;
                sequence = 0;
            }
            catch (Exception exception) { failure = exception.Message; }
            finally { password.text = string.Empty; }
            Refresh();
        }

        private void Disconnect()
        {
            if (client == null) return;
            var result = client.Disconnect();
            failure = result == CoreResult.Ok ? null : "Disconnect: " + result;
        }

        private void Select(int slot)
        {
            if (client == null || observed.state != CoreState.CharacterListReady || observed.characters == null ||
                slot >= observed.characters.Length || observed.characters[slot].valid == 0) return;
            var result = client.SelectCharacter(observed.characters[slot].characterId, 0, observed);
            if (result != CoreResult.Ok) failure = "Select: " + result;
        }

        private void Refresh()
        {
            status.text = string.IsNullOrEmpty(failure) ? (client == null ? "Native core unavailable" : observed.state.ToString()) : failure;
            if (client != null && observed.state == CoreState.InGame)
                status.text += "\nMap " + observed.game.mapNumber + " | Level " + observed.game.level +
                    " | HP " + observed.game.life + "/" + observed.game.maxLife;
            if (client != null && observed.state == CoreState.InGame)
                status.text += "\nPosition " + observed.game.positionX + ", " + observed.game.positionZ;
            connect.interactable = client != null && (observed.state == CoreState.Idle || observed.state == CoreState.Failed);
            disconnect.interactable = client != null && observed.state != CoreState.Idle;
            for (int i = 0; i < characters.Length; ++i)
            {
                bool valid = observed.characters != null && i < observed.characters.Length && observed.characters[i].valid != 0;
                characters[i].interactable = valid && observed.state == CoreState.CharacterListReady;
                characterLabels[i].text = valid ? observed.characters[i].DisplayName + "  Lv." + observed.characters[i].level : "Empty slot";
            }
        }

        private void Release()
        {
            var owned = client;
            client = null;
            if (owned == null) return;
            try { owned.Dispose(); }
            catch (Exception exception) { Debug.LogError("Native core cleanup failed: " + exception.GetType().Name); }
        }

        private void OnDisable()
        {
            connect.onClick.RemoveListener(Connect);
            disconnect.onClick.RemoveListener(Disconnect);
            foreach (var button in characters) button.onClick.RemoveAllListeners();
            password.text = string.Empty;
            Release();
        }
        private void OnApplicationQuit() { Release(); }
    }
}
