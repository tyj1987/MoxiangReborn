using TMPro;
using UnityEngine;

namespace Moxiang
{
    /// <summary>A functional item label, not a substitute for final item artwork.</summary>
    [RequireComponent(typeof(ServerGroundDrop), typeof(GroundDropPickup))]
    public sealed class GroundDropLabel : MonoBehaviour
    {
        public TMP_FontAsset font;
        private Canvas canvas;
        private TextMeshProUGUI label;
        private UnityEngine.UI.Button button;
        private ServerGroundDrop drop;
        private GroundDropPickup pickup;
        private string lastText;

        private void Awake()
        {
            drop = GetComponent<ServerGroundDrop>();
            pickup = GetComponent<GroundDropPickup>();
            var panel = new GameObject("ItemLabel", typeof(RectTransform), typeof(Canvas),
                typeof(UnityEngine.UI.CanvasScaler), typeof(UnityEngine.UI.GraphicRaycaster));
            panel.transform.SetParent(transform, false);
            panel.transform.localPosition = new Vector3(0, .6f, 0);
            panel.transform.localScale = Vector3.one * .01f;
            canvas = panel.GetComponent<Canvas>();
            canvas.renderMode = RenderMode.WorldSpace;
            var rect = panel.GetComponent<RectTransform>();
            rect.sizeDelta = new Vector2(160, 36);
            var background = panel.AddComponent<UnityEngine.UI.Image>();
            background.color = new Color(.08f, .08f, .08f, .95f);
            button = panel.AddComponent<UnityEngine.UI.Button>();
            button.targetGraphic = background;
            button.onClick.AddListener(() => pickup.RequestPickup());
            var text = new GameObject("Item", typeof(RectTransform));
            text.transform.SetParent(panel.transform, false);
            var textRect = text.GetComponent<RectTransform>();
            textRect.anchorMin = Vector2.zero; textRect.anchorMax = Vector2.one;
            textRect.offsetMin = Vector2.zero; textRect.offsetMax = Vector2.zero;
            label = text.AddComponent<TextMeshProUGUI>();
            if (font != null) label.font = font;
            label.fontSize = 17; label.color = Color.white;
            label.alignment = TextAlignmentOptions.Center;
            label.raycastTarget = false;
        }

        private void LateUpdate()
        {
            var camera = Camera.main;
            if (camera != null)
            {
                canvas.worldCamera = camera;
                canvas.transform.rotation = camera.transform.rotation;
            }
            button.interactable = drop.ObjectId != 0 && !pickup.Pending;
            var text = "Item #" + drop.ItemId + " x" + drop.Count;
            if (lastText != text) { label.text = text; lastText = text; }
        }
    }
}
