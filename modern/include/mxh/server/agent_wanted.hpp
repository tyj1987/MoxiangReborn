#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_WANTED (MP_WANTED=51).
inline constexpr std::uint8_t wanted_category=51;
// Sub-protocols within MP_PROTOCOL_WANTED (offset 0..30).
inline constexpr std::uint8_t wanted_notify_delete_to_map=9,wanted_notify_regist_to_map=8;
inline constexpr std::uint8_t wanted_notify_notcomplete_to_map=18,wanted_destroyed_to_map=28;
inline constexpr std::uint8_t wanted_notcomplete_to_agent=23,wanted_notcomplete_by_delchr=24;
enum class WantedServerActionKind : std::uint8_t { broadcast_to_other_maps, complete_notcomplete_send_to_map, default_forward_to_client, drop_no_user };
struct WantedRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; bool user_found=true; };
struct WantedAction { WantedServerActionKind kind=WantedServerActionKind::default_forward_to_client; std::uint8_t protocol=0; std::uint32_t object_id=0; };
WantedAction classify_wanted(const WantedRequest&);
}
