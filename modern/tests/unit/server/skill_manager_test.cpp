//
// Tests for mxh::server::MugongManager + SkillManager (Phase D1 1:1 lock).
//
// Covers the 1:1 surface needed to lock legacy gameplay:
//   * MugongSlot layout (mugong_idx / exp / level / sp / db_idx / kind)
//   * MugongManager: add / find / update existing / remove / max-slot
//   * MugongManager: total_sp sums all slots
//   * MugongManager: owner_id round-trip via ctor + set_owner_id
//   * MugongManager: clear() resets slots + idx_ (regression,
//     pre-fix only cleared slots_ and stale idx_ caused OOB on find)
//   * SkillManager: register / find / re-register (in-place update)
//   * SkillManager: level-1 accessors (phy_attack_lv1 / att_attack_lv1 /
//     att_rate_lv1 / naeryuk_lv1) + 0 on unknown skill
//   * SkillManager: skills() vector order is insertion order
//   * SkillManager: size() tracks unique registrations
//

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

}  // namespace

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

TEST(MugongManager, OwnerIdDefaultZero) {
    MugongManager mgr;
    EXPECT_EQ(mgr.owner_id(), 0u);
}

TEST(MugongManager, SetOwnerIdRoundTrip) {
    MugongManager mgr;
    mgr.set_owner_id(42);
    EXPECT_EQ(mgr.owner_id(), 42u);
    mgr.set_owner_id(0);
    EXPECT_EQ(mgr.owner_id(), 0u);
}

TEST(MugongManager, ClearResetsSlotsAndIndex) {
    // Regression: pre-fix clear() left idx_ pointing past the end of
    // slots_, causing vector subscript OOB on find().
    MugongManager mgr(123);
    MugongSlot s1{}; s1.mugong_idx = 1; s1.sp = 5;
    MugongSlot s2{}; s2.mugong_idx = 2; s2.sp = 7;
    mgr.set_slot(s1);
    mgr.set_slot(s2);
    EXPECT_EQ(mgr.size(), 2u);

    mgr.clear();
    EXPECT_EQ(mgr.size(), 0u);
    EXPECT_EQ(mgr.find(1), nullptr);
    EXPECT_EQ(mgr.find(2), nullptr);
    EXPECT_EQ(mgr.total_sp(), 0u);
    EXPECT_EQ(mgr.owner_id(), 123u);  // owner_id preserved

    // Reuse after clear: index must be reset, not stale.
    MugongSlot s3{}; s3.mugong_idx = 99; s3.sp = 11;
    EXPECT_TRUE(mgr.set_slot(s3));
    EXPECT_EQ(mgr.size(), 1u);
    auto* f = mgr.find(99);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->sp, 11u);
}

TEST(MugongManager, RemoveMissingReturnsFalse) {
    MugongManager mgr;
    EXPECT_FALSE(mgr.remove(99));
    MugongSlot s1{}; s1.mugong_idx = 1;
    mgr.set_slot(s1);
    EXPECT_FALSE(mgr.remove(99));
    EXPECT_EQ(mgr.size(), 1u);
}

TEST(MugongManager, FindConstReturnsSameValue) {
    MugongManager mgr(5);
    MugongSlot s1{}; s1.mugong_idx = 1; s1.level = 3;
    mgr.set_slot(s1);

    const MugongManager& cmgr = mgr;
    const auto* f = cmgr.find(1);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->level, 3u);
    EXPECT_EQ(cmgr.find(99), nullptr);
}

TEST(MugongManager, SlotsVectorReturnsAllInInsertionOrder) {
    MugongManager mgr;
    MugongSlot s1{}; s1.mugong_idx = 1;
    MugongSlot s2{}; s2.mugong_idx = 2;
    MugongSlot s3{}; s3.mugong_idx = 3;
    mgr.set_slot(s1); mgr.set_slot(s2); mgr.set_slot(s3);
    const auto& slots = mgr.slots();
    ASSERT_EQ(slots.size(), 3u);
    EXPECT_EQ(slots[0].mugong_idx, 1u);
    EXPECT_EQ(slots[1].mugong_idx, 2u);
    EXPECT_EQ(slots[2].mugong_idx, 3u);
}

TEST(MugongManager, TotalSpOnEmptyManagerIsZero) {
    MugongManager mgr;
    EXPECT_EQ(mgr.total_sp(), 0u);
}

