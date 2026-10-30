using System;
using System.Collections.Generic;

namespace Moxiang
{
    public readonly struct ShopOffer
    {
        public readonly ushort ItemId;
        public readonly uint Price;
        public ShopOffer(ushort itemId, uint price) { ItemId = itemId; Price = price; }
    }

    // Publish an immutable view only after all ordered native chunks arrive.
    public sealed class ShopCatalog
    {
        public IReadOnlyList<ShopOffer> Offers { get; private set; } = Array.Empty<ShopOffer>();
        public uint NpcId { get; private set; }
        public bool Ready { get; private set; }
        private ShopOffer[] pending;
        private uint pendingNpc;
        private int received;
        private ulong session, map;
        private bool inGame;

        public void Clear()
        {
            Offers = Array.Empty<ShopOffer>(); NpcId = 0; Ready = false;
            pending = null; pendingNpc = 0; received = 0;
        }

        public void Observe(CoreSnapshot snapshot)
        {
            bool active = snapshot.state == CoreState.InGame;
            if (!active || session != snapshot.sessionGeneration || map != snapshot.mapGeneration) Clear();
            session = snapshot.sessionGeneration; map = snapshot.mapGeneration; inGame = active;
        }

        public bool Accept(CoreEvent e)
        {
            if (!inGame || e.state != CoreState.InGame || e.sessionGeneration != session ||
                e.mapGeneration != map || e.type != NativeClient.EventShopCatalog) return false;
            uint total = e.argument1, offset = e.reserved0;
            if (e.result != CoreResult.Ok || total > ushort.MaxValue || offset > total ||
                e.textLength > 240 || e.textLength % 6 != 0 ||
                (e.textLength != 0 && (e.text == null || e.text.Length < e.textLength)) ||
                (total != 0 && e.argument0 == 0)) { Clear(); return false; }
            uint count = e.textLength / 6;
            if (count != Math.Min(40u, total - offset) || (total != 0 && count == 0))
            { Clear(); return false; }
            if (offset == 0) {
                Clear(); pending = new ShopOffer[(int)total]; pendingNpc = e.argument0;
            }
            if (pending == null || pending.Length != total || pendingNpc != e.argument0 || received != offset)
            { Clear(); return false; }
            for (int i = 0; i < count; ++i) {
                int p = i * 6;
                ushort item = (ushort)(e.text[p] | e.text[p + 1] << 8);
                uint price = (uint)e.text[p + 2] | (uint)e.text[p + 3] << 8 |
                    (uint)e.text[p + 4] << 16 | (uint)e.text[p + 5] << 24;
                pending[received++] = new ShopOffer(item, price);
            }
            if (received != pending.Length) return false;
            Offers = Array.AsReadOnly(pending); NpcId = pendingNpc; Ready = true;
            pending = null; received = 0; return true;
        }
    }
}
