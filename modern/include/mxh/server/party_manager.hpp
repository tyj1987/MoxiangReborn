// party_manager.hpp - 1:1 port of legacy [Server]Map/Party.h (CParty +
// PARTYMEMBER) into modern POD structs + pure functions.

#pragma once

#include "mxh/server/player_state.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mxh::server {

// MAX_PARTY_LISTNUM = 8 (legacy CN/KR; sometimes 7 in JP)
inline constexpr std::uint16_t kPartyMaxMembers = 8;

// Party options (legacy m_Option byte): bits for distribute-mode flags.
// Item-distribute and exp-distribute modes can be 1:1 / random / master-only.
inline constexpr std::uint8_t kPartyOptItemFree      = 0x01;
inline constexpr std::uint8_t kPartyOptItemRandom    = 0x02;
inline constexpr std::uint8_t kPartyOptExpEqual      = 0x04;
inline constexpr std::uint8_t kPartyOptExpLevelRatio = 0x08;

// Legacy PARTYMEMBER struct (1:1 fields).
struct PartyMember final {
    std::uint32_t member_id = 0;
    std::array<char, 17> name{};  // MAX_NAME_LENGTH + 1
    bool logged_in = false;
    std::uint8_t life_percent = 0;     // 0..100
    std::uint8_t shield_percent = 0;
    std::uint8_t naeryuk_percent = 0;
    std::uint16_t level = 0;
};

// Legacy CParty data (1:1 port of members + party-wide options).
// Includes master-decision machinery with a 10 s window.
struct Party final {
    std::uint32_t party_id = 0;
    std::array<PartyMember, kPartyMaxMembers> members{};
    std::uint8_t member_count = 0;
    std::uint32_t master_id = 0;        // member[0].member_id in legacy
    std::uint32_t tactic_object_id = 0;
    std::uint8_t option = 0;
    std::uint32_t old_send_time_ms = 0;
    std::uint32_t request_player_id = 0;
    std::uint32_t request_process_time_ms = 0;
    bool master_changing = false;

    // distribute mode helper
    static constexpr std::uint32_t kDecisionTimeMs = 10000;
};

// ---- Factory ----
// Create a new party with the given master. Returns party with
// members[0] populated as the master.
Party create_party(std::uint32_t party_id,
                   std::uint32_t master_id,
                   const std::string& master_name,
                   std::uint16_t master_level,
                   std::uint8_t option = 0) noexcept;

// ---- Membership mutations ----
// Add a member (assigns to slot[member_count]). Returns true if added.
bool add_member(Party& party, std::uint32_t member_id,
                const std::string& member_name, std::uint16_t level) noexcept;

// Remove a member by id. If the removed member was the master, the next
// member (slot 1) is promoted to master. Returns true if a member was
// removed.
bool remove_member(Party& party, std::uint32_t member_id) noexcept;

// Change the master to a different existing member. Returns true on success.
bool change_master(Party& party, std::uint32_t new_master_id) noexcept;

// ---- Queries ----
bool is_party_member(const Party& party, std::uint32_t member_id) noexcept;
std::optional<std::uint8_t> find_member_index(const Party& party,
                                              std::uint32_t member_id) noexcept;
std::uint8_t online_member_count(const Party& party) noexcept;
std::uint32_t first_online_member_id(const Party& party) noexcept;

// ---- Login state ----
// Mark a member as logged in (sets the percent values too). Used when a
// player logs back in to set HP/Shield/NA bars from fresh state.
void mark_member_logged_in(Party& party, std::uint32_t member_id,
                            std::uint8_t life_pct, std::uint8_t shield_pct,
                            std::uint8_t naeryuk_pct) noexcept;
void mark_member_logged_out(Party& party, std::uint32_t member_id) noexcept;
void set_member_level(Party& party, std::uint32_t member_id, std::uint16_t level) noexcept;

// ---- Master-decision machinery ----
// Start a master-decision request (legacy StartRequestProcessTime).
void start_master_request(Party& party, std::uint32_t requester_id,
                          std::uint32_t now_ms) noexcept;

// Check whether the master-decision 10 s window has expired. Resets the
// request state if expired (so a stale request does not block forever).
bool master_request_timed_out(Party& party, std::uint32_t now_ms) noexcept;

// ---- Per-party log (legacy PartyManager singleton) ----
struct PartyLog final {
    std::uint32_t next_party_id = 1;
    std::vector<Party> parties;
};

std::optional<Party*> find_party_by_id(PartyLog& log, std::uint32_t party_id) noexcept;
std::optional<Party*> find_party_of_player(PartyLog& log, std::uint32_t member_id) noexcept;
bool disband_if_empty(PartyLog& log, std::uint32_t party_id) noexcept;

}  // namespace mxh::server
