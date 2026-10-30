using System;
using System.Diagnostics;
using System.Threading;
using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class RealServerIntegrationTests
    {
        [Test]
        public void NativeMapRoutesLoadPackagedAuditedResource()
        {
            string path = System.IO.Path.Combine(UnityEngine.Application.streamingAssetsPath, "Gameplay", "MapChange.bin");
            using (var client = new NativeClient())
            {
                Assert.That(client.LoadMapRoutes(path), Is.EqualTo(CoreResult.Ok));
                Assert.That(client.LoadMapRoutes(path + ".missing"), Is.EqualTo(CoreResult.InvalidArgument));
                Assert.That(client.LoadMapRoutes(path), Is.EqualTo(CoreResult.Ok));
            }
        }

        [Test]
        public void RealThreeServerGameInAndReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_TRANSFER") == "1") Assert.Ignore("Separate dual-map fixture.");
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_TRADE") == "1") Assert.Ignore("Separate Map12 trade fixture.");
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_EQUIPMENT") == "1") Assert.Ignore("Separate equipment fixture.");
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_ITEM_USE") == "1") Assert.Ignore("Separate item-use fixture.");
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_SELL") == "1") Assert.Ignore("Separate sale fixture.");
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_DISCARD") == "1") Assert.Ignore("Separate discard fixture.");
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_QUEST") == "1") Assert.Ignore("Separate quest fixture.");
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_QUEST_REWARD") == "1") Assert.Ignore("Separate quest reward fixture.");
            string portText = Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT");
            if (string.IsNullOrEmpty(portText)) Assert.Ignore("Requires unity_three_server_smoke.py --editor-test isolated real servers.");
            ushort port = ushort.Parse(portText);
            string user = Environment.GetEnvironmentVariable("MXH_SMOKE_USER");
            string password = Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD");
            string createName = Environment.GetEnvironmentVariable("MXH_SMOKE_CREATE_NAME");
            uint characterId = string.IsNullOrEmpty(createName) ? 111u : 0u;
            bool createSubmitted = false, createConfirmed = false;
            using (var client = new NativeClient())
            {
                ulong lastGeneration = 0;
                for (int attempt = 0; attempt < 100; ++attempt)
                {
                    Assert.That(client.Connect("127.0.0.1", port, user, password), Is.EqualTo(CoreResult.Ok));
                    var timer = Stopwatch.StartNew();
                    var inventory = new InventoryState();
                    CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(30))
                    {
                        client.Tick(); state = client.Snapshot();
                        inventory.Observe(state);
                        while (client.PollEvent(out CoreEvent item))
                        {
                            inventory.Accept(item);
                            if (item.type == 6) { Assert.That(item.result, Is.EqualTo(CoreResult.Ok), item.Text); createConfirmed = true; }
                        }
                        if (state.state == CoreState.CharacterListReady)
                        {
                            if (!string.IsNullOrEmpty(createName) && state.characterCount == 0 && !createSubmitted)
                            {
                                Assert.That(client.CreateCharacter(createName, 0, 0, 0, 0, 0, 0, state), Is.EqualTo(CoreResult.Ok));
                                createSubmitted = true; continue;
                            }
                            Assert.That(state.characterCount, Is.EqualTo(1));
                            if (characterId == 0)
                            {
                                Assert.That(state.characters[0].DisplayName, Is.EqualTo(createName));
                                characterId = state.characters[0].characterId;
                                Assert.That(characterId, Is.GreaterThan(0));
                            }
                            Assert.That(client.SelectCharacter(characterId, 0, state), Is.EqualTo(CoreResult.Ok));
                        }
                        if ((state.state == CoreState.InGame && inventory.Ready) || state.state == CoreState.Failed) break;
                        Thread.Sleep(10);
                    }
                    Assert.That(state.state, Is.EqualTo(CoreState.InGame), state.Error);
                    Assert.That(state.game.playerId, Is.EqualTo(characterId));
                    Assert.That(inventory.Ready, Is.True, "Initial ItemTotalInfo never reached managed inventory");
                    Assert.That(inventory.Slots.Count, Is.EqualTo(124));
                    if (!string.IsNullOrEmpty(createName)) {
                        Assert.That(inventory.Slots[81].ItemId, Is.EqualTo(11000), "Initial weapon");
                        Assert.That(inventory.Slots[82].ItemId, Is.EqualTo(23000), "Initial clothing");
                        Assert.That(inventory.Slots[83].ItemId, Is.EqualTo(27000), "Initial boots");
                    }
                    if (!string.IsNullOrEmpty(createName)) Assert.That(createConfirmed, Is.True);
                    Assert.That(state.game.mapNumber, Is.EqualTo(10));
                    Assert.That(state.sessionGeneration, Is.GreaterThan(lastGeneration));
                    lastGeneration = state.sessionGeneration;
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok));
                    Assert.That(client.Snapshot().state, Is.EqualTo(CoreState.Idle));
                    Thread.Sleep(100);
                }
            }
        }

        [Test]
        public void RealMap10ToMap2TransferAndReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_TRANSFER") != "1") Assert.Ignore("Requires isolated dual-map servers.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            using (var client = new NativeClient()) {
                Assert.That(client.LoadMapRoutes(System.IO.Path.Combine(UnityEngine.Application.streamingAssetsPath,
                    "Gameplay", "MapChange.bin")), Is.EqualTo(CoreResult.Ok));
                for (int attempt = 0; attempt < 2; ++attempt) {
                    Assert.That(client.Connect("127.0.0.1", port, Environment.GetEnvironmentVariable("MXH_SMOKE_USER"),
                        Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD")), Is.EqualTo(CoreResult.Ok));
                    bool selected = false, requested = false, transferred = false, finished = false;
                    uint portal = 0;
                    var seen = new System.Collections.Generic.List<string>();
                    var timer = Stopwatch.StartNew();
                    CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(45)) {
                        client.Tick(); state = client.Snapshot();
                        while (client.PollEvent(out CoreEvent item)) {
                            if (item.type == NativeClient.EventNpcAdded) {
                                uint x = item.argument1 & 65535, z = item.argument1 >> 16;
                                seen.Add(item.argument0 + "@" + x + "," + z);
                                if (x == 46973 && z == 4198) portal = item.argument0;
                            }
                            if (item.type == NativeClient.EventMapChange) {
                                Assert.That(item.result, Is.EqualTo(CoreResult.Ok), "Transfer rejected");
                                Assert.That(item.argument0, Is.EqualTo(2));
                                Assert.That(item.argument1, Is.EqualTo(2));
                                transferred = true;
                            }
                        }
                        Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                        if (state.state == CoreState.CharacterListReady && !selected) {
                            Assert.That(client.SelectCharacter(111, 0, state), Is.EqualTo(CoreResult.Ok)); selected = true;
                        }
                        if (state.state == CoreState.InGame) {
                            Assert.That(state.game.money, Is.EqualTo(4242));
                            if (attempt == 0 && !requested && portal != 0) {
                                Assert.That(state.game.mapNumber, Is.EqualTo(10));
                                Assert.That(client.SubmitNpcInteraction(portal, state), Is.EqualTo(CoreResult.Ok)); requested = true;
                            }
                            if ((attempt == 1 || transferred) && state.game.mapNumber == 2) {
                                Assert.That(state.game.positionX, Is.EqualTo(7211));
                                Assert.That(state.game.positionZ, Is.EqualTo(43329));
                                finished = true; break;
                            }
                        }
                        Thread.Sleep(5);
                    }
                    Assert.That(finished, Is.True, "Map transfer incomplete: " + state.state + "; NPCs=" + string.Join(";", seen));
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok));
                    Thread.Sleep(200); // Allow isolated server disconnect persistence before reconnecting.
                }
            }
        }

        [Test]
        public void RealMerchantPurchasePersistsAcrossReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_TRADE") != "1")
                Assert.Ignore("Requires unity_three_server_smoke.py --editor-test --trade.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            using (var client = new NativeClient()) {
                uint expectedMoney = 100000000;
                for (int attempt = 0; attempt < 2; ++attempt) {
                    var inventory = new InventoryState();
                    var shop = new ShopCatalog();
                    bool selected = false, speechSent = false, speechAck = false, bought = false, ack = false;
                    var timer = Stopwatch.StartNew();
                    Assert.That(client.Connect("127.0.0.1", port,
                        Environment.GetEnvironmentVariable("MXH_SMOKE_USER"),
                        Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD")), Is.EqualTo(CoreResult.Ok));
                    CoreSnapshot state = default;
                    bool finished = false;
                    while (timer.Elapsed < TimeSpan.FromSeconds(30)) {
                        client.Tick(); state = client.Snapshot();
                        inventory.Observe(state); shop.Observe(state);
                        while (client.PollEvent(out CoreEvent item)) {
                            inventory.Accept(item); shop.Accept(item);
                            if (item.type == NativeClient.EventNpcResponse) {
                                Assert.That(item.result, Is.EqualTo(CoreResult.Ok));
                                Assert.That(item.argument0, Is.EqualTo(92)); speechAck = true;
                            }
                            if (item.type == NativeClient.EventBuyResponse) {
                                Assert.That(item.result, Is.EqualTo(CoreResult.Ok), item.Text);
                                Assert.That(item.argument0, Is.EqualTo(53343));
                                Assert.That(item.argument1, Is.EqualTo(1)); ack = true;
                            }
                        }
                        Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                        if (state.state == CoreState.CharacterListReady && !selected) {
                            Assert.That(client.SelectCharacter(111, 0, state), Is.EqualTo(CoreResult.Ok)); selected = true;
                        }
                        if (state.state == CoreState.InGame && inventory.Ready) {
                            Assert.That(state.game.mapNumber, Is.EqualTo(12));
                            if (attempt == 0 && !speechSent) {
                                // NPC_ADD can follow the initial inventory; retry only a locally rejected lookup.
                                var result = client.SubmitNpcInteraction(92, state);
                                Assert.That(result, Is.EqualTo(CoreResult.Ok).Or.EqualTo(CoreResult.Rejected));
                                speechSent = result == CoreResult.Ok;
                            }
                            if (attempt == 0 && shop.Ready && !bought) {
                                Assert.That(shop.NpcId, Is.EqualTo(92));
                                ShopOffer offer = default;
                                foreach (var entry in shop.Offers) if (entry.ItemId == 53343) offer = entry;
                                Assert.That(offer.ItemId, Is.EqualTo(53343));
                                Assert.That(offer.Price, Is.GreaterThan(0).And.LessThan(expectedMoney));
                                Assert.That(state.game.money, Is.EqualTo(expectedMoney));
                                expectedMoney -= offer.Price;
                                Assert.That(client.SubmitBuy(53343, 1, state), Is.EqualTo(CoreResult.Ok)); bought = true;
                            }
                            int count = 0;
                            foreach (var slot in inventory.Slots) if (slot.ItemId == 53343) ++count;
                            if ((attempt == 1 || (ack && speechAck)) && count == 1 && state.game.money == expectedMoney) {
                                finished = true; break;
                            }
                        }
                        Thread.Sleep(10);
                    }
                    Assert.That(finished, Is.True, "Trade did not produce matching money, inventory and acknowledgement, or reconnect lost state.");
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok));
                    Thread.Sleep(200);
                }
            }
        }

        [Test]
        public void RealEquipmentMovePersistsAcrossReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_EQUIPMENT") != "1")
                Assert.Ignore("Requires unity_three_server_smoke.py --editor-test --equipment.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            using (var client = new NativeClient()) {
                for (int attempt = 0; attempt < 3; ++attempt) {
                    var inventory = new InventoryState();
                    bool selected = false, submitted = false, acknowledged = false, finished = false;
                    Assert.That(client.Connect("127.0.0.1", port,
                        Environment.GetEnvironmentVariable("MXH_SMOKE_USER"),
                        Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD")), Is.EqualTo(CoreResult.Ok));
                    var timer = Stopwatch.StartNew();
                    CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(30)) {
                        client.Tick(); state = client.Snapshot(); inventory.Observe(state);
                        while (client.PollEvent(out CoreEvent item)) {
                            inventory.Accept(item);
                            if (item.type == NativeClient.EventItemMoveResponse) {
                                Assert.That(item.result, Is.EqualTo(CoreResult.Ok), item.Text);
                                acknowledged = true;
                            }
                        }
                        Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                        if (state.state == CoreState.CharacterListReady && !selected) {
                            Assert.That(client.SelectCharacter(111, 0, state), Is.EqualTo(CoreResult.Ok)); selected = true;
                        }
                        if (state.state == CoreState.InGame && inventory.Ready) {
                            bool carried = inventory.Slots[0].ItemId == 11000;
                            bool worn = inventory.Slots[81].ItemId == 11000;
                            if (attempt == 0 && !submitted) {
                                Assert.That(worn, Is.True); Assert.That(carried, Is.False);
                                Assert.That(client.SubmitMoveItem(81, 0, state), Is.EqualTo(CoreResult.Ok)); submitted = true;
                            } else if (attempt == 1 && !submitted) {
                                Assert.That(carried, Is.True); Assert.That(worn, Is.False);
                                Assert.That(client.SubmitMoveItem(0, 81, state), Is.EqualTo(CoreResult.Ok)); submitted = true;
                            } else if (attempt == 2 && worn && !carried) {
                                finished = true; break;
                            }
                            if (attempt < 2 && acknowledged && ((attempt == 0 && carried && !worn) ||
                                (attempt == 1 && worn && !carried))) { finished = true; break; }
                        }
                        Thread.Sleep(10);
                    }
                    Assert.That(finished, Is.True, "Equipment move or reconnect persistence did not converge.");
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok));
                    Thread.Sleep(200);
                }
            }
        }

        [Test]
        public void RealConsumableUsePersistsAcrossReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_ITEM_USE") != "1")
                Assert.Ignore("Requires unity_three_server_smoke.py --editor-test --item-use.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            using (var client = new NativeClient()) {
                for (int attempt = 0; attempt < 2; ++attempt) {
                    var inventory = new InventoryState();
                    bool selected = false, submitted = false, acknowledged = false, finished = false;
                    Assert.That(client.Connect("127.0.0.1", port,
                        Environment.GetEnvironmentVariable("MXH_SMOKE_USER"),
                        Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD")), Is.EqualTo(CoreResult.Ok));
                    var timer = Stopwatch.StartNew();
                    CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(30)) {
                        client.Tick(); state = client.Snapshot(); inventory.Observe(state);
                        while (client.PollEvent(out CoreEvent item)) {
                            inventory.Accept(item);
                            if (item.type == NativeClient.EventItemUseResponse) {
                                Assert.That(item.result, Is.EqualTo(CoreResult.Ok), item.Text);
                                Assert.That(item.argument0, Is.EqualTo(0));
                                Assert.That(item.argument1, Is.EqualTo(1));
                                acknowledged = true;
                            }
                        }
                        Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                        if (state.state == CoreState.CharacterListReady && !selected) {
                            Assert.That(client.SelectCharacter(111, 0, state), Is.EqualTo(CoreResult.Ok)); selected = true;
                        }
                        if (state.state == CoreState.InGame && inventory.Ready) {
                            bool present = inventory.Slots[0].ItemId == 1;
                            if (attempt == 0 && !submitted) {
                                Assert.That(present, Is.True);
                                Assert.That(client.SubmitUseItem(0, state), Is.EqualTo(CoreResult.Ok)); submitted = true;
                            } else if ((attempt == 0 && acknowledged && !present) || (attempt == 1 && !present)) {
                                finished = true; break;
                            }
                        }
                        Thread.Sleep(10);
                    }
                    Assert.That(finished, Is.True, "Consumable acknowledgement, inventory refresh or reconnect persistence failed.");
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok));
                    Thread.Sleep(200);
                }
            }
        }

        [Test]
        public void RealMerchantSalePersistsAcrossReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_SELL") != "1")
                Assert.Ignore("Requires unity_three_server_smoke.py --editor-test --sell.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            using (var client = new NativeClient()) {
                uint soldMoney = 0;
                for (int attempt = 0; attempt < 2; ++attempt) {
                    var inventory = new InventoryState(); var shop = new ShopCatalog();
                    bool selected = false, speechSent = false, submitted = false, acknowledged = false, finished = false;
                    Assert.That(client.Connect("127.0.0.1", port,
                        Environment.GetEnvironmentVariable("MXH_SMOKE_USER"),
                        Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD")), Is.EqualTo(CoreResult.Ok));
                    var timer = Stopwatch.StartNew(); CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(30)) {
                        client.Tick(); state = client.Snapshot(); inventory.Observe(state); shop.Observe(state);
                        while (client.PollEvent(out CoreEvent item)) {
                            inventory.Accept(item); shop.Accept(item);
                            if (item.type == NativeClient.EventSellResponse) {
                                Assert.That(item.result, Is.EqualTo(CoreResult.Ok), item.Text);
                                Assert.That(item.argument0, Is.EqualTo(0));
                                Assert.That(item.argument1, Is.EqualTo(53343)); acknowledged = true;
                            }
                        }
                        Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                        if (state.state == CoreState.CharacterListReady && !selected) {
                            Assert.That(client.SelectCharacter(111, 0, state), Is.EqualTo(CoreResult.Ok)); selected = true;
                        }
                        if (state.state == CoreState.InGame && inventory.Ready) {
                            bool present = inventory.Slots[0].ItemId == 53343;
                            if (attempt == 0 && !speechSent) {
                                var result = client.SubmitNpcInteraction(92, state);
                                Assert.That(result, Is.EqualTo(CoreResult.Ok).Or.EqualTo(CoreResult.Rejected));
                                speechSent = result == CoreResult.Ok;
                            }
                            if (attempt == 0 && shop.Ready && !submitted) {
                                Assert.That(state.game.money, Is.EqualTo(1000)); Assert.That(present, Is.True);
                                Assert.That(client.SubmitSell(0, 1, 92, state), Is.EqualTo(CoreResult.Ok)); submitted = true;
                            } else if (attempt == 0 && acknowledged && !present && state.game.money > 1000) {
                                soldMoney = state.game.money; finished = true; break;
                            } else if (attempt == 1 && !present && state.game.money == soldMoney) {
                                finished = true; break;
                            }
                        }
                        Thread.Sleep(10);
                    }
                    Assert.That(finished, Is.True, "Sale acknowledgement, money, inventory refresh or reconnect persistence failed.");
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok)); Thread.Sleep(200);
                }
            }
        }

        [Test]
        public void RealItemDiscardPersistsAcrossReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_DISCARD") != "1")
                Assert.Ignore("Requires unity_three_server_smoke.py --editor-test --discard.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            using (var client = new NativeClient()) {
                for (int attempt = 0; attempt < 2; ++attempt) {
                    var inventory = new InventoryState(); bool selected = false, submitted = false, acknowledged = false, finished = false;
                    Assert.That(client.Connect("127.0.0.1", port,
                        Environment.GetEnvironmentVariable("MXH_SMOKE_USER"),
                        Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD")), Is.EqualTo(CoreResult.Ok));
                    var timer = Stopwatch.StartNew(); CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(30)) {
                        client.Tick(); state = client.Snapshot(); inventory.Observe(state);
                        while (client.PollEvent(out CoreEvent item)) {
                            inventory.Accept(item);
                            if (item.type == NativeClient.EventDiscardResponse) {
                                Assert.That(item.result, Is.EqualTo(CoreResult.Ok), item.Text);
                                Assert.That(item.argument0, Is.EqualTo(0)); acknowledged = true;
                            }
                        }
                        Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                        if (state.state == CoreState.CharacterListReady && !selected) {
                            Assert.That(client.SelectCharacter(111, 0, state), Is.EqualTo(CoreResult.Ok)); selected = true;
                        }
                        if (state.state == CoreState.InGame && inventory.Ready) {
                            bool present = inventory.Slots[0].ItemId == 53343;
                            if (attempt == 0 && !submitted) {
                                Assert.That(present, Is.True);
                                Assert.That(client.SubmitDiscardItem(0, state), Is.EqualTo(CoreResult.Ok)); submitted = true;
                            } else if (attempt == 0 && acknowledged && !present) { finished = true; break; }
                            else if (attempt == 1 && !present) { finished = true; break; }
                        }
                        Thread.Sleep(10);
                    }
                    Assert.That(finished, Is.True, "Discard acknowledgement, inventory refresh or reconnect persistence failed.");
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok)); Thread.Sleep(200);
                }
            }
        }

        [Test]
        public void RealQuestAcceptancePersistsAcrossReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_QUEST") != "1")
                Assert.Ignore("Requires unity_three_server_smoke.py --editor-test --quest.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            using (var client = new NativeClient()) {
                for (int attempt = 0; attempt < 2; ++attempt) {
                    bool selected = false, submitted = false, acknowledged = false, restored = false;
                    Assert.That(client.Connect("127.0.0.1", port,
                        Environment.GetEnvironmentVariable("MXH_SMOKE_USER"),
                        Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD")), Is.EqualTo(CoreResult.Ok));
                    var timer = Stopwatch.StartNew(); CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(30)) {
                        client.Tick(); state = client.Snapshot();
                        while (client.PollEvent(out CoreEvent item)) {
                            if (item.type != NativeClient.EventQuestUpdated || item.argument0 != 1) continue;
                            if (attempt == 0 && item.reserved0 == 10) {
                                Assert.That(item.argument1, Is.EqualTo(10)); acknowledged = true;
                            }
                            if (attempt == 1 && item.reserved0 == 0 && item.argument1 == 1) restored = true;
                        }
                        Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                        if (state.state == CoreState.CharacterListReady && !selected) {
                            Assert.That(client.SelectCharacter(111, 0, state), Is.EqualTo(CoreResult.Ok)); selected = true;
                        }
                        if (state.state == CoreState.InGame && attempt == 0 && !submitted) {
                            Assert.That(state.game.mapNumber, Is.EqualTo(10));
                            Assert.That(client.SubmitQuest(1, 9, state), Is.EqualTo(CoreResult.Ok)); submitted = true;
                        }
                        if ((attempt == 0 && acknowledged) || (attempt == 1 && restored)) break;
                        Thread.Sleep(10);
                    }
                    Assert.That(attempt == 0 ? acknowledged : restored, Is.True,
                        attempt == 0 ? "Quest StartAck was not observed." : "Persisted active quest TotalInfo was not observed after reconnect.");
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok)); Thread.Sleep(200);
                }
            }
        }

        [Test]
        public void RealQuestCompletionRewardAndReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_QUEST_REWARD") != "1")
                Assert.Ignore("Requires unity_three_server_smoke.py --editor-test --quest-reward.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            using (var client = new NativeClient()) {
                for (int attempt = 0; attempt < 2; ++attempt) {
                    var inventory = new InventoryState();
                    bool selected = false, completed = false, claimSubmitted = false, claimAck = false, restored = false;
                    long lastAttackMs = -1000;
                    Assert.That(client.Connect("127.0.0.1", port,
                        Environment.GetEnvironmentVariable("MXH_SMOKE_USER"),
                        Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD")), Is.EqualTo(CoreResult.Ok));
                    var timer = Stopwatch.StartNew(); CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(45)) {
                        client.Tick(); state = client.Snapshot(); inventory.Observe(state);
                        while (client.PollEvent(out CoreEvent item)) {
                            inventory.Accept(item);
                            if (item.type != NativeClient.EventQuestUpdated || item.argument0 != 173) continue;
                            if (item.reserved0 == 1 && item.argument1 == 2) completed = true;
                            if (item.reserved0 == 13 && item.argument1 == 13) claimAck = true;
                            if (attempt == 1 && item.reserved0 == 0 && item.argument1 == 3) restored = true;
                        }
                        Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                        if (state.state == CoreState.CharacterListReady && !selected) {
                            Assert.That(client.SelectCharacter(111, 0, state), Is.EqualTo(CoreResult.Ok)); selected = true;
                        }
                        if (state.state == CoreState.InGame && inventory.Ready) {
                            if (attempt == 0 && !completed && timer.ElapsedMilliseconds - lastAttackMs >= 750) {
                                Assert.That(client.SubmitSkill(1, 50023, 44178.5f, 13253.4f, state), Is.EqualTo(CoreResult.Ok));
                                lastAttackMs = timer.ElapsedMilliseconds;
                            }
                            if (attempt == 0 && completed && !claimSubmitted) {
                                Assert.That(client.SubmitQuest(173, 12, state), Is.EqualTo(CoreResult.Ok)); claimSubmitted = true;
                            }
                            bool rewardPresent = false;
                            foreach (var slot in inventory.Slots)
                                if (slot.ItemId == 414 && slot.ItemParameter == 30) rewardPresent = true;
                            if ((attempt == 0 && claimAck && rewardPresent) || (attempt == 1 && restored && rewardPresent)) break;
                        }
                        Thread.Sleep(10);
                    }
                    bool persistedReward = false;
                    foreach (var slot in inventory.Slots)
                        if (slot.ItemId == 414 && slot.ItemParameter == 30) persistedReward = true;
                    Assert.That(persistedReward, Is.True, "Quest reward item 414 x30 was not present.");
                    Assert.That(attempt == 0 ? claimAck : restored, Is.True,
                        attempt == 0 ? "Quest did not complete and return EndAck." : "Rewarded quest did not restore after reconnect.");
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok)); Thread.Sleep(200);
                }
            }
        }

        [Test]
        public void RealQuestNpcTalkAndReconnect()
        {
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_QUEST_NPC") != "1")
                Assert.Ignore("Requires unity_three_server_smoke.py --editor-test --quest-npc.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            using (var client = new NativeClient()) {
                for (int attempt = 0; attempt < 2; ++attempt) {
                    bool selected = false, submitted = false, acknowledged = false, restored = false;
                    Assert.That(client.Connect("127.0.0.1", port,
                        Environment.GetEnvironmentVariable("MXH_SMOKE_USER"),
                        Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD")), Is.EqualTo(CoreResult.Ok));
                    var timer = Stopwatch.StartNew(); CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(30)) {
                        client.Tick(); state = client.Snapshot();
                        while (client.PollEvent(out CoreEvent item)) {
                            if (item.type == NativeClient.EventQuestNpcResponse && item.argument0 == 572 && item.argument1 == 180)
                                acknowledged = item.result == CoreResult.Ok;
                            if (attempt == 1 && item.type == NativeClient.EventQuestUpdated && item.argument0 == 180 && item.argument1 == 1)
                                restored = true;
                        }
                        Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                        if (state.state == CoreState.CharacterListReady && !selected) {
                            Assert.That(client.SelectCharacter(111, 0, state), Is.EqualTo(CoreResult.Ok)); selected = true;
                        }
                        if (state.state == CoreState.InGame && attempt == 0 && !submitted) {
                            Assert.That(state.game.mapNumber, Is.EqualTo(10));
                            Assert.That(client.SubmitQuestNpcTalk(572, 180, state), Is.EqualTo(CoreResult.Ok)); submitted = true;
                        }
                        if ((attempt == 0 && acknowledged) || (attempt == 1 && restored)) break;
                        Thread.Sleep(10);
                    }
                    Assert.That(attempt == 0 ? acknowledged : restored, Is.True,
                        attempt == 0 ? "Quest NpcTalk Ack was not observed." : "Quest 180 did not restore after reconnect.");
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok)); Thread.Sleep(200);
                }
            }
        }
    }
}
