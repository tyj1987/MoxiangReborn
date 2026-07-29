// skill_manager_test.cpp - 1:1 port tests for MugongManager + SkillManager.

#include "mxh/server/skill_manager.hpp"
#include "mxh/game/skill_types.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace ms = mxh::server;

namespace {

using mxh::server::MugongManager;
using mxh::server::MugongSlot;
using mxh::server::SkillManager;
using mxh::game::SkillInfo;
using mxh::game::SkillKind;

SkillInfo make_basic_skill(std::uint16_t idx, std::uint16_t phy) {
    SkillInfo s{};
    s.SkillIdx = idx;
    const char nm[] = "Skill";
    for (std::size_t i = 0; i < sizeof(nm); ++i) s.SkillName[i] = nm[i];
    s.SkillKind = static_cast<std::uint16_t>(SkillKind::OuterMugong);
    s.UpPhyAttack[0] = static_cast<float>(phy);
    s.AttackSuccessRate[0] = 100.0f;
    s.NeedNaeRyuk[0] = 10;
    return s;
}

}

TEST(MugongManager, AddAndFind) {
    MugongManager mgr(7);
    EXPECT_EQ(mgr.owner_id(), 7u);
    MugongSlot s1{}; s1.mugong_idx = 100; s1.level = 1; s1.sp = 5;
    MugongSlot s2{}; s2.mugong_idx = 200; s2.level = 1; s2.sp = 7;
    EXPECT_TRUE(mgr.set_slot(s1));
    EXPECT_TRUE(mgr.set_slot(s2));
    EXPECT_EQ(mgr.size(), 2u);
    auto* f = mgr.find(100);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->sp, 5u);
    EXPECT_EQ(mgr.find(999), nullptr);
}

TEST(MugongManager, UpdateExistingSlot) {
    MugongManager mgr;
    MugongSlot s1{}; s1.mugong_idx = 1; s1.level = 1;
    MugongSlot s2{}; s2.mugong_idx = 1; s2.level = 2;
    EXPECT_TRUE(mgr.set_slot(s1));
    EXPECT_TRUE(mgr.set_slot(s2));  // update
    EXPECT_EQ(mgr.size(), 1u);
    EXPECT_EQ(mgr.find(1)->level, 2u);
}

TEST(MugongManager, RemoveShrinksAndReindexes) {
    MugongManager mgr;
    MugongSlot s1{}; s1.mugong_idx = 1;
    MugongSlot s2{}; s2.mugong_idx = 2;
    MugongSlot s3{}; s3.mugong_idx = 3;
    mgr.set_slot(s1); mgr.set_slot(s2); mgr.set_slot(s3);
    EXPECT_TRUE(mgr.remove(2));
    EXPECT_EQ(mgr.size(), 2u);
    EXPECT_EQ(mgr.find(2), nullptr);  // not found
    EXPECT_EQ(mgr.find(2), nullptr);
    EXPECT_NE(mgr.find(1), nullptr);
    EXPECT_NE(mgr.find(3), nullptr);
}

TEST(MugongManager, EnforcesMaxSlot) {
    MugongManager mgr;
    for (std::uint8_t i = 0; i < mxh::server::MXH_MAX_MUGONG_SLOT; ++i) {
        MugongSlot s{}; s.mugong_idx = i + 1;
        EXPECT_TRUE(mgr.set_slot(s));
    }
    MugongSlot overflow{}; overflow.mugong_idx = 9999;
    EXPECT_FALSE(mgr.set_slot(overflow));
    EXPECT_EQ(mgr.size(), mxh::server::MXH_MAX_MUGONG_SLOT);
}

TEST(MugongManager, TotalSpSumsAllSlots) {
    MugongManager mgr;
    MugongSlot s1{}; s1.mugong_idx = 1; s1.sp = 5;
    MugongSlot s2{}; s2.mugong_idx = 2; s2.sp = 7;
    MugongSlot s3{}; s3.mugong_idx = 3; s3.sp = 3;
    mgr.set_slot(s1); mgr.set_slot(s2); mgr.set_slot(s3);
    EXPECT_EQ(mgr.total_sp(), 15u);
}

TEST(SkillManager, RegisterAndFind) {
    SkillManager mgr;
    mgr.register_skill(make_basic_skill(10, 50));
    mgr.register_skill(make_basic_skill(20, 30));
    EXPECT_EQ(mgr.size(), 2u);
    auto* s = mgr.find(10);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->UpPhyAttack[0], 50.0f);
    EXPECT_EQ(mgr.find(99), nullptr);
}

TEST(SkillManager, Level1AccessorsAreStable) {
    SkillManager mgr;
    mgr.register_skill(make_basic_skill(7, 88));
    EXPECT_EQ(mgr.phy_attack_lv1(7), 88u);
    EXPECT_EQ(mgr.att_attack_lv1(7), 0u);
    EXPECT_EQ(mgr.att_rate_lv1(7), 100u);
    EXPECT_EQ(mgr.naeryuk_lv1(7), 10u);
    // Unknown skill returns 0.
    EXPECT_EQ(mgr.phy_attack_lv1(99), 0u);
}





