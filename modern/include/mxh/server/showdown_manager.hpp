// showdown_manager.hpp + auction_manager.hpp
// 1:1 port of legacy [Server]Map/ShowdownManager + AuctionManager.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::server {

// One showdown entry (legacy ShowdownEntry).
struct ShowdownEntry final {
    std::uint32_t challenger_id  = 0;
    std::uint32_t defender_id    = 0;
    std::uint32_t money          = 0;
    std::uint32_t create_ms      = 0;
    std::uint32_t expire_ms      = 0;     // legacy accept deadline
    std::uint8_t  state          = 0;     // 0=open, 1=accepted, 2=closed
    std::uint8_t  reserved0      = 0;
    std::uint16_t reserved1      = 0;
};

class ShowdownManager final {
public:
    bool open_challenge(std::uint32_t challenger_id, std::uint32_t defender_id,
                          std::uint32_t money, std::uint32_t now_ms) noexcept;
    bool accept(std::uint32_t challenger_id, std::uint32_t defender_id) noexcept;
    bool cancel(std::uint32_t challenger_id) noexcept;
    const ShowdownEntry* find_open(std::uint32_t challenger_id,
                                       std::uint32_t defender_id) const noexcept;
    std::size_t size() const noexcept { return entries_.size(); }
private:
    std::vector<ShowdownEntry> entries_;
};

// One auction entry (legacy AuctionEntry / SellItemInfo).
struct AuctionEntry final {
    std::uint32_t auction_idx  = 0;
    std::uint32_t seller_id    = 0;
    std::uint32_t buyer_id     = 0;       // 0 = listed, not sold
    std::uint32_t item_idx     = 0;
    std::uint16_t item_count   = 1;
    std::uint16_t reserved0    = 0;
    std::uint32_t money        = 0;
    std::uint32_t register_ms  = 0;
    std::uint32_t end_ms       = 0;
};

class AuctionManager final {
public:
    bool register_item(std::uint32_t seller_id, std::uint32_t item_idx,
                         std::uint16_t count, std::uint32_t money,
                         std::uint32_t now_ms) noexcept;
    bool bid(std::uint32_t auction_idx, std::uint32_t buyer_id) noexcept;
    bool cancel(std::uint32_t auction_idx, std::uint32_t seller_id) noexcept;
    const AuctionEntry* find(std::uint32_t auction_idx) const noexcept;
    std::size_t size() const noexcept { return entries_.size(); }
    // Tick to expire listings past end_ms.
    void tick(std::uint32_t now_ms) noexcept;
private:
    std::vector<AuctionEntry> entries_;
    std::uint32_t next_idx_ = 1;
};

}  // namespace mxh::server
