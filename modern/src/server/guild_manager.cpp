// guild_manager.cpp

#include "mxh/server/guild_manager.hpp"
#include <algorithm>
#include <cstring>

namespace mxh::server {

namespace {
void copy_name(std::array<char, 17>& dst, const std::string& src) noexcept {
    std::size_t n = std::min(src.size(), dst.size() - 1);
    std::memset(dst.data(), 0, dst.size());
    std::memcpy(dst.data(), src.data(), n);
}
constexpr std::uint32_t kGuildPointCap = 10000000u;  // 10 million (legacy)
}

Guild create_guild(std::uint32_t guild_id, const std::string& name, std::uint32_t master_id, std::uint32_t money) noexcept {
    Guild g;
    g.guild_id = guild_id;
    copy_name(g.name, name);
    g.master_id = master_id;
    g.money = money;
    g.level = 1;

    GuildMember m;
    m.member_id = master_id;
    copy_name(m.name, name);
    m.rank = 3;  // master
    m.is_student = false;
    g.members[0] = m;
    g.member_count = 1;
    return g;
}

bool add_member(Guild& g, const GuildMember& m) noexcept {
    if (is_member(g, m.member_id)) return false;
    if (g.member_count >= kGuildMemberMax) return false;
    g.members[g.member_count++] = m;
    return true;
}

bool delete_member(Guild& g, std::uint32_t member_id) noexcept {
    auto idx = find_member_index(g, member_id);
    if (!idx.has_value()) return false;
    std::uint8_t i = *idx;
    for (std::uint8_t j = i; j + 1 < g.member_count; ++j) {
        g.members[j] = g.members[j + 1];
    }
    g.members[g.member_count - 1] = GuildMember{};
    --g.member_count;
    // Also clear any rank pointer
    if (g.rank_member_idx[0] == member_id) g.rank_member_idx[0] = 0;
    if (g.rank_member_idx[1] == member_id) g.rank_member_idx[1] = 0;
    if (g.rank_member_idx[2] == member_id) g.rank_member_idx[2] = 0;
    return true;
}

bool is_member(const Guild& g, std::uint32_t member_id) noexcept {
    return find_member_index(g, member_id).has_value();
}

bool is_master(const Guild& g, std::uint32_t member_id) noexcept {
    return g.master_id == member_id && member_id != 0;
}

bool is_vice_master(const Guild& g, std::uint32_t member_id) noexcept {
    return g.rank_member_idx[0] == member_id && member_id != 0;
}

std::optional<std::uint8_t> find_member_index(const Guild& g, std::uint32_t member_id) noexcept {
    if (member_id == 0) return std::nullopt;
    for (std::uint8_t i = 0; i < g.member_count; ++i) {
        if (g.members[i].member_id == member_id) return i;
    }
    return std::nullopt;
}

bool set_rank(Guild& g, std::uint32_t member_id, RankPos pos) noexcept {
    auto idx = static_cast<std::size_t>(pos);
    if (idx >= g.rank_member_idx.size()) return false;
    if (!is_member(g, member_id) && pos != RankPos::ViceMaster)
        return false;
    // Vice-master is rank slot 0 specifically
    if (pos == RankPos::ViceMaster) {
        // Demote previous vice master to rank 0 if they are a member
        if (g.rank_member_idx[idx] != 0) {
            auto prev = find_member_index(g, g.rank_member_idx[idx]);
            if (prev.has_value()) g.members[*prev].rank = 0;
        }
        g.rank_member_idx[idx] = member_id;
        auto new_idx = find_member_index(g, member_id);
        if (new_idx.has_value()) g.members[*new_idx].rank = 2;  // vice
        return true;
    }
    if (pos == RankPos::Senior_1 || pos == RankPos::Senior_2) {
        if (g.rank_member_idx[idx] != 0) {
            auto prev = find_member_index(g, g.rank_member_idx[idx]);
            if (prev.has_value()) g.members[*prev].rank = 0;
        }
        g.rank_member_idx[idx] = member_id;
        auto new_idx = find_member_index(g, member_id);
        if (new_idx.has_value()) g.members[*new_idx].rank = 1;  // senior
        return true;
    }
    return false;  // Max/Err or other
}

std::uint32_t get_rank_member(const Guild& g, RankPos pos) noexcept {
    auto idx = static_cast<std::size_t>(pos);
    if (idx >= g.rank_member_idx.size()) return 0;
    return g.rank_member_idx[idx];
}

std::optional<std::uint32_t> add_guild_point(Guild& g, std::uint32_t delta) noexcept {
    std::uint64_t sum = static_cast<std::uint64_t>(g.guild_point) + delta;
    if (sum > kGuildPointCap) return std::nullopt;
    g.guild_point = static_cast<std::uint32_t>(sum);
    return g.guild_point;
}

std::optional<std::uint32_t> use_guild_point(Guild& g, std::uint32_t amount,
                                          GuildPointKind kind) noexcept {
    (void)kind;
    if (amount > g.guild_point) return std::nullopt;
    g.guild_point -= amount;
    return g.guild_point;
}

std::optional<Guild*> find_guild_by_id(GuildLog& log, std::uint32_t guild_id) noexcept {
    for (auto& g : log.guilds) {
        if (g.guild_id == guild_id) return &g;
    }
    return std::nullopt;
}

std::optional<Guild*> find_guild_of_member(GuildLog& log, std::uint32_t member_id) noexcept {
    for (auto& g : log.guilds) {
        if (is_member(g, member_id)) return &g;
    }
    return std::nullopt;
}

}  // namespace mxh::server
