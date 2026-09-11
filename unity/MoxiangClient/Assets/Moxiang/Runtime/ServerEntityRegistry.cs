using System.Collections.Generic;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Materializes server entity add events only when an audited prefab is assigned.</summary>
    public sealed class ServerEntityRegistry : MonoBehaviour
    {
        public ConnectionPanel connection;
        public GameObject monsterPrefab;
        public GameObject npcPrefab;
        public float mapWidth = 51200f;
        public float mapDepth = 102400f;
        private readonly Dictionary<uint, GameObject> entities = new Dictionary<uint, GameObject>();

        private void OnEnable() { if (connection != null) connection.CoreEventReceived += OnCoreEvent; }
        private void OnDisable() { if (connection != null) connection.CoreEventReceived -= OnCoreEvent; }

        private void OnCoreEvent(CoreEvent e)
        {
            if (e.type == NativeClient.EventDisconnected || e.state != CoreState.InGame)
            {
                foreach (var entity in entities.Values) if (entity != null) Destroy(entity);
                entities.Clear();
                if (e.type == NativeClient.EventDisconnected) return;
            }
            if (e.type == NativeClient.EventEntityRemoved)
            {
                if (entities.TryGetValue(e.argument0, out var removed)) Destroy(removed);
                entities.Remove(e.argument0);
                return;
            }
            if (e.type != NativeClient.EventMonsterAdded && e.type != NativeClient.EventNpcAdded || e.argument0 == 0) return;
            var prefab = e.type == NativeClient.EventMonsterAdded ? monsterPrefab : npcPrefab;
            if (prefab == null) return;
            if (entities.TryGetValue(e.argument0, out var old)) Destroy(old);
            ushort x = (ushort)(e.argument1 & 0xffff), z = (ushort)(e.argument1 >> 16);
            var instance = Instantiate(prefab, new MapCoordinates(mapWidth, mapDepth).ToScene(new Vector3(x, 0, z)), Quaternion.identity, transform);
            var selectable = instance.GetComponent<TargetSelectable>() ?? instance.AddComponent<TargetSelectable>();
            selectable.objectId = e.argument0;
            entities[e.argument0] = instance;
        }
    }
}
