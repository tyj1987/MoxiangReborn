// guild_manager_test.cpp

#include "mxh/server/guild_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::Guild;
using mxh::server::GuildMember;
using mxh::server::GuildLog;
using mxh::server::RankPos;
using mxh::server::GuildPointKind;
using mxh::server::create_guild;
using mxh::server::add_member;
using mxh::server::delete_member;
using mxh::server::is_member;
using mxh::server::is_master;
using mxh::server::is_vice_master;
using mxh::server::find_member_index;
using mxh::server::set_rank;
using mxh::server::get_rank_member;
using mxh::server::add_guild_point;
using mxh::server::use_guild_point;
using mxh::server::find_guild_by_id;
using mxh::server::find_guild_of_member;
using mxh::server::kGuildMemberMax;

static GuildMember make_member(std::uint32_t id, const std::string& name,
                              std::uint16_t level = 50, std::uint8_t rank = 0,
                              bool student = true) {
    GuildMember m;
    m.member_id = id;
    std::size_t n = std::min(name.size(), m.name.size() - 1);
    std::memcpy(m.name.data(), name.data(), n);
    m.level = level;
    m.rank = rank;
    m.is_student = student;
    return m;
}

static Guild make_guild_with_members() {
    auto g = create_guild(100, "Dragons", 1000, 1000);
    add_member(g, make_member(1001, "Alice"));
    add_member(g, make_member(1002, "Bob"));
    return g;
}
}

// ---- create_guild ----
TEST(GuildCreateGuild, MasterIsSlot0) {
    auto g = create_guild(1, "Tigers", 7);
    EXPECT_EQ(g.guild_id, 1u);
    EXPECT_EQ(g.master_id, 7u);
    EXPECT_EQ(g.member_count, 1u);
    EXPECT_EQ(g.members[0].rank, 3u);
    EXPECT_EQ(g.money, 0u);
    EXPECT_EQ(g.guild_point, 0u);
}

TEST(GuildCreateGuild, NameCopied) {
    auto g = create_guild(1, "MyGuild", 1);
    EXPECT_EQ(g.name[0], 'M');
    EXPECT_EQ(g.name[6], 'd');  // last char
}

// ---- add_member ----
TEST(GuildAddMember, AddsToArray) {
    auto g = make_guild_with_members();
    EXPECT_EQ(g.member_count, 3u);
    EXPECT_TRUE(is_member(g, 1001));
    EXPECT_TRUE(is_member(g, 1002));
}

TEST(GuildAddMember, DuplicateFails) {
    auto g = make_guild_with_members();
    EXPECT_FALSE(add_member(g, make_member(1001, "Alice2")));
}

TEST(GuildAddMember, BeyondMaxFails) {
    auto g = create_guild(1, "X", 1);
    for (std::uint16_t i = 0; i < kGuildMemberMax; ++i) {
        add_member(g, make_member(100 + i, "m"));
    }
    EXPECT_FALSE(add_member(g, make_member(999, "z")));
}

// ---- delete_member ----
TEST(GuildDeleteMember, RemovesFromArray) {
    auto g = make_guild_with_members();
    EXPECT_TRUE(delete_member(g, 1001));
    EXPECT_FALSE(is_member(g, 1001));
    EXPECT_EQ(g.member_count, 2u);
}

TEST(GuildDeleteMember, ViceMasterDeletionClearsRank) {
    auto g = make_guild_with_members();
    set_rank(g, 1001, RankPos::ViceMaster);
    EXPECT_TRUE(is_vice_master(g, 1001));
    delete_member(g, 1001);
    EXPECT_FALSE(is_vice_master(g, 1001));
    EXPECT_EQ(get_rank_member(g, RankPos::ViceMaster), 0u);
}

TEST(GuildDeleteMember, UnknownReturnsFalse) {
    auto g = make_guild_with_members();
    EXPECT_FALSE(delete_member(g, 9999));
}

