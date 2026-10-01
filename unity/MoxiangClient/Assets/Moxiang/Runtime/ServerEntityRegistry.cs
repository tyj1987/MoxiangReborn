using System.Collections.Generic;
using System.Globalization;
using System;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Materializes server entity add events only when an audited prefab is assigned.</summary>
    public sealed class ServerEntityRegistry : MonoBehaviour
    {
        [Serializable]
        public struct VisualKindPrefab
        {
            public uint visualKind;
            public GameObject prefab;
        }
        [Serializable]
        public struct MonsterVisualPrefab
        {
            public uint visualKind;
            public GameObject prefab;
            public ImportedMotion[] motions;
        }

        public ConnectionPanel connection;
        public GameObject monsterPrefab;
        public GameObject map44TrainingPrefab;
        public MonsterVisualPrefab[] monsterVisuals = Array.Empty<MonsterVisualPrefab>();
        public LegacyAnimationDriver animationDriver;
        public GameObject npcPrefab;
        public VisualKindPrefab[] npcVisuals = Array.Empty<VisualKindPrefab>();
        public GameObject groundDropPrefab;
        public float mapWidth = 51200f;
        public float mapDepth = 102400f;
        private readonly Dictionary<uint, GameObject> entities = new Dictionary<uint, GameObject>();
        private readonly Dictionary<uint, GameObject> drops = new Dictionary<uint, GameObject>();
        private readonly List<CoreEvent> pendingPresentation = new List<CoreEvent>();
        private ulong activeMapGeneration;

        private void Update()
        {
            if (connection == null || !connection.MapPresentationReady || pendingPresentation.Count == 0) return;
            var ready = pendingPresentation.ToArray();
            pendingPresentation.Clear();
            foreach (var item in ready) OnCoreEvent(item);
        }

        private void OnEnable() { if (connection != null) connection.CoreEventReceived += OnCoreEvent; }
        private void OnDisable() { if (connection != null) connection.CoreEventReceived -= OnCoreEvent; ClearPresentation(); }

        public void ClearPresentation()
        {
            foreach (var entity in entities.Values) if (entity != null) { entity.SetActive(false); Destroy(entity); }
            foreach (var drop in drops.Values) if (drop != null) { drop.SetActive(false); Destroy(drop); }
            entities.Clear(); drops.Clear(); activeMapGeneration = 0;
            pendingPresentation.Clear();
        }

        private void OnCoreEvent(CoreEvent e)
        {
            if (e.type == NativeClient.EventDisconnected)
            {
                ClearPresentation();
                return;
            }
            if (connection != null && !connection.MapPresentationReady)
            {
                if (e.state != CoreState.InGame) { pendingPresentation.Clear(); return; }
                if (pendingPresentation.Count != 0 && pendingPresentation[0].mapGeneration != e.mapGeneration)
                    pendingPresentation.Clear();
                if (e.type == NativeClient.EventMonsterAdded || e.type == NativeClient.EventNpcAdded ||
                    e.type == NativeClient.EventGroundDrop || e.type == NativeClient.EventEntityRemoved ||
                    e.type == NativeClient.EventEntityLife || e.type == NativeClient.EventEntityShield ||
                    e.type == NativeClient.EventObjectMovement || e.type == NativeClient.EventSkillHit ||
                    e.type == NativeClient.EventPickupConfirmed)
                {
                    if (pendingPresentation.Count >= 4096)
                    {
                        pendingPresentation.Clear();
                        Debug.LogError("Server entity presentation queue exceeded 4096 events; discarded incomplete map generation.");
                    }
                    else pendingPresentation.Add(e);
                }
                return;
            }
            if (e.state == CoreState.InGame && activeMapGeneration != 0 &&
                e.mapGeneration != activeMapGeneration)
            {
                foreach (var entity in entities.Values) if (entity != null) Destroy(entity);
                entities.Clear();
                foreach (var drop in drops.Values) if (drop != null) Destroy(drop);
                drops.Clear();
            }
            if (e.state == CoreState.InGame) activeMapGeneration = e.mapGeneration;
            else if (e.type == NativeClient.EventDisconnected) activeMapGeneration = 0;
            if (e.state != CoreState.InGame)
            {
                foreach (var entity in entities.Values) if (entity != null) Destroy(entity);
                entities.Clear();
                foreach (var drop in drops.Values) if (drop != null) Destroy(drop);
                drops.Clear();
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
                if (dropInstance.GetComponent<Collider>() == null)
                    dropInstance.AddComponent<BoxCollider>();
                drops[e.argument0] = dropInstance;
                return;
            }
            if (e.type == NativeClient.EventPickupConfirmed)
            {
                if (drops.TryGetValue(e.argument0, out var picked))
                {
                    var pickup = picked.GetComponent<GroundDropPickup>();
                    if (pickup != null) pickup.Confirmed();
                    if (e.result == CoreResult.Ok)
                    {
                        picked.SetActive(false);
                        if (Application.isPlaying) Destroy(picked);
                        else DestroyImmediate(picked);
                    }
                }
                if (e.result == CoreResult.Ok) drops.Remove(e.argument0);
                return;
            }
            if (e.type == NativeClient.EventObjectMovement)
            {
                if (entities.TryGetValue(e.argument0, out var moving) && moving != null)
                    moving.GetComponent<ServerMonsterAnimation>()?.PlayMove();
                return;
            }
            if (e.type == NativeClient.EventSkillHit)
            {
                if (entities.TryGetValue(e.argument0, out var hit) && hit != null && e.argument1 > 0)
                {
                    hit.GetComponent<ServerMonsterAnimation>()?.PlayHit();
                    foreach (var receiver in hit.GetComponentsInChildren<WuguanServerHitReceiver>(true))
                        receiver.AcceptServerHit(e);
                }
                return;
            }
            if (e.type == NativeClient.EventEntityLife)
            {
                if (entities.TryGetValue(e.argument0, out var live) && live != null)
                {
                    var healthSink = live.GetComponent<ServerEntityHealth>();
                    var animation = live.GetComponent<ServerMonsterAnimation>();
                    if (animation != null)
                    {
                        if (e.argument1 == 0) animation.PlayDeath();
                    }
                    healthSink?.SetCurrentLife(e.argument1);
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
            MonsterVisualPrefab monsterVisual = default;
            var prefab = e.type == NativeClient.EventMonsterAdded ? ResolveMonsterPrefab(e.reserved0, out monsterVisual) : ResolvePrefab(npcVisuals, e.reserved0, npcPrefab);
            if (prefab == null) return;
            if (entities.TryGetValue(e.argument0, out var old)) Destroy(old);
            ushort x = (ushort)(e.argument1 & 0xffff), z = (ushort)(e.argument1 >> 16);
            var instance = Instantiate(prefab, new MapCoordinates(mapWidth, mapDepth).ToScene(new Vector3(x, 0, z)), Quaternion.identity, transform);
            var selectable = instance.GetComponent<TargetSelectable>() ?? instance.AddComponent<TargetSelectable>();
            selectable.objectId = e.argument0;
            selectable.isNpc = e.type == NativeClient.EventNpcAdded;
            selectable.visualKind = e.reserved0;
            selectable.displayName = e.Text;
            EnsureSelectableCollider(instance);
            // LifeNotify is authoritative; every materialized entity must have
            // a state sink even when the audited visual prefab omits it.
            var health = instance.GetComponent<ServerEntityHealth>();
            if (health == null) instance.AddComponent<ServerEntityHealth>();
            if (!selectable.isNpc && monsterVisual.motions != null && animationDriver != null)
                instance.AddComponent<ServerMonsterAnimation>().Initialize(animationDriver, monsterVisual.motions);
            // Map44 uses server-spawned monsters. Static wooden props receive no fabricated IDs.
            if(!selectable.isNpc&&connection!=null&&connection.Observed.game.mapNumber==WuguanMap44Content.MapNumber&&
               instance.GetComponent<WuguanServerHitReceiver>()==null)
                instance.AddComponent<WuguanServerHitReceiver>();
            // Only an authoritative add event may supply a training target identity.
            foreach (var receiver in instance.GetComponentsInChildren<WuguanServerHitReceiver>(true))
                receiver.BindServerEntity(e, selectable);
            entities[e.argument0] = instance;
        }

        private GameObject ResolveMonsterPrefab(uint visualKind, out MonsterVisualPrefab selected)
        {
            // Presentation-only skin; object kind, ID, spawn, life and damage stay server-authored.
            if(connection!=null&&connection.Observed.game.mapNumber==WuguanMap44Content.MapNumber&&map44TrainingPrefab)
            {selected=default;return map44TrainingPrefab;}
            if (monsterVisuals != null)
                foreach (var entry in monsterVisuals)
                    if (entry.visualKind == visualKind && entry.prefab != null) { selected = entry; return entry.prefab; }
            selected = default;
            return monsterPrefab;
        }

        private static GameObject ResolvePrefab(VisualKindPrefab[] visuals, uint visualKind, GameObject fallback)
        {
            if (visuals != null)
            {
                foreach (var entry in visuals)
                {
                    if (entry.visualKind == visualKind && entry.prefab != null) return entry.prefab;
                }
            }
            return fallback;
        }

        private static void EnsureSelectableCollider(GameObject instance)
        {
            if (instance.GetComponentInChildren<Collider>() != null) return;
            var renderers = instance.GetComponentsInChildren<Renderer>();
            if (renderers.Length == 0) return;
            var bounds = renderers[0].bounds;
            for (int i = 1; i < renderers.Length; ++i) bounds.Encapsulate(renderers[i].bounds);
            var collider = instance.AddComponent<BoxCollider>();
            collider.center = instance.transform.InverseTransformPoint(bounds.center);
            collider.size = instance.transform.InverseTransformVector(bounds.size);
        }
    }
}