TEST(MugongManager, MaxSlotConstantIsLegacyHundred) {
    // 1:1 with legacy CSkillManager::MAX_MUGONG_SLOT = 100.
    EXPECT_EQ(mxh::server::MXH_MAX_MUGONG_SLOT, 100);
}

TEST(MugongManager, UpdatePreservesPositionInVector) {
    MugongManager mgr;
    MugongSlot s1{}; s1.mugong_idx = 1;
    MugongSlot s2{}; s2.mugong_idx = 2;
    MugongSlot s3{}; s3.mugong_idx = 3;
    mgr.set_slot(s1); mgr.set_slot(s2); mgr.set_slot(s3);
    MugongSlot s2upd{}; s2upd.mugong_idx = 2; s2upd.level = 9;
    mgr.set_slot(s2upd);
    const auto& slots = mgr.slots();
    ASSERT_EQ(slots.size(), 3u);
    EXPECT_EQ(slots[0].mugong_idx, 1u);
    EXPECT_EQ(slots[1].mugong_idx, 2u);
    EXPECT_EQ(slots[1].level, 9u);
    EXPECT_EQ(slots[2].mugong_idx, 3u);
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

TEST(SkillManager, ReRegisterSameSkillIdxUpdatesInPlace) {
    SkillManager mgr;
    mgr.register_skill(make_basic_skill(5, 100));
    mgr.register_skill(make_basic_skill(10, 200));
    EXPECT_EQ(mgr.size(), 2u);

    SkillInfo updated = make_basic_skill(5, 999);
    updated.SkillKind = static_cast<std::uint16_t>(SkillKind::InnerMugong);
    mgr.register_skill(updated);
    EXPECT_EQ(mgr.size(), 2u);  // still 2 unique skills
    auto* s = mgr.find(5);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->UpPhyAttack[0], 999.0f);
}

TEST(SkillManager, SkillsVectorPreservesInsertionOrder) {
    SkillManager mgr;
    mgr.register_skill(make_basic_skill(30, 30));
    mgr.register_skill(make_basic_skill(10, 10));
    mgr.register_skill(make_basic_skill(20, 20));
    const auto& skills = mgr.skills();
    ASSERT_EQ(skills.size(), 3u);
    EXPECT_EQ(skills[0].SkillIdx, 30);
    EXPECT_EQ(skills[1].SkillIdx, 10);
    EXPECT_EQ(skills[2].SkillIdx, 20);
}

TEST(SkillManager, EmptyManagerReturnsZeroForAllAccessors) {
    SkillManager mgr;
    EXPECT_EQ(mgr.size(), 0u);
    EXPECT_EQ(mgr.find(0), nullptr);
    EXPECT_EQ(mgr.phy_attack_lv1(0), 0u);
    EXPECT_EQ(mgr.att_attack_lv1(0), 0u);
    EXPECT_EQ(mgr.att_rate_lv1(0), 0u);
    EXPECT_EQ(mgr.naeryuk_lv1(0), 0u);
}

TEST(SkillManager, FindReturnsNullForUnknownSkill) {
    SkillManager mgr;
    mgr.register_skill(make_basic_skill(1, 100));
    // 1 is registered, so find(1) returns the slot.
    auto* s = mgr.find(1);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->UpPhyAttack[0], 100.0f);
    // 0 / 99 are NOT registered -> nullptr.
    EXPECT_EQ(mgr.find(0), nullptr);
    EXPECT_EQ(mgr.find(99), nullptr);
}

TEST(SkillManager, Level1AccessorsPullArrayIndexZero) {
    // 1:1 with legacy: lv1 stats are at array index 0 of per-level arrays.
    SkillManager mgr;
    SkillInfo s = make_basic_skill(11, 100);
    s.UpPhyAttack[0] = 50.0f;
    s.UpPhyAttack[1] = 999.0f;  // index 1 must NOT be returned
    s.FirstAttAttack[0] = 33.0f;
    s.FirstAttAttack[1] = 999.0f;
    s.AttackSuccessRate[0] = 80.0f;
    s.AttackSuccessRate[1] = 999.0f;
    s.NeedNaeRyuk[0] = 7;
    s.NeedNaeRyuk[1] = 999;
    mgr.register_skill(s);
    EXPECT_EQ(mgr.phy_attack_lv1(11), 50u);
    EXPECT_EQ(mgr.att_attack_lv1(11), 33u);
    EXPECT_EQ(mgr.att_rate_lv1(11), 80u);
    EXPECT_EQ(mgr.naeryuk_lv1(11), 7u);
}
