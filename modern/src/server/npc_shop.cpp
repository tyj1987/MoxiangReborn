// mxh/src/server/npc_shop.cpp - implementation of the NPC-shop data
// plane declared in include/mxh/server/npc_shop.hpp.

#include "mxh/server/npc_shop.hpp"

#include <algorithm>
#include <cstring>

namespace mxh::server {

NpcShopBuyDecision npc_shop_buy_decision(
    const NpcShopCatalog& catalog,
    const NpcShopBuyRequest& req) noexcept {
    NpcShopBuyDecision out;

    // 1:1 quirk: legacy CShopItemManager::ItemBuyAsk rejects
    // mismatched NPC ids with NotForSale so the client gets a
    // descriptive Nack instead of silently mutating state.
    if (catalog.npc_id != req.npc_id) {
        out.status = NpcShopBuyStatus::NpcMismatch;
        return out;
    }

    // 1:1 quirk: item_id == 0 is reserved; the legacy guard rejects
    // before any catalog lookup.
    if (req.item_id == 0u) {
        out.status = NpcShopBuyStatus::InvalidItem;
        return out;
    }

    // 1:1 quirk: qty == 0 is rejected (the legacy guard returns
    // before the price multiplication; this also avoids the
    // overflow path below).
    if (req.qty == 0u) {
        out.status = NpcShopBuyStatus::InvalidQty;
        return out;
    }

    const auto entry_opt = catalog.find(req.item_id);
    if (!entry_opt.has_value()) {
        out.status = NpcShopBuyStatus::UnknownItem;
        return out;
    }
    const auto& entry = entry_opt.value();

    // Stock check (1:1 quirk: stock == 0 means "unlimited" in the
    // legacy table; only finite-stock entries enforce the cap).
    if (entry.stock != 0u && req.qty > entry.stock) {
        out.status = NpcShopBuyStatus::OutOfStock;
        return out;
    }

    // Overflow guard: price * qty must fit in u32 money.  The
    // legacy code assumes a 32-bit money field and would silently
    // wrap; we reject the request explicitly.
    const auto total = static_cast<std::uint64_t>(entry.price) * req.qty;
    if (total > std::numeric_limits<std::uint32_t>::max()) {
        out.status = NpcShopBuyStatus::InvalidQty;
        return out;
    }
    const auto total_32 = static_cast<std::uint32_t>(total);

    if (total_32 > req.player_money) {
        out.status = NpcShopBuyStatus::InsufficientFunds;
        return out;
    }

    out.status      = NpcShopBuyStatus::Ok;
    out.total_price = total_32;
    out.new_money   = req.player_money - total_32;
    return out;
}

std::optional<NpcShopBuyRequest>
parse_npc_shop_buy_request(std::uint32_t npc_id,
                           std::span<const std::uint8_t> payload) noexcept {
    if (payload.size() < 4u) return std::nullopt;
    NpcShopBuyRequest out;
    out.npc_id       = npc_id;
    std::uint16_t v16 = 0;
    std::memcpy(&v16, payload.data(),     sizeof(v16));
    out.item_id = v16;
    std::memcpy(&v16, payload.data() + 2, sizeof(v16));
    out.qty = v16;
    // player_money is filled by the orchestrator (it owns the
    // authoritative Player state).  Default 0; the orchestrator
    // overwrites before calling npc_shop_buy_decision.
    out.player_money = 0u;
    return out;
}

}  // namespace mxh::server
