using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class TradePanelTests
    {
        [Test]
        public void UseOnlyAcceptsNonemptyCarriedInventorySlots()
        {
            Assert.That(TradePanel.IsUsableInventorySlot(0, Item(321)), Is.True);
            Assert.That(TradePanel.IsUsableInventorySlot(79, Item(321)), Is.True);
            Assert.That(TradePanel.IsUsableInventorySlot(-1, Item(321)), Is.False);
            Assert.That(TradePanel.IsUsableInventorySlot(80, Item(321)), Is.False,
                "Worn equipment is not an ordinary UseSyn inventory target.");
            Assert.That(TradePanel.IsUsableInventorySlot(5, Item(0)), Is.False);
        }

        [Test]
        public void EquipmentMoveUsesOnlyInventoryAndWornPositionSpace()
        {
            Assert.That(TradePanel.IsMoveSource(0, Item(321)), Is.True);
            Assert.That(TradePanel.IsMoveSource(89, Item(321)), Is.True);
            Assert.That(TradePanel.IsMoveSource(90, Item(321)), Is.False);
            Assert.That(TradePanel.IsMoveSource(5, Item(0)), Is.False);
            Assert.That(TradePanel.IsMoveTarget(5, 80), Is.True);
            Assert.That(TradePanel.IsMoveTarget(80, 5), Is.True);
            Assert.That(TradePanel.IsMoveTarget(5, 5), Is.False);
            Assert.That(TradePanel.IsMoveTarget(5, 90), Is.False);
        }

        [Test]
        public void SellUsesOnlyNonemptyCarriedInventorySlots()
        {
            Assert.That(TradePanel.IsSellableInventorySlot(0, Item(321)), Is.True);
            Assert.That(TradePanel.IsSellableInventorySlot(79, Item(321)), Is.True);
            Assert.That(TradePanel.IsSellableInventorySlot(80, Item(321)), Is.False);
            Assert.That(TradePanel.IsSellableInventorySlot(5, Item(0)), Is.False);
        }

        [Test]
        public void DiscardUsesOnlyNonemptyCarriedInventorySlots()
        {
            Assert.That(TradePanel.IsDiscardableInventorySlot(0, Item(321)), Is.True);
            Assert.That(TradePanel.IsDiscardableInventorySlot(79, Item(321)), Is.True);
            Assert.That(TradePanel.IsDiscardableInventorySlot(80, Item(321)), Is.False);
            Assert.That(TradePanel.IsDiscardableInventorySlot(5, Item(0)), Is.False);
        }

        private static InventoryItem Item(ushort itemId)
        {
            var bytes = new byte[22];
            bytes[4] = (byte)itemId;
            bytes[5] = (byte)(itemId >> 8);
            return new InventoryItem(bytes, 0);
        }
    }
}
