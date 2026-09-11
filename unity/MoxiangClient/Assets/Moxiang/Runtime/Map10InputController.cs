using UnityEngine;
using UnityEngine.EventSystems;

namespace Moxiang
{
    /// <summary>Translates Map10 world clicks into authoritative legacy move commands.</summary>
    public sealed class Map10InputController : MonoBehaviour
    {
        public ConnectionPanel connection;
        public Camera worldCamera;
        public Collider mapCollider;
        public float mapWidth = 51200f;
        public float mapDepth = 102400f;
        public bool stopWithRightClick = true;

        private MapCoordinates Coordinates => new MapCoordinates(mapWidth, mapDepth);

        private void Update()
        {
            if (connection == null || worldCamera == null || mapCollider == null ||
                connection.Observed.state != CoreState.InGame) return;
            bool stop = stopWithRightClick && Input.GetMouseButtonDown(1);
            bool move = Input.GetMouseButtonDown(0);
            if (!move && !stop) return;
            if (EventSystem.current != null && EventSystem.current.IsPointerOverGameObject()) return;
            Ray ray = worldCamera.ScreenPointToRay(Input.mousePosition);
            if (!mapCollider.Raycast(ray, out RaycastHit hit, worldCamera.farClipPlane)) return;
            Vector3 game = Coordinates.ToGame(hit.point);
            if (game.x < 0 || game.z < 0 || game.x > mapWidth || game.z > mapDepth) return;
            connection.Move((ushort)Mathf.Clamp(Mathf.RoundToInt(game.x), 0, ushort.MaxValue),
                (ushort)Mathf.Clamp(Mathf.RoundToInt(game.z), 0, ushort.MaxValue), stop);
        }
    }
}
