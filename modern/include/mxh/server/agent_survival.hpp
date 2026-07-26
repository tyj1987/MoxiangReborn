#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_SURVIVAL (MP_SURVIVAL=70).
inline constexpr std::uint8_t survival_category=70;
// Sub-protocols within MP_PROTOCOL_SURVIVAL (offset 0..25).
inline constexpr std::uint8_t survival_info=0,survival_aliveuser_count=1,survival_returntomap=2,survival_leave_syn=3;
inline constexpr std::uint8_t survival_ready_syn=7,survival_stop_syn=16,survival_mapoff_syn=19,survival_itemusingcount_set=24;
enum class SurvivalUserActionKind : std::uint8_t { send_leave_syn_to_map, gm_protected_forward_to_map, default_forward_to_map };
enum class SurvivalServerActionKind : std::uint8_t { update_user_map_and_forward_to_client, default_forward_to_client };
struct SurvivalUserRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t unique_connect_idx=0; std::uint8_t user_level=0; std::uint8_t channel=0; bool user_found=true; };
struct SurvivalServerRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t target_map=0; bool target_map_port_found=false; bool user_found=true; };
struct SurvivalUserAction { SurvivalUserActionKind kind=SurvivalUserActionKind::default_forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t unique_connect_idx=0; std::uint8_t user_level=0; std::uint8_t channel=0; };
struct SurvivalServerAction { SurvivalServerActionKind kind=SurvivalServerActionKind::default_forward_to_client; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t target_map=0; bool update_user_state=false; };
SurvivalUserAction classify_survival_user(const SurvivalUserRequest&);
SurvivalServerAction classify_survival_server(const SurvivalServerRequest&);
}
