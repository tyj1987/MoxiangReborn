using System;
using System.IO;
using TMPro;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem.UI;
using UnityEngine.SceneManagement;

namespace Moxiang.Editor
{
    public static class ConnectionSceneBuilder
    {
        public const string ScenePath = "Assets/Moxiang/Scenes/ConnectionValidation.unity";
        private static TMP_FontAsset font;

        public static void Create()
        {
            if (File.Exists(ScenePath)) throw new InvalidOperationException("Connection validation scene already exists; edit it explicitly.");
            if (SceneManager.GetActiveScene().isDirty) throw new InvalidOperationException("Save the current scene before creating validation scene.");
            font = AssetDatabase.LoadAssetAtPath<TMP_FontAsset>("Assets/TextMesh Pro/Resources/Fonts & Materials/LiberationSans SDF.asset");
            var heightfield = AssetDatabase.LoadAssetAtPath<ImportedHeightField>("Assets/Moxiang/Derived/Map10/Map10.mxhasset");
            if (font == null || heightfield == null) throw new InvalidOperationException("Import TMP essentials and Map10 first.");
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            var camera = new GameObject("MainCamera", typeof(Camera), typeof(AudioListener)).GetComponent<Camera>();
            camera.tag = "MainCamera";
            camera.transform.position = new Vector3(38, 42, -45);
            camera.transform.LookAt(new Vector3(0, 2, 0));
            camera.backgroundColor = new Color(0.06f, 0.08f, 0.1f);
            camera.clearFlags = CameraClearFlags.SolidColor;
            camera.nearClipPlane = 0.1f; camera.farClipPlane = 300;
            var light = new GameObject("Sun", typeof(Light)).GetComponent<Light>();
            light.type = LightType.Directional; light.intensity = 1.3f;
            light.transform.rotation = Quaternion.Euler(50, -30, 0);
            var terrain = new GameObject("Map10GeometryInspection", typeof(MeshFilter), typeof(MeshRenderer));
            terrain.GetComponent<MeshFilter>().sharedMesh = heightfield.inspectionMesh;
            Directory.CreateDirectory("Assets/Moxiang/Scenes");
            var material = new Material(Shader.Find("Universal Render Pipeline/Lit")) { name = "InspectionTerrain" };
            material.SetColor("_BaseColor", new Color(0.23f, 0.32f, 0.26f));
            material.SetFloat("_Smoothness", 0);
            AssetDatabase.CreateAsset(material, "Assets/Moxiang/Scenes/InspectionTerrain.mat");
            terrain.GetComponent<MeshRenderer>().sharedMaterial = material;

            var canvas = new GameObject("ConnectionCanvas", typeof(RectTransform), typeof(Canvas), typeof(UnityEngine.UI.CanvasScaler), typeof(UnityEngine.UI.GraphicRaycaster));
            canvas.GetComponent<Canvas>().renderMode = RenderMode.ScreenSpaceOverlay;
            var scaler = canvas.GetComponent<UnityEngine.UI.CanvasScaler>();
            scaler.uiScaleMode = UnityEngine.UI.CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1280, 720); scaler.matchWidthOrHeight = 0.5f;
            var panel = Rect("ConnectionPanel", canvas.transform, new Vector2(400, 660));
            panel.anchorMin = panel.anchorMax = new Vector2(0, 0.5f); panel.pivot = new Vector2(0, 0.5f); panel.anchoredPosition = new Vector2(24, 0);
            panel.gameObject.AddComponent<UnityEngine.UI.Image>().color = new Color(0.025f, 0.04f, 0.06f, 0.94f);
            var logic = panel.gameObject.AddComponent<ConnectionPanel>();
            var inputController = terrain.AddComponent<Map10InputController>();
            var mapCollider = terrain.AddComponent<MeshCollider>();
            inputController.mapCollider = mapCollider;
            mapCollider.sharedMesh = heightfield.inspectionMesh;
            Label(panel, "MOXIANG", 24, -20, 350, 40, 30);
            Label(panel, "Connection validation / Development build", 24, -64, 350, 32, 15);
            logic.host = Input(panel, "Host", "127.0.0.1", -110, false);
            logic.port = Input(panel, "Port", "", -160, false);
            logic.user = Input(panel, "User", "", -210, false);
            logic.password = Input(panel, "Password", "", -260, true);
            logic.connect = Button(panel, "Connect", -316, out _);
            logic.disconnect = Button(panel, "Disconnect", -362, out _);
            logic.presentRevive = Button(panel, "原地复活", -580, out _);
            logic.loginRevive = Button(panel, "登录点复活", -616, out _);
            logic.status = Label(panel, "Idle", 24, -410, 350, 60, 16);
            inputController.connection = logic;
            var targetSelection = terrain.AddComponent<TargetSelectionController>();
            targetSelection.connection = logic;
            targetSelection.worldCamera = camera;
            var skillInput = terrain.AddComponent<SkillHotkeyController>();
            skillInput.connection = logic;
            skillInput.targetSelection = targetSelection;
            var entityRegistry = terrain.AddComponent<ServerEntityRegistry>();
            entityRegistry.connection = logic;
            var questState = terrain.AddComponent<QuestStateController>();
            questState.connection = logic;
            var chatLog = terrain.AddComponent<ChatLogController>();
            chatLog.connection = logic;
            logic.characters = new UnityEngine.UI.Button[5]; logic.characterLabels = new TMP_Text[5];
            for (int i = 0; i < 5; ++i) logic.characters[i] = Button(panel, "Empty slot", -474 - i * 32, out logic.characterLabels[i], 28);
            var note = Label(canvas.transform, "MAP 10 · GEOMETRY INSPECTION\nMaterials, collision and gameplay are not accepted yet.", 0, 0, 600, 70, 18);
            var noteRect = note.rectTransform; noteRect.anchorMin = noteRect.anchorMax = new Vector2(1, 0); noteRect.pivot = new Vector2(1, 0); noteRect.anchoredPosition = new Vector2(-28, 20);
            note.alignment = TextAlignmentOptions.BottomRight;
            var events = new GameObject("EventSystem", typeof(EventSystem), typeof(InputSystemUIInputModule));
            events.GetComponent<InputSystemUIInputModule>().AssignDefaultActions();
            if (!EditorSceneManager.SaveScene(scene, ScenePath)) throw new IOException("Could not save validation scene.");
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
            AssetDatabase.SaveAssets();
            Debug.Log("MXH_CONNECTION_SCENE_CREATED");
        }

