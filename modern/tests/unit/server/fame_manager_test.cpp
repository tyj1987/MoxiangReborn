// fame_manager_test.cpp - Phase D6 FameManager 1:1 port tests.

#include "mxh/server/fame_manager.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::apply_fame_delta;
using mxh::server::apply_bad_fame_delta;
using mxh::server::is_time_to_fame_update;
using mxh::server::FameCase;
using mxh::server::BadFameKind;
using mxh::server::FameUpdateClock;
using mxh::server::MUNPA_MASTER;

TEST(FameDelta, BeMasterIs30) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeMaster), 130u);
}
TEST(FameDelta, BeMemberIs10) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeMember), 110u);
}
TEST(FameDelta, BeMembertoSeniorIs5) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeMembertoSenior), 105u);
}
TEST(FameDelta, BeMembertoViceMasterIs15) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeMembertoViceMaster), 115u);
}
TEST(FameDelta, BeSeniortoViceMasterIs10) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeSeniortoViceMaster), 110u);
}
TEST(FameDelta, BeSeniortoMemberSubtracts10) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeSeniortoMember), 90u);
}
TEST(FameDelta, BeViceMastertoSeniorSubtracts15) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeViceMastertoSenior), 85u);
}
TEST(FameDelta, BeViceMastertoMemberSubtracts25) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeViceMastertoMember), 75u);
}
TEST(FameDelta, BeVicemastertoNotmemberSubtracts25) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeVicemastertoNotmember), 75u);
}
TEST(FameDelta, BeSeniortoNotmemberSubtracts20) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeSeniortoNotmember), 80u);
}
TEST(FameDelta, BeMembertoNotmemberSubtracts15) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BeMembertoNotmember), 85u);
}
TEST(FameDelta, BreakupMasterSubtracts70) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BreakupMaster), 30u);
}
TEST(FameDelta, BreakupViceMasterSubtracts30) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BreakupViceMaster), 70u);
}
TEST(FameDelta, BreakupSeniorSubtracts20) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BreakupSenior), 80u);
}
TEST(FameDelta, BreakupMemberSubtracts15) {
    EXPECT_EQ(apply_fame_delta(100u, FameCase::BreakupMember), 85u);
}
TEST(FameDelta, BreakupSentinelValueIsCorrect) {
    EXPECT_EQ(static_cast<std::uint32_t>(FameCase::BreakupMaster),
              50u + static_cast<std::uint32_t>(MUNPA_MASTER));
}
TEST(BadFameDelta, PkModeOnAdds1) {
    EXPECT_EQ(apply_bad_fame_delta(0, BadFameKind::PkModeOn), 1);
}
TEST(BadFameDelta, AttackAdds5) {
    EXPECT_EQ(apply_bad_fame_delta(10, BadFameKind::Attack), 15);
}
TEST(BadFameDelta, KillAdds5) {
    EXPECT_EQ(apply_bad_fame_delta(10, BadFameKind::Kill), 15);
}
TEST(BadFameDelta, BailSubtracts500) {
    EXPECT_EQ(apply_bad_fame_delta(100, BadFameKind::Bail), -400);
}

TEST(FameTick, FirstUpdateReturnsTrue) {
    FameUpdateClock c{};
    c.start_update_hour = 4;
    EXPECT_TRUE(is_time_to_fame_update(c, 5, 5));
    EXPECT_TRUE(c.is_updated);
    EXPECT_EQ(c.updated_day, 5u);
}
TEST(FameTick, SameDayReturnsFalse) {
    FameUpdateClock c{};
    c.start_update_hour = 4;
    is_time_to_fame_update(c, 5, 5);
    EXPECT_FALSE(is_time_to_fame_update(c, 5, 6));
}
TEST(FameTick, NewDayReturnsTrue) {
    FameUpdateClock c{};
    c.start_update_hour = 4;
    is_time_to_fame_update(c, 5, 5);
    EXPECT_TRUE(is_time_to_fame_update(c, 6, 5));
}
TEST(FameTick, BeforeStartHourReturnsFalse) {
    FameUpdateClock c{};
    c.start_update_hour = 20;
    EXPECT_FALSE(is_time_to_fame_update(c, 1, 5));
}
TEST(FameTick, ExactlyStartHourReturnsTrue) {
    FameUpdateClock c{};
    c.start_update_hour = 4;
    EXPECT_TRUE(is_time_to_fame_update(c, 1, 4));
}

}  // namespace
