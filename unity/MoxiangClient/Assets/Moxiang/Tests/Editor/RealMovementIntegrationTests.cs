using System;
using System.Diagnostics;
using System.Threading;
using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class RealMovementIntegrationTests
    {
        private static CoreSnapshot Enter(NativeClient client, ushort port, string account, string password)
        {
            Assert.That(client.Connect("127.0.0.1", port, account, password), Is.EqualTo(CoreResult.Ok));
            var timer = Stopwatch.StartNew();
            while (timer.Elapsed < TimeSpan.FromSeconds(20))
            {
                client.Tick(); var state = client.Snapshot();
                while (client.PollEvent(out _)) { }
                Assert.That(state.state, Is.Not.EqualTo(CoreState.Failed), state.Error);
                if (state.state == CoreState.CharacterListReady)
                {
                    Assert.That(state.characterCount, Is.EqualTo(1));
                    Assert.That(client.SelectCharacter(state.characters[0].characterId, 0, state), Is.EqualTo(CoreResult.Ok));
                }
                if (state.state == CoreState.InGame) return state;
                Thread.Sleep(10);
            }
            Assert.Fail("Real server entry timed out."); return default;
        }

        private static CoreEvent WaitFor(NativeClient sender, NativeClient observer, uint type, uint objectId, uint packed, uint protocol)
        {
            var timer = Stopwatch.StartNew();
            while (timer.Elapsed < TimeSpan.FromSeconds(5))
            {
                sender.Tick(); observer.Tick();
                Assert.That(sender.Snapshot().state, Is.EqualTo(CoreState.InGame), sender.Snapshot().Error);
                Assert.That(observer.Snapshot().state, Is.EqualTo(CoreState.InGame), observer.Snapshot().Error);
                while (observer.PollEvent(out var item))
                    if (item.type == type && item.argument0 == objectId)
                    {
                        Assert.That(item.argument1, Is.EqualTo(packed));
                        Assert.That(item.reserved0, Is.EqualTo(protocol));
                        return item;
                    }
                Thread.Sleep(10);
            }
            Assert.Fail("Expected real movement event timed out."); return default;
        }

        [Test]
        public void RealTwoClientsObserveMoveStopAndRejectedJump()
        {
            string observerAccount = Environment.GetEnvironmentVariable("MXH_SMOKE_OBSERVER_USER");
            if (string.IsNullOrEmpty(observerAccount)) Assert.Ignore("Requires --movement isolated real two-account fixture.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            string password = Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD");
            using (var mover = new NativeClient())
            using (var observer = new NativeClient())
            {
                var initial = Enter(mover, port, Environment.GetEnvironmentVariable("MXH_SMOKE_USER"), password);
                var other = Enter(observer, port, observerAccount, password);
                Assert.That(initial.game.playerId, Is.Not.EqualTo(other.game.playerId));
                Assert.That(initial.game.mapNumber, Is.EqualTo(other.game.mapNumber));
                ushort x = checked((ushort)(initial.game.positionX + 64));
                ushort z = checked((ushort)(initial.game.positionZ + 32));
                uint packed = x | ((uint)z << 16);
                Assert.That(mover.Move(x, z, false, initial), Is.EqualTo(CoreResult.Ok));
                var moved = WaitFor(mover, observer, 9, initial.game.playerId, packed, 13);
                Assert.That(moved.sessionGeneration, Is.EqualTo(other.sessionGeneration));
                Assert.That(moved.mapGeneration, Is.EqualTo(other.mapGeneration));
                Assert.That(mover.Move(x, z, true, mover.Snapshot()), Is.EqualTo(CoreResult.Ok));
                var stopped = WaitFor(mover, observer, 9, initial.game.playerId, packed, 8);
                Assert.That(stopped.sequence, Is.GreaterThan(moved.sequence));
                while (mover.PollEvent(out _)) { }
                // The existing modern server rejects a >5000-unit target jump.
                Assert.That(mover.Move(100, 100, false, mover.Snapshot()), Is.EqualTo(CoreResult.Ok));
                var corrected = WaitFor(observer, mover, 8, initial.game.playerId, packed, 2);
                Assert.That(corrected.requestId, Is.Zero, "Legacy corrections have no request ID.");
                Assert.That(mover.Snapshot().game.positionX, Is.EqualTo(x));
                Assert.That(mover.Snapshot().game.positionZ, Is.EqualTo(z));
                ushort blockedX = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_BLOCKED_X"));
                ushort blockedZ = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_BLOCKED_Z"));
                long dx = (long)blockedX - x, dz = (long)blockedZ - z;
                Assert.That(dx * dx + dz * dz, Is.LessThan(5000L * 5000L));
                Assert.That(mover.Move(blockedX, blockedZ, false, mover.Snapshot()), Is.EqualTo(CoreResult.Ok));
                WaitFor(observer, mover, 8, initial.game.playerId, packed, 2);
                Assert.That(mover.Snapshot().game.positionX, Is.EqualTo(x));
                Assert.That(mover.Snapshot().game.positionZ, Is.EqualTo(z));
                var quiet = Stopwatch.StartNew();
                while (quiet.Elapsed < TimeSpan.FromMilliseconds(400))
                {
                    mover.Tick(); observer.Tick();
                    while (observer.PollEvent(out var item))
                        Assert.That(item.type == 9 && item.argument0 == initial.game.playerId, Is.False,
                            "Rejected movement must not be broadcast to the other player.");
                    Thread.Sleep(10);
                }
                Assert.That(mover.Move(x, z, true, initial), Is.EqualTo(CoreResult.Ok));
                WaitFor(mover, observer, 9, initial.game.playerId, packed, 8);
            }
        }

        [Test]
        public void RealTwoClientsSeeExactUtf8ChatAndSenderLocalEcho()
        {
            string observerAccount = Environment.GetEnvironmentVariable("MXH_SMOKE_OBSERVER_USER");
            if (string.IsNullOrEmpty(observerAccount)) Assert.Ignore("Requires --movement isolated real two-account fixture.");
            ushort port = ushort.Parse(Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT"));
            string password = Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD");
            using (var sender = new NativeClient())
            using (var observer = new NativeClient())
            {
                var owner = Enter(sender, port, Environment.GetEnvironmentVariable("MXH_SMOKE_USER"), password);
                Enter(observer, port, observerAccount, password);
                const string text = "墨香 Map10 双人聊天";
                Assert.That(sender.SubmitChat(text, owner), Is.EqualTo(CoreResult.Ok));
                bool local = false, remote = false;
                var timer = Stopwatch.StartNew();
                while (timer.Elapsed < TimeSpan.FromSeconds(5) && (!local || !remote))
                {
                    sender.Tick(); observer.Tick();
                    while (sender.PollEvent(out var item))
                        if (item.type == NativeClient.EventChatMessage && item.argument0 == owner.game.playerId) {
                            Assert.That(item.Text, Is.EqualTo(text)); Assert.That(item.requestId, Is.GreaterThan(0)); local = true;
                        }
                    while (observer.PollEvent(out var item))
                        if (item.type == NativeClient.EventChatMessage && item.argument0 == owner.game.playerId) {
                            Assert.That(item.Text, Is.EqualTo(text)); Assert.That(item.requestId, Is.Zero); remote = true;
                        }
                    Thread.Sleep(10);
                }
                Assert.That(local, Is.True, "Sender did not receive its original-client-style local echo.");
                Assert.That(remote, Is.True, "Second client did not receive the server-routed UTF-8 chat.");
            }
        }
    }
}
