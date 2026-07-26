#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_HACKCHECK (MP_HACKCHECK=41).
inline constexpr std::uint8_t hackcheck_category=41;
// Sub-protocols within MP_PROTOCOL_HACKCHECK (offset 0..2).
inline constexpr std::uint8_t hackcheck_speedhack=0,hackcheck_ban_user=1,hackcheck_ban_user_toagent=2;
// Speedhack threshold (SPEEDHACK_CHECKTIME minus 3000ms tolerance); kept abstract as 7000 to match legacy time-budget heuristic.
inline constexpr std::uint32_t speedhack_checktime=10000;
inline constexpr std::uint32_t speedhack_tolerance_ms=3000;
enum class HackCheckActionKind : std::uint8_t { detect_speedhack_and_ban, ban_user_to_agent_always, drop_no_user, ignore };
struct HackCheckRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t client_time=0; std::uint32_t server_time=0; bool user_found=true; };
struct HackCheckAction { HackCheckActionKind kind=HackCheckActionKind::ignore; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t data=0; };
HackCheckAction classify_hackcheck(const HackCheckRequest&);
}
