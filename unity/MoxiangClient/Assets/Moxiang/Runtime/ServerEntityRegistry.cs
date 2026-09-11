using System.Collections.Generic;
using System.Globalization;
using System;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Materializes server entity add events only when an audited prefab is assigned.</summary>
    public sealed class ServerEntityRegistry : MonoBehaviour
    {
        public ConnectionPanel connection;
        public GameObject monsterPrefab;
        public GameObject npcPrefab;
        public GameObject groundDropPrefab;
        public float mapWidth = 51200f;
        public float mapDepth = 102400f;
        private readonly Dictionary<uint, GameObject> entities = new Dictionary<uint, GameObject>();
        private readonly Dictionary<uint, GameObject> drops = new Dictionary<uint, GameObject>();

        private void OnEnable() { if (connection != null) connection.CoreEventReceived += OnCoreEvent; }
        private void OnDisable() { if (connection != null) connection.CoreEventReceived -= OnCoreEvent; }

        private void OnCoreEvent(CoreEvent e)
        {
            if (e.type == NativeClient.EventDisconnected || e.state != CoreState.InGame)
            {
                foreach (var entity in entities.Values) if (entity != null) Destroy(entity);
                entities.Clear();
                foreach (var drop in drops.Values) if (drop != null) Destroy(drop);
                drops.Clear();
                if (e.type == NativeClient.EventDisconnected) return;
            }
            if (e.type == NativeClient.EventEntityRemoved)
            {
                if (entities.TryGetValue(e.argument0, out var removed)) Destroy(removed);
                entities.Remove(e.argument0);
                if (drops.TryGetValue(e.argument0, out var removedDrop)) Destroy(removedDrop);
                drops.Remove(e.argument0);
                return;
            }
            if (e.type == NativeClient.EventGroundDrop)
            {
                if (groundDropPrefab == null || e.argument0 == 0 || e.argument1 == 0) return;
                var fields = e.Text.Split(',');
                if (fields.Length != 3 || !ushort.TryParse(fields[0], NumberStyles.None, CultureInfo.InvariantCulture, out var dropCount) ||
                    !float.TryParse(fields[1], NumberStyles.Float, CultureInfo.InvariantCulture, out var dropX) ||
                    !float.TryParse(fields[2], NumberStyles.Float, CultureInfo.InvariantCulture, out var dropZ) ||
                    dropCount == 0 || !float.IsFinite(dropX) || !float.IsFinite(dropZ)) return;
                if (drops.TryGetValue(e.argument0, out var oldDrop)) Destroy(oldDrop);
                var dropInstance = Instantiate(groundDropPrefab, new MapCoordinates(mapWidth, mapDepth).ToScene(new Vector3(dropX, 0, dropZ)), Quaternion.identity, transform);
                var dropSelectable = dropInstance.GetComponent<TargetSelectable>() ?? dropInstance.AddComponent<TargetSelectable>();
                dropSelectable.objectId = e.argument0;
                var state = dropInstance.GetComponent<ServerGroundDrop>() ?? dropInstance.AddComponent<ServerGroundDrop>();
                state.Initialize(e.argument0, e.argument1, dropCount);
                var pickup = dropInstance.GetComponent<GroundDropPickup>() ?? dropInstance.AddComponent<GroundDropPickup>();
                pickup.connection = connection;
                pickup.drop = state;
                drops[e.argument0] = dropInstance;
                return;
            }
            if (e.type == NativeClient.EventPickupConfirmed)
            {
                if (drops.TryGetValue(e.argument0, out var picked)) Destroy(picked);
                drops.Remove(e.argument0);
                return;
            }
            if (e.type == NativeClient.EventEntityLife)
            {
                if (entities.TryGetValue(e.argument0, out var live) && live != null)
                {
                    var healthSink = live.GetComponent<ServerEntityHealth>();
                    if (healthSink != null) healthSink.SetCurrentLife(e.argument1);
                    if (e.argument1 == 0) live.SetActive(false);
                }
                return;
            }
            if (e.type == NativeClient.EventEntityShield)
            {
                if (entities.TryGetValue(e.argument0, out var shielded) && shielded != null)
                {
                    var healthSink = shielded.GetComponent<ServerEntityHealth>();
                    if (healthSink != null) healthSink.SetCurrentShield(e.argument1);
                }
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
            // LifeNotify is authoritative; every materialized entity must have
            // a state sink even when the audited visual prefab omits it.
            var health = instance.GetComponent<ServerEntityHealth>();
            if (health == null) instance.AddComponent<ServerEntityHealth>();
            entities[e.argument0] = instance;
        }
    }
}