// ---- rank ----
TEST(GuildSetRank, ViceMasterAssigned) {
    auto g = make_guild_with_members();
    EXPECT_TRUE(set_rank(g, 1001, RankPos::ViceMaster));
    EXPECT_TRUE(is_vice_master(g, 1001));
    EXPECT_EQ(g.members[1].rank, 2u);  // vice
}

TEST(GuildSetRank, DemotePreviousViceMaster) {
    auto g = make_guild_with_members();
    set_rank(g, 1001, RankPos::ViceMaster);
    set_rank(g, 1002, RankPos::ViceMaster);  // replace
    EXPECT_FALSE(is_vice_master(g, 1001));
    EXPECT_TRUE(is_vice_master(g, 1002));
    EXPECT_EQ(g.members[1].rank, 0u);  // 1001 demoted
    EXPECT_EQ(g.members[2].rank, 2u);  // 1002 is vice
}

TEST(GuildSetRank, Senior_1_Assigned) {
    auto g = make_guild_with_members();
    EXPECT_TRUE(set_rank(g, 1001, RankPos::Senior_1));
    EXPECT_EQ(get_rank_member(g, RankPos::Senior_1), 1001u);
    EXPECT_EQ(g.members[1].rank, 1u);  // senior
}

TEST(GuildSetRank, Senior_2_Assigned) {
    auto g = make_guild_with_members();
    EXPECT_TRUE(set_rank(g, 1001, RankPos::Senior_2));
    EXPECT_EQ(get_rank_member(g, RankPos::Senior_2), 1001u);
}

TEST(GuildSetRank, MaxInvalidFails) {
    auto g = make_guild_with_members();
    EXPECT_FALSE(set_rank(g, 1001, RankPos::Max));
}

TEST(GuildIsMaster, OnlyMasterMatches) {
    auto g = make_guild_with_members();
    EXPECT_TRUE(is_master(g, 1000));
    EXPECT_FALSE(is_master(g, 1001));
    EXPECT_FALSE(is_master(g, 0));
}

TEST(GuildIsViceMaster, OnlyRank0Matches) {
    auto g = make_guild_with_members();
    EXPECT_FALSE(is_vice_master(g, 1000));
    set_rank(g, 1001, RankPos::ViceMaster);
    EXPECT_TRUE(is_vice_master(g, 1001));
    EXPECT_FALSE(is_vice_master(g, 1002));
}

// ---- guild point ----
TEST(GuildAddGuildPoint, Increase) {
    auto g = create_guild(1, "X", 1);
    auto r = add_guild_point(g, 100);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 100u);
    EXPECT_EQ(g.guild_point, 100u);
}

TEST(GuildAddGuildPoint, CapOverflow) {
    auto g = create_guild(1, "X", 1);
    g.guild_point = 10000000u - 5;
    EXPECT_FALSE(add_guild_point(g, 100).has_value());
}

TEST(GuildUseGuildPoint, Decrease) {
    auto g = create_guild(1, "X", 1);
    g.guild_point = 1000;
    auto r = use_guild_point(g, 200, GuildPointKind::PlusTime);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 800u);
}

TEST(GuildUseGuildPoint, Insufficient) {
    auto g = create_guild(1, "X", 1);
    g.guild_point = 100;
    EXPECT_FALSE(use_guild_point(g, 200, GuildPointKind::Mugong).has_value());
}

// ---- GuildLog ----
TEST(GuildFindGuildById, ReturnsExisting) {
    GuildLog log;
    log.guilds.push_back(create_guild(10, "X", 1));
    auto f = find_guild_by_id(log, 10);
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ((*f)->guild_id, 10u);
}

TEST(GuildFindGuildOfMember, FindsGuildForMember) {
    GuildLog log;
    auto& g = log.guilds.emplace_back(create_guild(10, "X", 1));
    add_member(g, make_member(1001, "A"));
    auto f = find_guild_of_member(log, 1001);
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ((*f)->guild_id, 10u);
}

TEST(GuildFindGuildOfMember, MasterAlsoMatched) {
    GuildLog log;
    log.guilds.push_back(create_guild(10, "X", 1000));
    auto f = find_guild_of_member(log, 1000);
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ((*f)->guild_id, 10u);
}
