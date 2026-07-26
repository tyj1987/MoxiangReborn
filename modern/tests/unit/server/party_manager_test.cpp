// party_manager_test.cpp

#include "mxh/server/party_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::Party;
using mxh::server::PartyMember;
using mxh::server::PartyLog;
using mxh::server::create_party;
using mxh::server::add_member;
using mxh::server::remove_member;
using mxh::server::change_master;
using mxh::server::is_party_member;
using mxh::server::find_member_index;
using mxh::server::online_member_count;
using mxh::server::first_online_member_id;
using mxh::server::mark_member_logged_in;
using mxh::server::mark_member_logged_out;
using mxh::server::set_member_level;
using mxh::server::start_master_request;
using mxh::server::master_request_timed_out;
using mxh::server::find_party_by_id;
using mxh::server::find_party_of_player;
using mxh::server::disband_if_empty;

static std::string N(std::uint32_t i) { return "name" + std::to_string(i); }
static Party make_party_filled(std::uint32_t id, std::uint32_t count) {
    Party p = create_party(id, 1000, "Master", 50);
    for (std::uint32_t i = 1; i < count; ++i) {
        add_member(p, 1000 + i, N(i), 50);
    }
    return p;
}
}

// ---- create_party ----
TEST(CreateParty, MasterIsSlot0) {
    auto p = create_party(1, 1001, "Alice", 30, 0);
    EXPECT_EQ(p.party_id, 1u);
    EXPECT_EQ(p.master_id, 1001u);
    EXPECT_EQ(p.member_count, 1u);
    EXPECT_EQ(p.members[0].member_id, 1001u);
    EXPECT_EQ(p.members[0].level, 30u);
    EXPECT_TRUE(p.members[0].logged_in);
    // Name copied with NUL terminator
    EXPECT_EQ(p.members[0].name[0], 'A');
}

TEST(CreateParty, EmptyOptionIs0) {
    auto p = create_party(1, 1, "X", 1);
    EXPECT_EQ(p.option, 0u);
}

// ---- add_member ----
TEST(AddMember, UpToMax) {
    auto p = create_party(1, 1000, "M", 10);
    EXPECT_TRUE(add_member(p, 1001, "A1", 11));
    EXPECT_TRUE(add_member(p, 1002, "A2", 12));
    EXPECT_EQ(p.member_count, 3u);
}

TEST(AddMember, BeyondMaxFails) {
    auto pp = make_party_filled(1, mxh::server::kPartyMaxMembers); auto& p = pp;
    EXPECT_FALSE(add_member(p, 9999, "X", 1));
    EXPECT_EQ(p.member_count, mxh::server::kPartyMaxMembers);
}

TEST(AddMember, DuplicateFails) {
    auto p = create_party(1, 1000, "M", 10);
    add_member(p, 1001, "A", 11);
    EXPECT_FALSE(add_member(p, 1001, "A", 11));
    EXPECT_EQ(p.member_count, 2u);
}

// ---- remove_member ----
TEST(RemoveMember, RemovesAndShiftsDown) {
    auto p = make_party_filled(1, 4);  // slots 1000..1003
    EXPECT_TRUE(remove_member(p, 1001));
    EXPECT_EQ(p.member_count, 3u);
    EXPECT_EQ(p.members[0].member_id, 1000u);
    EXPECT_EQ(p.members[1].member_id, 1002u);
    EXPECT_EQ(p.members[2].member_id, 1003u);
}

TEST(RemoveMember, MasterRemovalPromotesNext) {
    auto p = make_party_filled(1, 3);  // 1000 master, 1001, 1002
    EXPECT_TRUE(remove_member(p, 1000));
    EXPECT_EQ(p.master_id, 1001u);  // slot[0] promoted
    EXPECT_EQ(p.member_count, 2u);
}

TEST(RemoveMember, UnknownReturnsFalse) {
    auto p = create_party(1, 1000, "M", 1);
    EXPECT_FALSE(remove_member(p, 9999));
    EXPECT_EQ(p.member_count, 1u);
}

// ---- change_master ----
TEST(ChangeMaster, AssignsNewMaster) {
    auto p = make_party_filled(1, 3);
    EXPECT_TRUE(change_master(p, 1002));
    EXPECT_EQ(p.master_id, 1002u);
    EXPECT_FALSE(p.master_changing);  // change_master clears the flag
}

TEST(ChangeMaster, NonMemberFails) {
    auto p = make_party_filled(1, 3);
    EXPECT_FALSE(change_master(p, 9999));
}

TEST(ChangeMaster, ZeroFails) {
    auto p = make_party_filled(1, 3);
    EXPECT_FALSE(change_master(p, 0));
}

// ---- queries ----
TEST(IsPartyMember, ReturnsTrueForMembers) {
    auto p = make_party_filled(1, 3);
    EXPECT_TRUE(is_party_member(p, 1000));
    EXPECT_TRUE(is_party_member(p, 1002));
}

