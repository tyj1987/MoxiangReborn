using System;
using System.Collections.Generic;

namespace Moxiang
{
    public readonly struct InventoryItem
    {
        public readonly uint DatabaseId, Durability, RareId, ItemParameter;
        public readonly ushort ItemId, Position, QuickPosition;
        private static ushort U16(byte[] bytes, int p) => (ushort)(bytes[p] | bytes[p + 1] << 8);
        private static uint U32(byte[] bytes, int p) => (uint)bytes[p] | (uint)bytes[p + 1] << 8 |
            (uint)bytes[p + 2] << 16 | (uint)bytes[p + 3] << 24;
        public InventoryItem(byte[] bytes, int p)
        {
            DatabaseId = U32(bytes, p); ItemId = U16(bytes, p + 4); Position = U16(bytes, p + 6);
            Durability = U32(bytes, p + 8); RareId = U32(bytes, p + 12);
            QuickPosition = U16(bytes, p + 16); ItemParameter = U32(bytes, p + 18);
        }
    }

    public sealed class InventoryState
    {
        // Legacy order: 80 inventory, 10 worn, 20 shop, 3 pet, 7 titan, 4 titan shop.
        public const int CarriedSlotCount = 80;
        public const int SlotCount = 124;
        public IReadOnlyList<InventoryItem> Slots { get; private set; } = Array.Empty<InventoryItem>();
        public bool Ready { get; private set; }
        private InventoryItem[] pending;
        private int received;
        private CoreSnapshot observed;
        public bool Matches(CoreSnapshot snapshot) => Ready && snapshot.state == CoreState.InGame &&
            observed.state == CoreState.InGame && snapshot.sessionGeneration == observed.sessionGeneration &&
            snapshot.mapGeneration == observed.mapGeneration && snapshot.game.playerId == observed.game.playerId;
        public void Clear() { Slots = Array.Empty<InventoryItem>(); Ready = false; pending = null; received = 0; }
        public void Observe(CoreSnapshot snapshot)
        {
            if (snapshot.state != CoreState.InGame || snapshot.sessionGeneration != observed.sessionGeneration ||
                snapshot.mapGeneration != observed.mapGeneration || snapshot.game.playerId != observed.game.playerId) Clear();
            observed = snapshot;
        }
        public bool Accept(CoreEvent e)
        {
            if (e.type != NativeClient.EventInventory || observed.state != CoreState.InGame ||
                e.state != CoreState.InGame || e.sessionGeneration != observed.sessionGeneration ||
                e.mapGeneration != observed.mapGeneration) return false;
            uint offset = e.reserved0;
            if (e.result != CoreResult.Ok || e.argument0 != observed.game.playerId || e.argument1 != SlotCount ||
                offset >= SlotCount || e.text == null || e.textLength > e.text.Length ||
                e.textLength != Math.Min(11u, SlotCount - offset) * 22) { Clear(); return false; }
            if (offset == 0) { Clear(); pending = new InventoryItem[SlotCount]; }
            if (pending == null || received != offset) { Clear(); return false; }
            for (int p = 0; p < e.textLength; p += 22) pending[received++] = new InventoryItem(e.text, p);
            if (received != SlotCount) return false;
            Slots = Array.AsReadOnly(pending); pending = null; received = 0; Ready = true; return true;
        }
    }
}
