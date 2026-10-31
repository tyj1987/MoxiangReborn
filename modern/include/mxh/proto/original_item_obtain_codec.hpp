#pragma once

// ITEMOBTAINARRAY payload following the original MSGBASE. This codec does not
// choose a transport header, protocol profile, recipient, or dispatch path.
// Source: [CC]Header/CommonStruct.h ITEMOBTAINARRAY/ITEMBASE,
// SHA-256 5b485d298868a87294de7ebfc7a64919c69a754d9f9a44422251bf1947524625.

#include "mxh/game/item_types.hpp"
#include "mxh/proto/original_item_profile.hpp"
#include "mxh/proto/protocol.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace mxh::proto::original_item {

inline constexpr std::size_t kItemObtainPrefixBytes = 3;
inline constexpr std::size_t kItemObtainRecordBytes = 22;
inline constexpr std::size_t kItemObtainMaxRecords =
    static_cast<std::size_t>(mxh::game::SLOT_INVENTORY_NUM);
static_assert(sizeof(mxh::game::ItemBase) == kItemObtainRecordBytes);

struct ItemObtainArray {
    // Original wObtainCount is the quantity awarded, not ItemNum (the number
    // of changed ITEMBASE records). Keep them independent.
    std::uint32_t obtain_count = 0;
    std::vector<mxh::game::ItemBase> items;
};

namespace detail {
inline void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8u));
}
inline void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    append_u16(out, static_cast<std::uint16_t>(value));
    append_u16(out, static_cast<std::uint16_t>(value >> 16u));
}
inline std::uint16_t read_u16(std::span<const std::uint8_t> bytes,
                              std::size_t at) noexcept {
    return static_cast<std::uint16_t>(bytes[at]) |
           static_cast<std::uint16_t>(bytes[at + 1u] << 8u);
}
inline std::uint32_t read_u32(std::span<const std::uint8_t> bytes,
                              std::size_t at) noexcept {
    return static_cast<std::uint32_t>(read_u16(bytes, at)) |
           (static_cast<std::uint32_t>(read_u16(bytes, at + 2u)) << 16u);
}
}  // namespace detail

inline std::optional<std::vector<std::uint8_t>> encode_item_obtain_payload(
    const ItemObtainArray& value) {
    if (value.items.size() > kItemObtainMaxRecords ||
        value.obtain_count > std::numeric_limits<std::uint16_t>::max()) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> out;
    out.reserve(kItemObtainPrefixBytes +
                value.items.size() * kItemObtainRecordBytes);
    out.push_back(static_cast<std::uint8_t>(value.items.size()));
    detail::append_u16(out, static_cast<std::uint16_t>(value.obtain_count));
    for (const auto& item : value.items) {
        detail::append_u32(out, item.dwDBIdx);
        detail::append_u16(out, item.wIconIdx);
        detail::append_u16(out, item.Position);
        detail::append_u32(out, item.Durability);
        detail::append_u32(out, item.RareIdx);
        detail::append_u16(out, item.QuickPosition);
        detail::append_u32(out, item.ItemParam);
    }
    return out;
}

// Bytes MapHandler::send_direct_inventory_obtain puts on MonsterObtainNotify.
struct DirectInventoryObtain {
    std::uint8_t protocol =
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MonsterObtainNotify);
    std::vector<std::uint8_t> payload;
};

inline std::optional<DirectInventoryObtain> make_direct_inventory_obtain(
    std::uint32_t db_idx, std::uint16_t icon, std::uint16_t position,
    std::uint32_t durability, std::uint16_t count) {
    ItemObtainArray obtained;
    obtained.obtain_count = count;
    obtained.items.push_back(
        mxh::game::make_item(db_idx, icon, position, durability, count));
    auto payload = encode_item_obtain_payload(obtained);
    if (!payload) return std::nullopt;
    DirectInventoryObtain message;
    message.payload = std::move(*payload);
    return message;
}

inline std::optional<ItemObtainArray> decode_item_obtain_payload(
    std::span<const std::uint8_t> bytes) {
    if (bytes.size() < kItemObtainPrefixBytes) return std::nullopt;
    const auto item_num = static_cast<std::size_t>(bytes[0]);
    if (item_num > kItemObtainMaxRecords ||
        bytes.size() != kItemObtainPrefixBytes +
                        item_num * kItemObtainRecordBytes) {
        return std::nullopt;
    }
    ItemObtainArray value;
    value.obtain_count = detail::read_u16(bytes, 1u);
    value.items.reserve(item_num);
    for (std::size_t i = 0; i < item_num; ++i) {
        const auto at = kItemObtainPrefixBytes + i * kItemObtainRecordBytes;
        mxh::game::ItemBase item{};
        item.dwDBIdx = detail::read_u32(bytes, at);
        item.wIconIdx = detail::read_u16(bytes, at + 4u);
        item.Position = detail::read_u16(bytes, at + 6u);
        item.Durability = detail::read_u32(bytes, at + 8u);
        item.RareIdx = detail::read_u32(bytes, at + 12u);
        item.QuickPosition = detail::read_u16(bytes, at + 16u);
        item.ItemParam = detail::read_u32(bytes, at + 18u);
        value.items.push_back(item);
    }
    return value;
}

}  // namespace mxh::proto::original_item
