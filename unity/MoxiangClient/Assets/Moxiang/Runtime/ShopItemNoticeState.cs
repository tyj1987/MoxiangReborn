using System;

namespace Moxiang
{
    public enum ShopItemNoticeKind
    {
        None,
        OneMinute,
        UseEnd,
        Protection
    }

    /// <summary>Keeps only a notification belonging to the current admitted session and map.</summary>
    public sealed class ShopItemNoticeState
    {
        public ShopItemNoticeKind Kind { get; private set; }
        public ushort ItemId { get; private set; }
        public string Message { get; private set; }
        private CoreSnapshot observed;
        private ulong sequence;

        public void Clear()
        {
            Kind = ShopItemNoticeKind.None;
            ItemId = 0;
            Message = null;
            sequence = 0;
        }

        public void Observe(CoreSnapshot snapshot)
        {
            if (snapshot.state != CoreState.InGame || snapshot.sessionGeneration != observed.sessionGeneration ||
                snapshot.mapGeneration != observed.mapGeneration || snapshot.game.playerId != observed.game.playerId)
                Clear();
            observed = snapshot;
        }

        public bool Accept(CoreEvent e)
        {
            bool useEnd = e.type == NativeClient.EventShopItemUseEnd;
            bool oneMinute = e.type == NativeClient.EventShopItemOneMinute;
            if (e.type == NativeClient.EventShopProtection)
            {
                bool validValue = (e.reserved0 == 109 && e.argument0 == 55311) ||
                    (e.reserved0 == 110 && e.argument0 == 55312) ||
                    (e.reserved0 == 150 && e.argument0 <= 127);
                if (!validValue || observed.state != CoreState.InGame || e.state != CoreState.InGame ||
                    e.sessionGeneration != observed.sessionGeneration || e.mapGeneration != observed.mapGeneration ||
                    e.sequence <= sequence || e.result != CoreResult.Ok || e.argument1 != 0 || e.textLength != 0)
                    return false;
                sequence = e.sequence;
                Kind = ShopItemNoticeKind.Protection;
                ItemId = e.reserved0 == 150 ? (ushort)0 : (ushort)e.argument0;
                Message = e.reserved0 == 109 ? "金钱保护已使用。" : e.reserved0 == 110
                    ? "经验保护已使用。" : "综合保护剩余 " + e.argument0 + " 次。";
                return true;
            }
            if (!useEnd && !oneMinute) return false;
            uint protocol = useEnd ? NativeClient.ProtocolShopItemUseEnd : NativeClient.ProtocolShopItemOneMinute;
            if (observed.state != CoreState.InGame || e.state != CoreState.InGame ||
                e.sessionGeneration != observed.sessionGeneration || e.mapGeneration != observed.mapGeneration ||
                e.sequence <= sequence || e.result != CoreResult.Ok || e.argument0 == 0 || e.argument0 > ushort.MaxValue ||
                e.argument1 != 0 || e.textLength != 0 || e.reserved0 != protocol)
                return false;

            sequence = e.sequence;
            ItemId = (ushort)e.argument0;
            Kind = useEnd ? ShopItemNoticeKind.UseEnd : ShopItemNoticeKind.OneMinute;
            Message = useEnd
                ? "商城道具 " + ItemId + " 已到期。"
                : "商城道具 " + ItemId + " 将在 1 分钟内到期。";
            return true;
        }
    }
}
