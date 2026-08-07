// agent_char.hpp - AgentChar data plane (category=3, MP_MP_CHAR).
//
// 1:1 port of MP_MP_CHARMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp.
//
// The agent server has no per-protocol handler in legacy for this category;
// the default branch in every category parser falls through to
// Send2User(FindUserByObjectID(pTempMsg->dwObjectID), pMsg, dwLength).
// That is, MP_MP_CHAR traffic is forwarded verbatim to the
// connected user whose object_id matches the packet header. If the user
// is not found the packet is silently dropped (no NACK, no log).
//
// We preserve this verbatim: MP_MP_CHAR at the agent is a pure
// object-id forward/drop switch with no validation, no DB write, no broadcast.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_MP_CHAR (3 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t mp_char_category = 3u;

// Legacy sub-protocol offsets within MP_PROTOCOL_MP_CHAR (0..76,
// see [CC]Header/Protocol.h). Each is preserved verbatim so the
// modern orchestrator can echo the protocol byte when forwarding
// to the resolved user.
inline constexpr std::uint8_t mp_char_life_syn = 0u;
inline constexpr std::uint8_t mp_char_life_ack = 1u;
inline constexpr std::uint8_t mp_char_life_nack = 2u;
inline constexpr std::uint8_t mp_char_maxlife_notify = 3u;
inline constexpr std::uint8_t mp_char_shield_syn = 4u;
inline constexpr std::uint8_t mp_char_shield_ack = 5u;
inline constexpr std::uint8_t mp_char_shield_nack = 6u;
inline constexpr std::uint8_t mp_char_maxshield_notify = 7u;
inline constexpr std::uint8_t mp_char_naeryuk_syn = 8u;
inline constexpr std::uint8_t mp_char_naeryuk_ack = 9u;
inline constexpr std::uint8_t mp_char_naeryuk_nack = 10u;
inline constexpr std::uint8_t mp_char_maxnaeryuk_notify = 11u;
inline constexpr std::uint8_t mp_char_exppoint_syn = 12u;
inline constexpr std::uint8_t mp_char_exppoint_ack = 13u;
inline constexpr std::uint8_t mp_char_exppoint_nack = 14u;
inline constexpr std::uint8_t mp_char_gengol_notify = 15u;
inline constexpr std::uint8_t mp_char_minchub_notify = 16u;
inline constexpr std::uint8_t mp_char_simmek_notify = 17u;
inline constexpr std::uint8_t mp_char_cheryuk_notify = 18u;
inline constexpr std::uint8_t mp_char_level_notify = 19u;
inline constexpr std::uint8_t mp_char_playerlevelup_notify = 20u;
inline constexpr std::uint8_t mp_char_pointadd_syn = 21u;
inline constexpr std::uint8_t mp_char_pointadd_ack = 22u;
inline constexpr std::uint8_t mp_char_pointadd_nack = 23u;
inline constexpr std::uint8_t mp_char_leveluppoint_notify = 24u;
inline constexpr std::uint8_t mp_char_leveldown_syn = 25u;
inline constexpr std::uint8_t mp_char_leveldown_ack = 26u;
inline constexpr std::uint8_t mp_char_leveldown_nack = 27u;
inline constexpr std::uint8_t mp_char_fame_notify = 28u;
inline constexpr std::uint8_t mp_char_state_notify = 29u;
inline constexpr std::uint8_t mp_char_life_notify = 30u;
inline constexpr std::uint8_t mp_char_abilityexppoint_syn = 31u;
inline constexpr std::uint8_t mp_char_abilityexppoint_ack = 32u;
inline constexpr std::uint8_t mp_char_abilityexppoint_nack = 33u;
inline constexpr std::uint8_t mp_char_ability_upgrade_syn = 34u;
inline constexpr std::uint8_t mp_char_ability_upgrade_ack = 35u;
inline constexpr std::uint8_t mp_char_ability_upgrade_nack = 36u;
inline constexpr std::uint8_t mp_char_youaredied = 37u;
inline constexpr std::uint8_t mp_char_exitstart_syn = 38u;
inline constexpr std::uint8_t mp_char_exitstart_ack = 39u;
inline constexpr std::uint8_t mp_char_exitstart_nack = 40u;
inline constexpr std::uint8_t mp_char_exit_syn = 41u;
inline constexpr std::uint8_t mp_char_exit_ack = 42u;
inline constexpr std::uint8_t mp_char_exit_nack = 43u;
inline constexpr std::uint8_t mp_char_badfame_notify = 44u;
inline constexpr std::uint8_t mp_char_badfame_syn = 45u;
inline constexpr std::uint8_t mp_char_badfame_ack = 46u;
inline constexpr std::uint8_t mp_char_badfame_nack = 47u;
inline constexpr std::uint8_t mp_char_badfame_changed = 48u;
inline constexpr std::uint8_t mp_char_playtime_syn = 49u;
inline constexpr std::uint8_t mp_char_playtime_ack = 50u;
inline constexpr std::uint8_t mp_char_playtime_nack = 51u;
inline constexpr std::uint8_t mp_char_pointminus_syn = 52u;
inline constexpr std::uint8_t mp_char_pointminus_ack = 53u;
inline constexpr std::uint8_t mp_char_pointminus_nack = 54u;
inline constexpr std::uint8_t mp_char_ability_upgrade_skpoint_syn = 55u;
inline constexpr std::uint8_t mp_char_ability_upgrade_skpoint_ack = 56u;
inline constexpr std::uint8_t mp_char_ability_upgrade_skpoint_nack = 57u;
inline constexpr std::uint8_t mp_char_ability_downgrade_skpoint_syn = 58u;
inline constexpr std::uint8_t mp_char_ability_downgrade_skpoint_ack = 59u;
inline constexpr std::uint8_t mp_char_ability_downgrade_skpoint_nack = 60u;
inline constexpr std::uint8_t mp_char_stage_notify = 61u;
inline constexpr std::uint8_t mp_char_change_subattr_ack = 62u;
inline constexpr std::uint8_t mp_char_change_subattr_nack = 63u;
inline constexpr std::uint8_t mp_char_mussang_syn = 64u;
inline constexpr std::uint8_t mp_char_mussang_ack = 65u;
inline constexpr std::uint8_t mp_char_mussang_nack = 66u;
inline constexpr std::uint8_t mp_char_mussang_info = 67u;
inline constexpr std::uint8_t mp_char_mussang_end = 68u;
inline constexpr std::uint8_t mp_char_single_special_state_notify = 69u;
inline constexpr std::uint8_t mp_char_single_special_state_ack = 70u;
inline constexpr std::uint8_t mp_char_single_special_state_nack = 71u;
inline constexpr std::uint8_t mp_char_fullmoonevent_change = 72u;
inline constexpr std::uint8_t mp_char_noactionpanelty_notify = 73u;
inline constexpr std::uint8_t mp_char_ability_reset_skpoint_syn = 74u;
inline constexpr std::uint8_t mp_char_ability_reset_skpoint_ack = 75u;
inline constexpr std::uint8_t mp_char_ability_reset_skpoint_nack = 76u;

enum class AgentCharOutcome : std::uint8_t {
    ForwardToUser,  // legacy: FindUser(pTempMsg->dwObjectID) succeeded -> Send2User
    DropNoUser,     // legacy: FindUser returned null -> silent no-op
};

struct AgentCharRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
};

inline AgentCharOutcome classify_agent_char(const AgentCharRequest& r) noexcept {
    return r.user_found ? AgentCharOutcome::ForwardToUser
                        : AgentCharOutcome::DropNoUser;
}

}  // namespace mxh::server
