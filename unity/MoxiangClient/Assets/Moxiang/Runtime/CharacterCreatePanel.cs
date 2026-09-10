using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace Moxiang
{
    public sealed class CharacterCreatePanel : MonoBehaviour
    {
        public ConnectionPanel connection;
        public TMP_InputField characterName;
        public Button create;
        public Button[] optionButtons;
        public TMP_Text[] optionLabels;
        public TMP_Text feedback;
        private readonly byte[] options = new byte[6];
        private bool pending;
        private ulong requestedRevision;
        private ushort previousCount;
        private string requestedName;
        private static readonly int[] Counts = { 2, 5, 5, 2, 2, 6 };
        private static readonly string[] Names = { "Sex", "Hair", "Face", "Clothing", "Boots", "Weapon" };
        private void OnEnable()
        {
            create.onClick.AddListener(Create);
            for (int i = 0; i < 6; ++i) { int index = i; optionButtons[i].onClick.AddListener(() => Cycle(index)); }
            RefreshOptions();
        }
        private void Cycle(int index) { options[index] = (byte)((options[index] + 1) % Counts[index]); RefreshOptions(); }
        private void RefreshOptions()
        {
            for (int i = 0; i < 6; ++i) optionLabels[i].text = i == 0 ? (options[i] == 0 ? "Male" : "Female") : Names[i] + " " + (options[i] + 1) + "/" + Counts[i];
        }
        private void Update()
        {
            if (pending && connection != null && connection.Observed.revision > requestedRevision)
            {
                var snapshot = connection.Observed;
                if (snapshot.state == CoreState.CharacterListReady)
                {
                    bool found = false;
                    foreach (var slot in snapshot.characters)
                        if (slot.valid != 0 && slot.DisplayName == requestedName) found = true;
                    feedback.text = found && snapshot.characterCount == previousCount + 1 ? "Character created. Select it to enter." : "Server rejected character creation.";
                    pending = false;
                }
                else if (snapshot.state == CoreState.Failed || snapshot.state == CoreState.Idle)
                { feedback.text = string.IsNullOrEmpty(snapshot.Error) ? "Creation interrupted." : snapshot.Error; pending = false; }
            }
            bool ready = connection != null && connection.Observed.state == CoreState.CharacterListReady && connection.Observed.characterCount < 5;
            create.interactable = ready; characterName.interactable = ready;
            foreach (var button in optionButtons) button.interactable = ready;
        }
        private void Create()
        {
            requestedRevision = connection.Observed.revision; previousCount = connection.Observed.characterCount; requestedName = characterName.text;
            var result = connection.CreateCharacterFromUi(characterName.text, options[0], options[1], options[2], options[3], options[4], options[5]);
            pending = result == CoreResult.Ok;
            feedback.text = result == CoreResult.Ok ? "Waiting for server character list..." : "Create: " + result;
        }
        private void OnDisable()
        {
            create.onClick.RemoveListener(Create);
            foreach (var button in optionButtons) button.onClick.RemoveAllListeners();
        }
    }
}
