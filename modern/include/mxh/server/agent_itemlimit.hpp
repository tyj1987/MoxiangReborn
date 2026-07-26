#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_ITEMLIMIT (MP_ITEMLIMIT=74).
inline constexpr std::uint8_t itemlimit_category=74;
// Sub-protocols within MP_PROTOCOL_ITEMLIMIT (offset 0..1).
inline constexpr std::uint8_t itemlimit_addcount_to_map=0;
inline constexpr std::uint8_t itemlimit_full_to_client=1;
enum class ItemLimitActionKind : std::uint8_t { broadcast_to_other_maps, forward_to_client };
struct ItemLimitRequest { std::uint8_t protocol=0; };
struct ItemLimitAction { ItemLimitActionKind kind=ItemLimitActionKind::forward_to_client; std::uint8_t protocol=0; };
ItemLimitAction classify_itemlimit(const ItemLimitRequest&);
}