        public static void AddTradeControls()
        {
            if (SceneManager.GetActiveScene().path != ScenePath) throw new InvalidOperationException("Open the connection validation scene first.");
            var canvas = GameObject.Find("ConnectionCanvas");
            var connection = UnityEngine.Object.FindFirstObjectByType<ConnectionPanel>();
            if (canvas == null || connection == null) throw new InvalidOperationException("Missing connection canvas or controller.");
            if (canvas.transform.Find("TradePanel") != null) return;
            font = AssetDatabase.LoadAssetAtPath<TMP_FontAsset>("Assets/TextMesh Pro/Resources/Fonts & Materials/LiberationSans SDF.asset");
            if (font == null) throw new InvalidOperationException("Missing UI font.");
            var panel = Rect("TradePanel", canvas.transform, new Vector2(400, 640));
            panel.gameObject.SetActive(false);
            panel.anchorMin = panel.anchorMax = new Vector2(1, 0.5f); panel.pivot = new Vector2(1, 0.5f); panel.anchoredPosition = new Vector2(-24, 0);
            panel.gameObject.AddComponent<UnityEngine.UI.Image>().color = new Color(0.025f, 0.04f, 0.06f, 0.94f);
            panel.GetComponent<UnityEngine.UI.Image>().color = new Color(0.025f, 0.04f, 0.06f, 1);
            var trade = panel.gameObject.AddComponent<TradePanel>(); trade.connection = connection;
            trade.heading = Label(panel, "Shop", 24, -16, 350, 34, 20);
            Label(panel, "Development IDs · item names/art pending", 24, -50, 350, 24, 13);
            trade.mode = Button(panel, "Switch shop / inventory", -82, out _);
            trade.quantity = Input(panel, "Quantity", "1", -132, false);
            trade.quantity.contentType = TMP_InputField.ContentType.IntegerNumber; trade.quantity.characterLimit = 5;
            trade.rows = new UnityEngine.UI.Button[8]; trade.rowLabels = new TMP_Text[8];
            for (int i = 0; i < 8; ++i) trade.rows[i] = Button(panel, "—", -180 - i * 40, out trade.rowLabels[i], 34);
            trade.previous = Button(panel, "Previous page", -512, out _, 30);
            trade.next = Button(panel, "Next page", -550, out _, 30);
            trade.pageLabel = Label(panel, "Waiting for server data", 24, -592, 350, 28, 15);
            panel.gameObject.SetActive(true);
            EditorSceneManager.MarkSceneDirty(SceneManager.GetActiveScene());
            if (!EditorSceneManager.SaveScene(SceneManager.GetActiveScene())) throw new IOException("Could not save trade panel.");
        }

