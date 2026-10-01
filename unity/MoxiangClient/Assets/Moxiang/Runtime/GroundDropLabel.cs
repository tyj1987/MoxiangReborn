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
        private UnityEngine.UI.GraphicRaycaster raycaster;
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
            raycaster = panel.GetComponent<UnityEngine.UI.GraphicRaycaster>();
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
            var visible = false;
            if (camera != null && camera.isActiveAndEnabled)
            {
                float depth = Vector3.Dot(canvas.transform.position - camera.transform.position, camera.transform.forward);
                float scale = WorldUnitsPerPixel(depth, camera.fieldOfView, camera.pixelHeight,
                    camera.orthographic, camera.orthographicSize);
                visible = scale > 0 && depth > camera.nearClipPlane && depth < camera.farClipPlane;
                if (visible) visible = !IsOccluded(camera, canvas.transform.position, transform);
                canvas.worldCamera = camera;
                canvas.transform.rotation = camera.transform.rotation;
                if (visible) canvas.transform.localScale = Vector3.one * scale;
            }
            // Keep the label at its world anchor so normal scene depth testing
            // still applies. Only the Canvas scales; gameplay/collider stay put.
            canvas.enabled = visible;
            raycaster.enabled = visible;
            button.interactable = visible && drop.ObjectId != 0 && !pickup.Pending;
            var text = "Item #" + drop.ItemId + " x" + drop.Count;
            if (lastText != text) { label.text = text; lastText = text; }
        }

        public static float WorldUnitsPerPixel(float depth, float fieldOfView, int pixelHeight,
            bool orthographic, float orthographicSize)
        {
            if (!float.IsFinite(depth) || depth <= 0 || pixelHeight <= 0) return 0;
            if (orthographic)
                return float.IsFinite(orthographicSize) && orthographicSize > 0
                    ? 2f * orthographicSize / pixelHeight : 0;
            if (!float.IsFinite(fieldOfView) || fieldOfView <= 0 || fieldOfView >= 180) return 0;
            float units = 2f * depth * Mathf.Tan(fieldOfView * Mathf.Deg2Rad * .5f) / pixelHeight;
            return float.IsFinite(units) ? units : 0;
        }

        public static bool IsOccluded(Camera camera, Vector3 anchor, Transform owner)
        {
            Vector3 ray = anchor - camera.transform.position;
            float distance = ray.magnitude;
            if (distance <= .01f) return false;
            // Ignore this drop's own click collider, but do not let a larger
            // readable label receive clicks through terrain or other entities.
            foreach (var hit in Physics.RaycastAll(camera.transform.position, ray / distance,
                distance - .01f, camera.cullingMask, QueryTriggerInteraction.Ignore))
                if (hit.collider != null && (owner == null || !hit.collider.transform.IsChildOf(owner))) return true;
            return false;
        }
    }
}
