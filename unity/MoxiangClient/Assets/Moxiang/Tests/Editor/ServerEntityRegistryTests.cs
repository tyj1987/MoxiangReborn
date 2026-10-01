using System.Reflection;
using NUnit.Framework;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class ServerEntityRegistryTests
    {
        [Test]
        public void BuffersNpcUntilAuditedMapPresentationIsReady()
        {
            var connectionGo = new GameObject("connection");
            var connection = connectionGo.AddComponent<ConnectionPanel>();
            var visuals = connectionGo.AddComponent<MapVisualController>();
            connection.mapVisuals = visuals;
            var registryGo = new GameObject("registry");
            var registry = registryGo.AddComponent<ServerEntityRegistry>();
            registry.connection = connection;
            registry.npcPrefab = new GameObject("audited-npc-prefab");
            var encoded = System.Text.Encoding.UTF8.GetBytes("NPC 572");
            var text = new byte[256];
            encoded.CopyTo(text, 0);
            var item = new CoreEvent {
                type = NativeClient.EventNpcAdded, state = CoreState.InGame,
                mapGeneration = 7, argument0 = 572,
                argument1 = 3500u | (46900u << 16), text = text, textLength = (uint)encoded.Length
            };
            var accept = typeof(ServerEntityRegistry).GetMethod("OnCoreEvent", BindingFlags.Instance | BindingFlags.NonPublic);
            var update = typeof(ServerEntityRegistry).GetMethod("Update", BindingFlags.Instance | BindingFlags.NonPublic);

            accept.Invoke(registry, new object[] { item });
            Assert.That(registryGo.transform.childCount, Is.Zero);
            typeof(MapVisualController).GetProperty("IsReady").SetValue(visuals, true);
            update.Invoke(registry, null);

            Assert.That(registryGo.transform.childCount, Is.EqualTo(1));
            var selectable = registryGo.GetComponentInChildren<TargetSelectable>();
            Assert.That(selectable, Is.Not.Null);
            Assert.That(selectable.objectId, Is.EqualTo(572));
            Assert.That(selectable.isNpc, Is.True);

            Object.DestroyImmediate(registry.npcPrefab);
            Object.DestroyImmediate(registryGo);
            Object.DestroyImmediate(connectionGo);
        }

        [Test]
        public void SelectsAuditedNpcPrefabByServerVisualKind()
        {
            var registryGo = new GameObject("registry");
            var registry = registryGo.AddComponent<ServerEntityRegistry>();
            var fallback = new GameObject("fallback");
            var kind54 = new GameObject("kind-54");
            registry.npcPrefab = fallback;
            registry.npcVisuals = new[]
            {
                new ServerEntityRegistry.VisualKindPrefab { visualKind = 54, prefab = kind54 }
            };
            var accept = typeof(ServerEntityRegistry).GetMethod("OnCoreEvent", BindingFlags.Instance | BindingFlags.NonPublic);
            accept.Invoke(registry, new object[] { new CoreEvent
            {
                type = NativeClient.EventNpcAdded, state = CoreState.InGame, argument0 = 572,
                argument1 = 10u | (20u << 16), reserved0 = 54, mapGeneration = 1,
                text = new byte[256]
            }});

            var selectable = registryGo.GetComponentInChildren<TargetSelectable>();
            Assert.That(selectable, Is.Not.Null);
            Assert.That(selectable.transform.name, Does.StartWith("kind-54"));
            Assert.That(selectable.visualKind, Is.EqualTo(54));

            Object.DestroyImmediate(registryGo);
            Object.DestroyImmediate(fallback);
            Object.DestroyImmediate(kind54);
        }

        [Test]
        public void SelectsAuditedMonsterPrefabByServerVisualKind()
        {
            var registryGo = new GameObject("registry");
            var registry = registryGo.AddComponent<ServerEntityRegistry>();
            var kind73 = new GameObject("kind-73");
            kind73.AddComponent<MeshRenderer>();
            registry.monsterVisuals = new[]
            {
                new ServerEntityRegistry.MonsterVisualPrefab { visualKind = 73, prefab = kind73 }
            };
            var accept = typeof(ServerEntityRegistry).GetMethod("OnCoreEvent", BindingFlags.Instance | BindingFlags.NonPublic);
            accept.Invoke(registry, new object[] { new CoreEvent
            {
                type = NativeClient.EventMonsterAdded, state = CoreState.InGame, argument0 = 50023,
                argument1 = 10u | (20u << 16), reserved0 = 73, mapGeneration = 1,
                text = new byte[256]
            }});

            var selectable = registryGo.GetComponentInChildren<TargetSelectable>();
            Assert.That(selectable, Is.Not.Null);
            Assert.That(selectable.transform.name, Does.StartWith("kind-73"));
            Assert.That(selectable.visualKind, Is.EqualTo(73));
            Assert.That(selectable.GetComponent<BoxCollider>(), Is.Not.Null);

            Object.DestroyImmediate(registryGo);
            Object.DestroyImmediate(kind73);
        }
    }
}
