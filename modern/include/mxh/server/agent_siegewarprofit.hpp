#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_SIEGEWAR_PROFIT (MP_SIEGEWAR_PROFIT=63).
inline constexpr std::uint8_t siegewarprofit_category=63;
// Sub-protocols within MP_PROTOCOL_SIEGEWAR_PROFIT (offset 0..11).
inline constexpr std::uint8_t siegewarprofit_change_texrate_notify_to_map=7;
inline constexpr std::uint8_t siegewarprofit_change_guild_notify_to_map=11;
enum class SiegeWarProfitUserActionKind : std::uint8_t { forward_to_map };
enum class SiegeWarProfitServerActionKind : std::uint8_t { broadcast_to_other_maps, forward_to_client };
struct SiegeWarProfitRequest { std::uint8_t protocol=0; };
struct SiegeWarProfitUserAction { SiegeWarProfitUserActionKind kind=SiegeWarProfitUserActionKind::forward_to_map; };
struct SiegeWarProfitServerAction { SiegeWarProfitServerActionKind kind=SiegeWarProfitServerActionKind::forward_to_client; std::uint8_t protocol=0; };
SiegeWarProfitUserAction classify_siegewarprofit_user();
SiegeWarProfitServerAction classify_siegewarprofit_server(const SiegeWarProfitRequest&);
}
