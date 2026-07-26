// guild_manager.hpp - 1:1 port of legacy [Server]Map/Guild.h (CGuild + RTChangeRank)
// + [Server]Map/GuildManager.h (CGuildManager singleton). Modern port models
// guild member list + rank table + master/vice-master logic + point usage
// tracking, all as POD structs + pure functions.

#pragma once

#include "mxh/server/player_state.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mxh::server {

inline constexpr std::uint16_t kGuildStudentMax = 25;  // GUILD_STUDENT_NUM_MAX
inline constexpr std::uint16_t kGuildMemberMax = 50;   // legacy m_MemberList cap

// Legacy RTChangeRank enum (1:1 numbering preserved for wire format).
enum class RankPos : std::uint8_t {
    ViceMaster = 0,  // vice master
    Senior_1  = 1,
    Senior_2  = 2,
    Max       = 3,  // sentinel
    Err       = 4,
};

// Legacy eGuildPointUseKind
enum class GuildPointKind : std::uint8_t {
    PlusTime = 0,
    Mugong  = 1,
};

// Legacy GUILDMEMBERINFO pod (used in code paths like AddMember).
struct GuildMember final {
    std::uint32_t member_id = 0;
    std::array<char, 17> name{};
    std::uint16_t level = 0;
    std::uint8_t  rank = 0;      // 0=student, 1=senior, 2=vice, 3=master
    bool is_student = false;
    std::uint32_t connected_map_num = 0;
};

// Legacy GUILDINFO pod (the persistent core, plus the in-memory state).
struct Guild final {
    std::uint32_t guild_id = 0;
    std::array<char, 17> name{};
    std::uint32_t master_id = 0;
    std::uint32_t money = 0;
    std::uint32_t guild_point = 0;
    std::array<std::uint32_t, 3> rank_member_idx{};
    std::uint32_t gt_battle_id = 0;
    std::uint8_t  level = 1;
    std::uint8_t  member_count = 0;
    std::array<GuildMember, kGuildMemberMax> members{};
};

// ---- Factory ----
Guild create_guild(std::uint32_t guild_id, const std::string& name,
                    std::uint32_t master_id, std::uint32_t money = 0) noexcept;

// ---- Membership ----
bool add_member(Guild& g, const GuildMember& m) noexcept;
bool delete_member(Guild& g, std::uint32_t member_id) noexcept;
bool is_member(const Guild& g, std::uint32_t member_id) noexcept;
bool is_master(const Guild& g, std::uint32_t member_id) noexcept;
bool is_vice_master(const Guild& g, std::uint32_t member_id) noexcept;
std::optional<std::uint8_t> find_member_index(const Guild& g, std::uint32_t member_id) noexcept;

// ---- Rank management ----
bool set_rank(Guild& g, std::uint32_t member_id, RankPos pos) noexcept;
std::uint32_t get_rank_member(const Guild& g, RankPos pos) noexcept;

// Guild point tracking (legacy: deduction/accrual with overflow clamp).
std::optional<std::uint32_t> add_guild_point(Guild& g, std::uint32_t delta) noexcept;
std::optional<std::uint32_t> use_guild_point(Guild& g, std::uint32_t amount,
                                          GuildPointKind kind) noexcept;

// ---- Guild log (legacy GuildManager singleton) ----
struct GuildLog final {
    std::uint32_t next_guild_id = 1;
    std::vector<Guild> guilds;
};

std::optional<Guild*> find_guild_by_id(GuildLog& log, std::uint32_t guild_id) noexcept;
std::optional<Guild*> find_guild_of_member(GuildLog& log, std::uint32_t member_id) noexcept;

}  // namespace mxh::server