        public static void AddQuestDialogueControls()
        {
            if (SceneManager.GetActiveScene().path != ScenePath)
                EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            var canvas = GameObject.Find("ConnectionCanvas");
            var connection = UnityEngine.Object.FindFirstObjectByType<ConnectionPanel>();
            var selection = UnityEngine.Object.FindFirstObjectByType<TargetSelectionController>();
            var questState = UnityEngine.Object.FindFirstObjectByType<QuestStateController>();
            if (canvas == null || connection == null)
                throw new InvalidOperationException("Missing quest dialogue dependencies.");
            if (selection == null)
            {
                var host = GameObject.Find("Map10GeometryInspection") ?? new GameObject("WorldInteractionControllers");
                selection = host.AddComponent<TargetSelectionController>();
                selection.connection = connection;
                selection.worldCamera = Camera.main;
            }
            if (questState == null)
            {
                questState = selection.gameObject.AddComponent<QuestStateController>();
                questState.connection = connection;
            }
            font = AssetDatabase.LoadAssetAtPath<TMP_FontAsset>("Assets/TextMesh Pro/Resources/Fonts & Materials/LiberationSans SDF.asset");
            if (font == null) throw new InvalidOperationException("Missing UI font.");
            var existing = canvas.transform.Find("QuestDialoguePanel");
            var panel = existing == null ? Rect("QuestDialoguePanel", canvas.transform, new Vector2(520, 292)) : (RectTransform)existing;
            if (existing != null)
            {
                var existingDialogue = panel.GetComponent<QuestDialogueController>();
                if (existingDialogue.questChoices == null) existingDialogue.questChoices = Dropdown(panel, -142);
                if (existingDialogue.continueLabel == null && existingDialogue.continueQuest != null)
                    existingDialogue.continueLabel = existingDialogue.continueQuest.GetComponentInChildren<TMP_Text>();
                panel.gameObject.SetActive(true);
                existingDialogue.Hide();
                EditorSceneManager.MarkSceneDirty(SceneManager.GetActiveScene());
                if (!EditorSceneManager.SaveScene(SceneManager.GetActiveScene())) throw new IOException("Could not update quest dialogue panel.");
                return;
            }
            panel.anchorMin = panel.anchorMax = new Vector2(0.5f, 0); panel.pivot = new Vector2(0.5f, 0);
            panel.anchoredPosition = new Vector2(0, 34);
            panel.gameObject.AddComponent<UnityEngine.UI.Image>().color = new Color(0.025f, 0.04f, 0.06f, 0.97f);
            var dialogue = panel.gameObject.AddComponent<QuestDialogueController>();
            dialogue.connection = connection; dialogue.targetSelection = selection; dialogue.questState = questState;
            dialogue.heading = Label(panel, "NPC", 24, -18, 472, 34, 22);
            dialogue.body = Label(panel, "任务对话", 24, -58, 472, 76, 17);
            dialogue.questChoices = Dropdown(panel, -142);
            dialogue.feedback = Label(panel, "", 24, -184, 472, 28, 15);
            dialogue.continueQuest = Button(panel, "继续任务", -218, out dialogue.continueLabel, 34);
            dialogue.close = Button(panel, "关闭", -256, out _, 30);
            dialogue.Hide();
            EditorSceneManager.MarkSceneDirty(SceneManager.GetActiveScene());
            if (!EditorSceneManager.SaveScene(SceneManager.GetActiveScene())) throw new IOException("Could not save quest dialogue panel.");
        }

