using System;
using UnityEngine;
using UnityEngine.EventSystems;

namespace Moxiang
{
    public sealed class TargetSelectionController : MonoBehaviour
    {
        public Camera worldCamera;
        public LayerMask targetLayers = ~0;
        public TargetSelectable selected;
        public event Action<uint, Vector2> TargetSelected;

        private void Update()
        {
            if (!Input.GetMouseButtonDown(0) || worldCamera == null ||
                (EventSystem.current != null && EventSystem.current.IsPointerOverGameObject())) return;
            Ray ray = worldCamera.ScreenPointToRay(Input.mousePosition);
            if (!Physics.Raycast(ray, out RaycastHit hit, worldCamera.farClipPlane, targetLayers)) return;
            var candidate = hit.collider.GetComponentInParent<TargetSelectable>();
            if (candidate == null || candidate.objectId == 0) return;
            selected = candidate;
            TargetSelected?.Invoke(candidate.objectId, new Vector2(hit.point.x, hit.point.z));
        }

        public bool TryGetTarget(out uint objectId, out Vector2 gamePosition)
        {
            if (selected == null || selected.objectId == 0)
            {
                objectId = 0; gamePosition = default; return false;
            }
            objectId = selected.objectId;
            gamePosition = new Vector2(selected.transform.position.x, selected.transform.position.z);
            return true;
        }
    }
}
