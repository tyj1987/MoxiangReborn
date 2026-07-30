// showdown_manager.cpp

#include "mxh/server/showdown_manager.hpp"
#include <algorithm>

namespace mxh::server {

bool ShowdownManager::open_challenge(std::uint32_t challenger_id,
                                       std::uint32_t defender_id,
                                       std::uint32_t money,
                                       std::uint32_t now_ms) noexcept {
    if (challenger_id == 0 || defender_id == 0 || challenger_id == defender_id) return false;
    for (auto& e : entries_) {
        if (e.state == 0 && e.challenger_id == challenger_id) return false;
    }
    ShowdownEntry e{};
    e.challenger_id = challenger_id;
    e.defender_id   = defender_id;
    e.money         = money;
    e.create_ms     = now_ms;
    e.expire_ms     = now_ms + 5ULL * 60ULL * 1000ULL;  // 5 minutes
    e.state         = 0;
    entries_.push_back(e);
    return true;
}

bool ShowdownManager::accept(std::uint32_t challenger_id, std::uint32_t defender_id) noexcept {
    auto* e = find_open(challenger_id, defender_id);
    if (!e) return false;
    const_cast<ShowdownEntry*>(e)->state = 1;
    return true;
}

bool ShowdownManager::cancel(std::uint32_t challenger_id) noexcept {
    bool any = false;
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->state == 0 && it->challenger_id == challenger_id) {
            entries_.erase(it);
            any = true;
            break;
        }
    }
    return any;
}

const ShowdownEntry* ShowdownManager::find_open(std::uint32_t challenger_id,
                                                  std::uint32_t defender_id) const noexcept {
    for (const auto& e : entries_) {
        if (e.state == 0 && e.challenger_id == challenger_id && e.defender_id == defender_id)
            return &e;
    }
    return nullptr;
}

// -------- AuctionManager --------

bool AuctionManager::register_item(std::uint32_t seller_id, std::uint32_t item_idx,
                                    std::uint16_t count, std::uint32_t money,
                                    std::uint32_t now_ms) noexcept {
    if (seller_id == 0 || item_idx == 0 || count == 0) return false;
    AuctionEntry e{};
    e.auction_idx = next_idx_++;
    e.seller_id   = seller_id;
    e.item_idx    = item_idx;
    e.item_count  = count;
    e.money       = money;
    e.register_ms = now_ms;
    e.end_ms      = now_ms + 24ULL * 3600ULL * 1000ULL;   // 24 hours
    entries_.push_back(e);
    return true;
}

bool AuctionManager::bid(std::uint32_t auction_idx, std::uint32_t buyer_id) noexcept {
    auto* e = find(auction_idx);
    if (!e) return false;
    if (e->buyer_id != 0) return false;     // already sold
    if (buyer_id == e->seller_id) return false;
    const_cast<AuctionEntry*>(e)->buyer_id = buyer_id;
    return true;
}

bool AuctionManager::cancel(std::uint32_t auction_idx, std::uint32_t seller_id) noexcept {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->auction_idx == auction_idx && it->seller_id == seller_id &&
            it->buyer_id == 0) {
            entries_.erase(it);
            return true;
        }
    }
    return false;
}

const AuctionEntry* AuctionManager::find(std::uint32_t auction_idx) const noexcept {
    for (const auto& e : entries_) if (e.auction_idx == auction_idx) return &e;
    return nullptr;
}

void AuctionManager::tick(std::uint32_t now_ms) noexcept {
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
        [&](const AuctionEntry& e){
            return e.buyer_id == 0 && now_ms >= e.end_ms;
        }), entries_.end());
}

}  // namespace mxh::server
