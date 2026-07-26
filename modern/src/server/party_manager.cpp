// party_manager.cpp - implementation for Party 1:1 port.

#include "mxh/server/party_manager.hpp"
#include <algorithm>
#include <cstring>

namespace mxh::server {

namespace {
void copy_name(std::array<char, 17>& dst, const std::string& src) noexcept {
    std::size_t n = std::min(src.size(), dst.size() - 1);
    std::memset(dst.data(), 0, dst.size());
    std::memcpy(dst.data(), src.data(), n);
}
}  // namespace

Party create_party(std::uint32_t party_id, std::uint32_t master_id,
                   const std::string& master_name, std::uint16_t master_level,
                   std::uint8_t option) noexcept {
    Party p;
    p.party_id = party_id;
    p.master_id = master_id;
    p.option = option;
    p.members[0].member_id = master_id;
    copy_name(p.members[0].name, master_name);
    p.members[0].level = master_level;
    p.members[0].logged_in = true;
    p.members[0].life_percent = 100;
    p.members[0].shield_percent = 100;
    p.members[0].naeryuk_percent = 100;
    p.member_count = 1;
    return p;
}

bool add_member(Party& party, std::uint32_t member_id,
                const std::string& member_name, std::uint16_t level) noexcept {
    if (is_party_member(party, member_id)) return false;
    if (party.member_count >= kPartyMaxMembers) return false;
    auto& slot = party.members[party.member_count++];
    slot.member_id = member_id;
    copy_name(slot.name, member_name);
    slot.level = level;
    slot.logged_in = true;
    slot.life_percent = 100;
    slot.shield_percent = 100;
    slot.naeryuk_percent = 100;
    return true;
}

bool remove_member(Party& party, std::uint32_t member_id) noexcept {
    auto idx = find_member_index(party, member_id);
    if (!idx.has_value()) return false;
    std::uint8_t i = *idx;
    // Shift members down to keep slots contiguous; legacy uses
    // m_Member[i+1..n] = m_Member[i..n-1] then n--.
    for (std::uint8_t j = i; j + 1 < party.member_count; ++j) {
        party.members[j] = party.members[j + 1];
    }
    party.members[party.member_count - 1] = PartyMember{};
    --party.member_count;
    // If the master was removed, promote slot 1 (old slot 0 if not removed).
    if (member_id == party.master_id) {
        if (party.member_count > 0) {
            party.master_id = party.members[0].member_id;
        } else {
            party.master_id = 0;
        }
    }
    return true;
}

bool change_master(Party& party, std::uint32_t new_master_id) noexcept {
    if (new_master_id == 0) return false;
    if (!is_party_member(party, new_master_id)) return false;
    party.master_id = new_master_id;
    party.master_changing = false;
    return true;
}

bool is_party_member(const Party& party, std::uint32_t member_id) noexcept {
    if (member_id == 0) return false;
    for (std::uint8_t i = 0; i < party.member_count; ++i) {
        if (party.members[i].member_id == member_id) return true;
    }
    return false;
}

std::optional<std::uint8_t> find_member_index(const Party& party, std::uint32_t member_id) noexcept {
    for (std::uint8_t i = 0; i < party.member_count; ++i) {
        if (party.members[i].member_id == member_id) return i;
    }
    return std::nullopt;
}

std::uint8_t online_member_count(const Party& party) noexcept {
    std::uint8_t n = 0;
    for (std::uint8_t i = 0; i < party.member_count; ++i) {
        if (party.members[i].logged_in) ++n;
    }
    return n;
}

std::uint32_t first_online_member_id(const Party& party) noexcept {
    for (std::uint8_t i = 0; i < party.member_count; ++i) {
        if (party.members[i].logged_in) return party.members[i].member_id;
    }
    return 0;
}

void mark_member_logged_in(Party& party, std::uint32_t member_id,
                            std::uint8_t life_pct, std::uint8_t shield_pct,
                            std::uint8_t naeryuk_pct) noexcept {
    auto idx = find_member_index(party, member_id);
    if (!idx.has_value()) return;
    auto& m = party.members[*idx];
    m.logged_in = true;
    m.life_percent = life_pct;
    m.shield_percent = shield_pct;
    m.naeryuk_percent = naeryuk_pct;
}

void mark_member_logged_out(Party& party, std::uint32_t member_id) noexcept {
    auto idx = find_member_index(party, member_id);
    if (!idx.has_value()) return;
    auto& m = party.members[*idx];
    m.logged_in = false;
    m.life_percent = 0;
    m.shield_percent = 0;
    m.naeryuk_percent = 0;
}

void set_member_level(Party& party, std::uint32_t member_id, std::uint16_t level) noexcept {
    auto idx = find_member_index(party, member_id);
    if (!idx.has_value()) return;
    party.members[*idx].level = level;
}

void start_master_request(Party& party, std::uint32_t requester_id, std::uint32_t now_ms) noexcept {
    party.master_changing = true;
    party.request_player_id = requester_id;
    party.request_process_time_ms = now_ms;
}

bool master_request_timed_out(Party& party, std::uint32_t now_ms) noexcept {
    if (!party.master_changing) return false;
    // Window: (start, start + 10000] i.e. exactly start+10000 expires.
    // If request_process_time_ms is 0 it means wall-clock=0; still valid.
    std::uint32_t elapsed = now_ms - party.request_process_time_ms;
    if (elapsed <= Party::kDecisionTimeMs) return false;
    // Expired: clear state, requester can retry
    party.master_changing = false;
    party.request_player_id = 0;
    party.request_process_time_ms = 0;
    return true;
}

std::optional<Party*> find_party_by_id(PartyLog& log, std::uint32_t party_id) noexcept {
    for (auto& p : log.parties) {
        if (p.party_id == party_id) return &p;
    }
    return std::nullopt;
}

std::optional<Party*> find_party_of_player(PartyLog& log, std::uint32_t member_id) noexcept {
    for (auto& p : log.parties) {
        if (is_party_member(p, member_id)) return &p;
    }
    return std::nullopt;
}

bool disband_if_empty(PartyLog& log, std::uint32_t party_id) noexcept {
    auto p = find_party_by_id(log, party_id);
    if (!p.has_value()) return false;
    if ((*p)->member_count > 0) return false;
    for (auto it = log.parties.begin(); it != log.parties.end(); ++it) {
        if (it->party_id == party_id) {
            log.parties.erase(it);
            return true;
        }
    }
    return false;
}

}  // namespace mxh::server
