using System;
using TMPro;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.UI;

namespace Moxiang.Editor
{
    public static class CharacterCreateSceneSetup
    {
        public static void Apply()
        {
            if (UnityEngine.SceneManagement.SceneManager.GetActiveScene().isDirty) throw new InvalidOperationException("Save active scene first.");
            var scene = EditorSceneManager.OpenScene(ConnectionSceneBuilder.ScenePath, OpenSceneMode.Single);
            if (GameObject.Find("CharacterCreatePanel") != null) throw new InvalidOperationException("Character creation panel already exists.");
            var connection = UnityEngine.Object.FindFirstObjectByType<ConnectionPanel>();
            var template = connection.transform as RectTransform;
            var rect = new GameObject("CharacterCreatePanel", typeof(RectTransform), typeof(Image)).GetComponent<RectTransform>();
            rect.SetParent(template.parent, false); rect.anchorMin = rect.anchorMax = new Vector2(1, 1); rect.pivot = new Vector2(1, 1);
            rect.anchoredPosition = new Vector2(-24, -30); rect.sizeDelta = new Vector2(340, 438);
            rect.GetComponent<Image>().color = new Color(0.025f, 0.04f, 0.06f, 0.94f);
            var panel = rect.gameObject.AddComponent<CharacterCreatePanel>(); panel.connection = connection;
            var title = UnityEngine.Object.Instantiate(connection.status, rect); title.text = "CREATE CHARACTER";
            Place(title.rectTransform, 16, -14, 308, 30);
            panel.characterName = UnityEngine.Object.Instantiate(connection.user, rect); panel.characterName.name = "CharacterName";
            panel.characterName.text = ""; panel.characterName.characterLimit = 64; panel.characterName.contentType = TMP_InputField.ContentType.Standard;
            Place(panel.characterName.GetComponent<RectTransform>(), 16, -56, 308, 32);
            panel.optionButtons = new Button[6]; panel.optionLabels = new TMP_Text[6];
            for (int i = 0; i < 6; ++i)
            {
                var button = UnityEngine.Object.Instantiate(connection.connect, rect); button.name = "CreateOption" + i;
                Place(button.GetComponent<RectTransform>(), 16, -102 - i * 36, 308, 30);
                panel.optionButtons[i] = button; panel.optionLabels[i] = button.GetComponentInChildren<TMP_Text>();
            }
            panel.create = UnityEngine.Object.Instantiate(connection.connect, rect); panel.create.name = "SubmitCharacter";
            Place(panel.create.GetComponent<RectTransform>(), 16, -326, 308, 34); panel.create.GetComponentInChildren<TMP_Text>().text = "Create";
            panel.feedback = UnityEngine.Object.Instantiate(connection.status, rect); panel.feedback.text = "Name: 4–16 encoded bytes.\nAppearance preview is not yet available.";
            Place(panel.feedback.rectTransform, 16, -374, 308, 58); panel.feedback.fontSize = 14;
            EditorUtility.SetDirty(panel); EditorSceneManager.SaveScene(scene); AssetDatabase.SaveAssets();
        }
        private static void Place(RectTransform rect, float x, float y, float width, float height)
        {
            rect.anchorMin = rect.anchorMax = new Vector2(0, 1); rect.pivot = new Vector2(0, 1);
            rect.anchoredPosition = new Vector2(x, y); rect.sizeDelta = new Vector2(width, height);
        }
    }
}