        private static RectTransform Rect(string name, Transform parent, Vector2 size)
        {
            var rect = new GameObject(name, typeof(RectTransform)).GetComponent<RectTransform>();
            rect.SetParent(parent, false); rect.anchorMin = rect.anchorMax = new Vector2(0, 1); rect.pivot = new Vector2(0, 1); rect.sizeDelta = size;
            return rect;
        }
        private static TMP_Text Label(Transform parent, string text, float x, float y, float width, float height, float size)
        {
            var rect = Rect("Label", parent, new Vector2(width, height)); rect.anchoredPosition = new Vector2(x, y);
            var label = rect.gameObject.AddComponent<TextMeshProUGUI>(); label.font = font; label.fontSize = size; label.text = text;
            label.color = new Color(0.92f, 0.94f, 0.95f); label.raycastTarget = false; label.textWrappingMode = TextWrappingModes.Normal;
            return label;
        }
        private static TMP_InputField Input(Transform parent, string title, string value, float y, bool secret)
        {
            Label(parent, title, 24, y, 88, 36, 17);
            var rect = Rect(title + "Input", parent, new Vector2(250, 36)); rect.anchoredPosition = new Vector2(124, y);
            rect.gameObject.AddComponent<UnityEngine.UI.Image>().color = new Color(0.13f, 0.17f, 0.21f);
            var input = rect.gameObject.AddComponent<TMP_InputField>();
            var viewport = Rect("Viewport", rect, new Vector2(234, 32)); viewport.anchoredPosition = new Vector2(8, -2);
            viewport.gameObject.AddComponent<UnityEngine.UI.RectMask2D>();
            input.textViewport = viewport; input.textComponent = (TextMeshProUGUI)Label(viewport, "", 0, 0, 234, 32, 18);
            input.contentType = secret ? TMP_InputField.ContentType.Password : TMP_InputField.ContentType.Standard;
            input.lineType = TMP_InputField.LineType.SingleLine; input.characterLimit = title == "Host" ? 255 : 63; input.text = value;
            return input;
        }
        private static TMP_Dropdown Dropdown(Transform parent, float y)
        {
            var rect = Rect("QuestChoices", parent, new Vector2(472, 36)); rect.anchoredPosition = new Vector2(24, y);
            rect.gameObject.AddComponent<UnityEngine.UI.Image>().color = new Color(0.13f, 0.17f, 0.21f);
            var dropdown = rect.gameObject.AddComponent<TMP_Dropdown>();
            dropdown.captionText = Label(rect, "", 10, 0, 438, 36, 16);
            dropdown.template = Rect("Template", rect, new Vector2(472, 144));
            dropdown.template.anchoredPosition = new Vector2(0, -38);
            dropdown.template.gameObject.AddComponent<UnityEngine.UI.Image>().color = new Color(0.08f, 0.11f, 0.14f);
            var viewport = Rect("Viewport", dropdown.template, new Vector2(472, 144));
            dropdown.itemText = Label(viewport, "", 10, 0, 438, 32, 16);
            dropdown.template.gameObject.SetActive(false);
            return dropdown;
        }
        private static UnityEngine.UI.Button Button(Transform parent, string text, float y, out TMP_Text label, float height = 38)
        {
            var rect = Rect(text.Replace(" ", "") + "Button", parent, new Vector2(350, height)); rect.anchoredPosition = new Vector2(24, y);
            var image = rect.gameObject.AddComponent<UnityEngine.UI.Image>(); image.color = new Color(0.2f, 0.31f, 0.39f);
            var button = rect.gameObject.AddComponent<UnityEngine.UI.Button>(); button.targetGraphic = image;
            label = Label(rect, text, 8, 0, 334, height, 17); label.alignment = TextAlignmentOptions.Midline;
            return button;
        }
    }
}
