using System;
using UnityEngine;
using UnityEngine.EventSystems;

namespace Moxiang
{
    public sealed class TargetSelectionController : MonoBehaviour
    {
        public ConnectionPanel connection;
        public Camera worldCamera;
        public LayerMask targetLayers = ~0;
        public TargetSelectable selected;
        public event Action<uint, Vector2> TargetSelected;
        private ulong selectedMapGeneration;

        private void Update()
        {
            if (connection != null)
            {
                var snapshot = connection.Observed;
                if (snapshot.state != CoreState.InGame ||
                    (selected != null && selectedMapGeneration != snapshot.mapGeneration))
                { selected = null; return; }
            }
            if (Input.GetMouseButtonDown(1)) { selected = null; return; }
            if (!Input.GetMouseButtonDown(0) || worldCamera == null ||
                (EventSystem.current != null && EventSystem.current.IsPointerOverGameObject())) return;
            Ray ray = worldCamera.ScreenPointToRay(Input.mousePosition);
            if (!Physics.Raycast(ray, out RaycastHit hit, worldCamera.farClipPlane, targetLayers)) return;
            var candidate = hit.collider.GetComponentInParent<TargetSelectable>();
            Select(candidate);
        }

        public bool Select(TargetSelectable candidate)
        {
            if (candidate == null || candidate.objectId == 0 || !candidate.gameObject.activeInHierarchy) return false;
            selected = candidate;
            selectedMapGeneration = connection == null ? 0 : connection.Observed.mapGeneration;
            if (TryGetTarget(out var id, out var position)) {
                TargetSelected?.Invoke(id, position);
                if (candidate.isNpc && connection != null) connection.InteractWithNpc(id);
                return true;
            }
            selected = null;
            return false;
        }

        public bool TryGetTarget(out uint objectId, out Vector2 gamePosition)
        {
            objectId = 0;
            gamePosition = default;
            if (selected == null || selected.objectId == 0 || !selected.gameObject.activeInHierarchy)
            {
                objectId = 0; gamePosition = default; return false;
            }
            if (connection != null && (connection.Observed.state != CoreState.InGame ||
                selectedMapGeneration != connection.Observed.mapGeneration)) return false;
            var registry = selected.GetComponentInParent<ServerEntityRegistry>();
            if (registry == null) return false;
            var position = new MapCoordinates(registry.mapWidth, registry.mapDepth).ToGame(selected.transform.position);
            objectId = selected.objectId;
            gamePosition = new Vector2(position.x, position.z);
            return true;
        }
    }
}
