using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class ShopAppearanceStateTests
    {
        private static CoreSnapshot Snapshot() => new CoreSnapshot {
            state = CoreState.InGame, sessionGeneration = 7, mapGeneration = 2,
            game = new CoreGame { playerId = 42 }
        };

        private static CoreEvent Event()
        {
            var e = new CoreEvent { type = NativeClient.EventShopAppearance, state = CoreState.InGame,
                result = CoreResult.Ok, sessionGeneration = 7, mapGeneration = 2, sequence = 10,
                argument0 = 42, argument1 = 1, textLength = 120, text = new byte[256] };
            for (int i = 0; i < 120; ++i) e.text[i] = (byte)(i * 17);
            return e;
        }

        [Test]
        public void PreservesWireBoundariesAndOwnsItsData()
        {
            var state = new ShopAppearanceState(); state.Observe(Snapshot()); var e = Event();
            Assert.That(state.Accept(e), Is.True);
            Assert.That(state.Avatar.Count, Is.EqualTo(23)); Assert.That(state.Skin.Count, Is.EqualTo(5));
            Assert.That(state.Avatar[22], Is.EqualTo(e.text[44] | e.text[45] << 8));
            Assert.That(state.Skin[0], Is.EqualTo(e.text[106] | e.text[107] << 8));
            Assert.That(state.Skin[4], Is.EqualTo(e.text[114] | e.text[115] << 8));
            Assert.That(state.StreetStallDecoration, Is.EqualTo((uint)e.text[116] | (uint)e.text[117] << 8 |
                (uint)e.text[118] << 16 | (uint)e.text[119] << 24));
            var raw = state.Raw; e.text[0] = 255; Assert.That(raw[0], Is.Zero);
            state.Clear(); Assert.That(state.Received, Is.False); Assert.That(raw.Count, Is.EqualTo(120));
        }

        [Test]
        public void RejectsStaleIdentityAndClearsMalformedOrChangedGeneration()
        {
            var state = new ShopAppearanceState(); var snapshot = Snapshot(); state.Observe(snapshot);
            Assert.That(state.Accept(Event()), Is.True);
            var e = Event(); e.sequence = 9; Assert.That(state.Accept(e), Is.False);
            e.sequence = 11; e.argument0 = 99; Assert.That(state.Accept(e), Is.False);
            e.argument0 = 42; e.mapGeneration = 1; Assert.That(state.Accept(e), Is.False);
            e.mapGeneration = 2; e.textLength = 124; Assert.That(state.Accept(e), Is.False);
            Assert.That(state.Received, Is.False);
            Assert.That(state.Accept(Event()), Is.False, "A rejected newer packet must not allow stale replay");
            e.sequence = 12; e.textLength = 120; Assert.That(state.Accept(e), Is.True);
            snapshot.mapGeneration++; state.Observe(snapshot); Assert.That(state.Received, Is.False);
            Assert.That(state.Accept(e), Is.False);
        }
    }
}
