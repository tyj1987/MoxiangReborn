// mxh/include/mxh/server/npc_shop.hpp
// Phase 13.4: 1:1 port of the NPC-shop data plane that legacy
// [Server]Map/ShopItemManager.cpp + MapDBShopList.cpp expose through
// CShopItemManager::ItemBuyAsk + the per-NPC catalog table.
//
// This header ships the *data plane* only: pure-function decision
// logic that takes a per-NPC catalog + the wire-format BuySyn
// request and returns a typed decision (BuyAck / BuyNack reason +
// post-state).  The orchestrator (MapHandler) applies the decision
// to its player inventory + money + wire broadcast under its own
// critical section.  Splitting data plane from orchestrator mirrors
// every other 1:1 port in this tree (quest_manager / add_using_shop
// item / skin_select_transition / etc.) and lets the decision be
// unit-tested without spinning up a server.
//
// Wire layout (BuySyn, 4B):
//   [0..2) item_id  (u16 LE)
//   [2..4) qty      (u16 LE)
//
// 1:1 quirks preserved:
//   * item_id == 0  is rejected (legacy reserved sentinel).
//   * qty      == 0 is rejected (legacy zero-qty guard).
//   * price    *  qty overflow is detected before subtraction
//     (32-bit money would silently wrap otherwise).
//   * stock    == 0 means "unlimited"; otherwise the qty must not
//     exceed the catalog stock.

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace mxh::server {

// ---- Buy decision outcome (legacy ItemBuyAsk return codes) ----
enum class NpcShopBuyStatus : std::uint8_t {
    Ok                = 0,  // accepted; orchestrator applies inventory+money mutation
    UnknownItem       = 1,  // item_id not in this NPC's catalog
    InsufficientFunds = 2,  // total_price > player_money
    InvalidQty        = 3,  // qty == 0
    InvalidItem       = 4,  // item_id == 0
    OutOfStock        = 5,  // qty > entry.stock (and stock != 0 = unlimited)
    NpcMismatch       = 6,  // catalog.npc_id != request.npc_id
};

// ---- One catalog row (per NPC x per item) ----
struct NpcShopEntry final {
    std::uint32_t item_id = 0;
    std::uint32_t price   = 0;  // per-unit price
    std::uint16_t stock   = 0;  // 0 = unlimited
};

// ---- Per-NPC catalog ----
struct NpcShopCatalog final {
    std::uint32_t            npc_id = 0;
    std::vector<NpcShopEntry> entries;

    // Find the catalog row for an item, or nullopt if not sold by
    // this NPC.  Linear search; the dialog tables in PlayDH are
    // small (tens of rows per NPC) so a hash map is not warranted.
    std::optional<NpcShopEntry>
    find(std::uint32_t item_id) const noexcept {
        for (const auto& e : entries) {
            if (e.item_id == item_id) return e;
        }
        return std::nullopt;
    }
};

// ---- Parsed BuySyn payload ----
struct NpcShopBuyRequest final {
    std::uint32_t npc_id       = 0;
    std::uint16_t item_id      = 0;
    std::uint16_t qty          = 0;
    std::uint32_t player_money = 0;
};

// ---- Decision returned by npc_shop_buy_decision ----
struct NpcShopBuyDecision final {
    NpcShopBuyStatus status      = NpcShopBuyStatus::InvalidQty;
    std::uint32_t    total_price = 0;  // filled on Ok (entry.price * qty)
    std::uint32_t    new_money   = 0;  // filled on Ok (player_money - total)
};

// ---- Pure-function decision ----
//
// Returns the typed outcome of an attempted NPC purchase.  The
// orchestrator maps the decision to MP_ITEM_BUY_ACK / _NACK on the
// wire and, on Ok, mutates the player's inventory + money + the
// NPC's stock under its critical section.
NpcShopBuyDecision npc_shop_buy_decision(
    const NpcShopCatalog& catalog,
    const NpcShopBuyRequest& req) noexcept;

// ---- Wire-format helper ----
//
// Parses the 4B BuySyn payload into a request the orchestrator can
// pass straight to npc_shop_buy_decision.  Returns nullopt if the
// payload is shorter than 4 bytes.
std::optional<NpcShopBuyRequest>
parse_npc_shop_buy_request(std::uint32_t npc_id,
                           std::span<const std::uint8_t> payload) noexcept;

}  // namespace mxh::server
