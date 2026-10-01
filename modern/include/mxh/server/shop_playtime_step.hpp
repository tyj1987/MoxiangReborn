#pragma once
#include <mxh/game/shop_item_types.hpp>
#include <mxh/game/item_list_types.hpp>
#include <mxh/server/calc_shop_item_option.hpp>
#include <bit>

namespace mxh::server {
struct ShopPlaytimeStep {
    game::ShopItemWithTime next;
    bool handled=false;
    bool expired=false;
    bool persist_remaining=false;
    bool notify_one_minute=false;
};

// Source CheckEndTime:1063-1117. Pure candidate calculation: caller must
// perform option removal, duplicate-counter update, persistence and notification
// before publishing an expired candidate. Param=10 is not the timer type.
inline ShopPlaytimeStep plan_shop_playtime_step(const game::ShopItemWithTime& current,
    const game::ItemInfo* info,const CalcShopItemOptionEnv& rates,
    std::uint32_t now,bool flush_due) noexcept {
    ShopPlaytimeStep out{current};
    if(!info || info->SellPrice!=game::SHOP_ITEM_PARAM_PLAY_TIME) return out;
    out.handled=true;
    const auto remaining=current.ShopItem.Remaintime;
    if(info->ItemKind==258 && info->MeleeAttackMin) {
        if(!remaining) return out;
        if(!rates.event_rate_active(info->MeleeAttackMin)) {
            out.next.LastCheckTime=now;
            return out;
        }
    }
    // DWORD subtraction followed by signed int in the original x86 source.
    const auto checksum=std::bit_cast<std::int32_t>(remaining-(now-current.LastCheckTime));
    if(checksum<=0) {
        out.next.ShopItem.Remaintime=0;
        out.expired=true;
    } else if(remaining) {
        out.next.ShopItem.Remaintime=static_cast<std::uint32_t>(checksum);
        out.next.LastCheckTime=now;
    }
    out.persist_remaining=flush_due && checksum>0;
    out.notify_one_minute=std::bit_cast<std::int32_t>(remaining)>60000 && out.next.ShopItem.Remaintime<=60000;
    return out;
}
} // namespace mxh::server
