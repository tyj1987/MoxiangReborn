// mxh/services/IItemShopService.hpp
// Phase 13.3 service interface for NPC shop dialog (cItemShopDialog).
//
// Tier 3 dialog (cItemShopDialog, cItemShopGridDialog) currently
// reads shop catalog + player money via legacy singletons (ITEMMGR,
// GameIn->GetCharacterDialog()->GetMoney()). This service is the
// modern replacement: dialog code takes an IItemShopService* and
// queries the NPC catalog + player economy through it.
//
// The interface is read-only for the catalog (the catalog itself is
// authored in DB / static table; the dialog only displays it) and
// exposes the player money query needed by cItemShopDialog::Buy
// (the dialog needs to verify the player can afford the item
// before sending MP_ITEM_BUY_SYN to the map server).
//
// Write paths (commit purchase, refund, restock) are out of scope;
// they belong to the network layer that mediates with the map server.
// cItemShopDialog keeps its existing PurchaseCallback as the host
// dispatch for MP_ITEM_BUY_SYN -- the service is only consulted for
// read-side validation.
//
// Usage pattern (from a future cItemShopDialog::Buy):
//   bool cItemShopDialog::Buy(std::size_t i, std::uint16_t qty) {
//     const auto* svc = m_shopService;
//     if (svc) {
//       if (i >= svc->shopEntryCount()) return false;
//       auto entry = svc->getShopEntry(i);
//       if (!entry) return false;
//       std::uint32_t total = entry->price * qty;
//       if (!svc->hasEnoughMoney(total)) return false;
//     }
//     // ... dispatch MP_ITEM_BUY_SYN via PurchaseCallback
//   }

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace mxh::services {

// One NPC shop entry (item for sale + price + per-stack qty).
// Mirrors the legacy shop item table layout (item wIconIdx +
// dwBuyPrice + wItemCount). The struct is intentionally trivial;
// the dialog renders the row and the service answers money/catalog
// queries.
struct ShopEntry {
    std::uint16_t item_id  = 0;
    std::uint32_t price    = 0;
    std::uint16_t quantity = 1;
};

class IItemShopService {
public:
    virtual ~IItemShopService() = default;

    // ----- Catalog (read) -----

    // Number of entries in the NPC's shop catalog. Must be in
    // sync with getShopEntry() (returning a non-nullopt for
    // any i in [0, shopEntryCount())).
    virtual std::size_t shopEntryCount() const noexcept = 0;

    // Return the catalog entry at index i, or std::nullopt if
    // i is out of range. Implementation-defined ordering; the
    // dialog does not rely on a specific tab order from this
    // interface (tab ordering is dialog-local state).
    virtual std::optional<ShopEntry> getShopEntry(std::size_t i) const noexcept = 0;

    // ----- Player economy (read) -----

    // Current gold (money) held by the local player. Matches
    // the legacy GameIn->GetCharacterDialog()->GetMoney() value.
    virtual std::uint32_t playerMoney() const noexcept = 0;

    // True iff playerMoney() >= amount. Convenience for the
    // dialog's purchase validation; equivalent to
    // (playerMoney() >= amount) but expresses intent better at
    // the call site and lets impls use a snapshot if needed.
    virtual bool hasEnoughMoney(std::uint32_t amount) const noexcept = 0;
};

}  // namespace mxh::services
