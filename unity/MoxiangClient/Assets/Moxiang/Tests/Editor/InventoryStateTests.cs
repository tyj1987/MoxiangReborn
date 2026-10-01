using NUnit.Framework;

namespace Moxiang.Tests
{
    public class InventoryStateTests
    {
        private static CoreSnapshot Snapshot(ulong map = 1) => new CoreSnapshot {
            state = CoreState.InGame, sessionGeneration = 1, mapGeneration = map,
            game = new CoreGame { playerId = 7001 } };
        private static CoreEvent Chunk(uint offset) {
            uint count = System.Math.Min(11u, 124u - offset);
            var bytes = new byte[count * 22];
            for (int i = 0; i < bytes.Length; ++i) bytes[i] = 255;
            return new CoreEvent { type = NativeClient.EventInventory, state = CoreState.InGame,
                sessionGeneration = 1, mapGeneration = 1, argument0 = 7001, argument1 = 124,
                reserved0 = offset, text = bytes, textLength = (uint)bytes.Length };
        }
        [Test] public void AllSlotsAndFieldsPublishTogether() {
            var inventory = new InventoryState(); inventory.Observe(Snapshot());
            for (uint offset = 0; offset < 121; offset += 11) {
                Assert.That(inventory.Accept(Chunk(offset)), Is.False);
                Assert.That(inventory.Slots, Is.Empty);
            }
            Assert.That(inventory.Accept(Chunk(121)), Is.True);
            Assert.That(inventory.Slots.Count, Is.EqualTo(124));
            var last = inventory.Slots[123];
            Assert.That(last.DatabaseId, Is.EqualTo(uint.MaxValue));
            Assert.That(last.ItemId, Is.EqualTo(ushort.MaxValue));
            Assert.That(last.Position, Is.EqualTo(ushort.MaxValue));
            Assert.That(last.Durability, Is.EqualTo(uint.MaxValue));
            Assert.That(last.RareId, Is.EqualTo(uint.MaxValue));
            Assert.That(last.QuickPosition, Is.EqualTo(ushort.MaxValue));
            Assert.That(last.ItemParameter, Is.EqualTo(uint.MaxValue));
        }
        [Test] public void MissingChunkAndMapTransitionDiscardPartialInventory() {
            var inventory = new InventoryState(); inventory.Observe(Snapshot());
            inventory.Accept(Chunk(0)); Assert.That(inventory.Accept(Chunk(22)), Is.False);
            Assert.That(inventory.Ready, Is.False);
            inventory.Accept(Chunk(0)); inventory.Observe(Snapshot(2));
            Assert.That(inventory.Accept(Chunk(11)), Is.False); Assert.That(inventory.Slots, Is.Empty);
        }
    }
}