TEST(IsPartyMember, ReturnsFalseForZero) {
    auto p = create_party(1, 1000, "M", 1);
    EXPECT_FALSE(is_party_member(p, 0));
}

TEST(FindMemberIndex, ReturnsSlotIndex) {
    auto p = make_party_filled(1, 3);
    auto idx = find_member_index(p, 1002);
    ASSERT_TRUE(idx.has_value());
    EXPECT_EQ(*idx, 2u);
}

TEST(FindMemberIndex, MissingReturnsNullopt) {
    auto p = make_party_filled(1, 3);
    EXPECT_FALSE(find_member_index(p, 9999).has_value());
}

TEST(OnlineMemberCount, AllLoggedIn) {
    auto p = make_party_filled(1, 4);
    EXPECT_EQ(online_member_count(p), 4u);
}

TEST(OnlineMemberCount, OneLoggedOut) {
    auto p = make_party_filled(1, 4);
    mark_member_logged_out(p, 1002);
    EXPECT_EQ(online_member_count(p), 3u);
}

TEST(FirstOnlineMemberId, ReturnsFirstSlotLoggedIn) {
    auto p = make_party_filled(1, 3);
    mark_member_logged_out(p, 1000);  // master offline
    mark_member_logged_out(p, 1001);  // 1001 offline
    EXPECT_EQ(first_online_member_id(p), 1002u);
}

TEST(FirstOnlineMemberId, NoneOnline) {
    auto p = make_party_filled(1, 3);
    for (std::uint32_t i = 0; i < 3; ++i) mark_member_logged_out(p, 1000 + i);
    EXPECT_EQ(first_online_member_id(p), 0u);
}

// ---- login state ----
TEST(MarkLoggedIn, UpdatesPercent) {
    auto p = make_party_filled(1, 3);
    mark_member_logged_out(p, 1002);
    mark_member_logged_in(p, 1002, 80, 60, 40);
    EXPECT_EQ(p.members[2].life_percent, 80u);
    EXPECT_EQ(p.members[2].shield_percent, 60u);
    EXPECT_EQ(p.members[2].naeryuk_percent, 40u);
    EXPECT_TRUE(p.members[2].logged_in);
}

TEST(MarkLoggedOut, ClearsPercent) {
    auto p = make_party_filled(1, 3);
    mark_member_logged_out(p, 1001);
    EXPECT_FALSE(p.members[1].logged_in);
    EXPECT_EQ(p.members[1].life_percent, 0u);
}

TEST(SetMemberLevel, UpdatesLevel) {
    auto p = make_party_filled(1, 3);
    set_member_level(p, 1001, 99);
    EXPECT_EQ(p.members[1].level, 99u);
}

// ---- master-request machinery ----
TEST(MasterRequest, StartSetsState) {
    auto p = make_party_filled(1, 3);
    start_master_request(p, 1001, 12345);
    EXPECT_TRUE(p.master_changing);
    EXPECT_EQ(p.request_player_id, 1001u);
    EXPECT_EQ(p.request_process_time_ms, 12345u);
}

TEST(MasterRequest, NotTimedOutBeforeWindow) {
    auto p = make_party_filled(1, 3);
    start_master_request(p, 1001, 0);
    EXPECT_FALSE(master_request_timed_out(p, 5000));  // < 10s
    EXPECT_TRUE(p.master_changing);
}

TEST(MasterRequest, TimedOutAfter10Sec) {
    auto p = make_party_filled(1, 3);
    start_master_request(p, 1001, 0);
    EXPECT_TRUE(master_request_timed_out(p, 11000));
    EXPECT_FALSE(p.master_changing);
    EXPECT_EQ(p.request_player_id, 0u);
}

// ---- PartyLog ----
TEST(FindPartyById, ReturnsExisting) {
    PartyLog log;
    auto& p = log.parties.emplace_back(create_party(10, 1, "A", 1));
    auto found = find_party_by_id(log, 10);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ((*found)->party_id, 10u);
}

TEST(FindPartyById, MissingReturnsNullopt) {
    PartyLog log;
    EXPECT_FALSE(find_party_by_id(log, 99).has_value());
}

TEST(FindPartyOfPlayer, FindsPartyForMember) {
    PartyLog log;
    log.parties.emplace_back(create_party(10, 1000, "A", 1));
    add_member(log.parties[0], 1001, "B", 1);
    auto found = find_party_of_player(log, 1001);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ((*found)->party_id, 10u);
}

TEST(DisbandIfEmpty, RemovesEmptyParty) {
    PartyLog log;
    log.parties.emplace_back(create_party(10, 1, "A", 1));
    log.parties[0].member_count = 0;
    EXPECT_TRUE(disband_if_empty(log, 10));
    EXPECT_EQ(log.parties.size(), 0u);
}

TEST(DisbandIfEmpty, NonEmptyFails) {
    PartyLog log;
    log.parties.emplace_back(create_party(10, 1, "A", 1));
    EXPECT_FALSE(disband_if_empty(log, 10));
    EXPECT_EQ(log.parties.size(), 1u);
}
