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
            Label(panel, "MOXIANG", 24, -20, 350, 40, 30);
            Label(panel, "Connection validation / Development build", 24, -64, 350, 32, 15);
            logic.host = Input(panel, "Host", "127.0.0.1", -110, false);
            logic.port = Input(panel, "Port", "", -160, false);
            logic.user = Input(panel, "User", "", -210, false);
            logic.password = Input(panel, "Password", "", -260, true);
            logic.connect = Button(panel, "Connect", -316, out _);
            logic.disconnect = Button(panel, "Disconnect", -362, out _);
            logic.status = Label(panel, "Idle", 24, -410, 350, 60, 16);
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
