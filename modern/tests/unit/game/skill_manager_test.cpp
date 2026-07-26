// mxh/tests/unit/game/skill_manager_test.cpp
// Unit tests for mxh::game::SkillManager (Phase D1.2 lookup class).
//
// Locks down the 1:1 surface:
//   * init() loads the D1.1 default skills (4 entries)
//   * get(skill_idx) returns the correct SkillInfo
//   * get() throws SkillNotFound on miss
//   * try_get() / exists() are non-throwing variants
//   * add() inserts new skills + throws on duplicate skill_idx
//   * clear() / size() work as expected
//   * skills() reflects insertion order

#include "mxh/game/skill_manager.hpp"
#include "mxh/game/skill_types.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using mxh::game::SkillInfo;
using mxh::game::SkillKind;
using mxh::game::SkillManager;
using mxh::game::SkillNotFound;

TEST(SkillManager, DefaultConstructedIsEmpty) {
    SkillManager mgr;
    EXPECT_EQ(mgr.size(), 0u);
    EXPECT_FALSE(mgr.exists(1));
}

TEST(SkillManager, InitLoadsFourDefaultSkills) {
    SkillManager mgr;
    mgr.init();
    EXPECT_EQ(mgr.size(), 4u);
    // The D1.1 placeholder skill IDs 1..4 must all be present.
    for (std::uint32_t id = 1; id <= 4; ++id) {
        EXPECT_TRUE(mgr.exists(id))
            << "skill_idx=" << id << " not in default table";
    }
}

TEST(SkillManager, GetReturnsCorrectSkill) {
    SkillManager mgr;
    mgr.init();
    const auto& s = mgr.get(1);
    EXPECT_EQ(s.skill_idx,  1u);
    EXPECT_EQ(s.name,       "BasicStrike");
    EXPECT_EQ(s.skill_kind, SkillKind::OuterMugong);
    EXPECT_EQ(s.phy_attack, 15u);
}

TEST(SkillManager, GetThrowsOnMiss) {
    SkillManager mgr;
    mgr.init();
    EXPECT_THROW(mgr.get(999), SkillNotFound);
    try {
        mgr.get(999);
        FAIL() << "expected SkillNotFound";
    } catch (const SkillNotFound& e) {
        EXPECT_EQ(e.skill_idx(), 999u);
    }
}

TEST(SkillManager, TryGetReturnsFalseOnMiss) {
    SkillManager mgr;
    mgr.init();
    SkillInfo out{};
    EXPECT_FALSE(mgr.try_get(999, out));
    // out must be untouched on miss.
    EXPECT_EQ(out.skill_idx, 0u);
    EXPECT_EQ(out.name, "");
}

TEST(SkillManager, TryGetPopulatesOnHit) {
    SkillManager mgr;
    mgr.init();
    SkillInfo out{};
    EXPECT_TRUE(mgr.try_get(3, out));
    EXPECT_EQ(out.skill_idx, 3u);
    EXPECT_EQ(out.name,      "HealSelf");
    EXPECT_EQ(out.skill_kind, SkillKind::Simbub);
}

TEST(SkillManager, ExistsIsFalseOnMiss) {
    SkillManager mgr;
    mgr.init();
    EXPECT_TRUE (mgr.exists(1));
    EXPECT_TRUE (mgr.exists(4));
    EXPECT_FALSE(mgr.exists(5));
    EXPECT_FALSE(mgr.exists(0));
    EXPECT_FALSE(mgr.exists(999));
}

TEST(SkillManager, AddInsertsNewSkill) {
    SkillManager mgr;
    SkillInfo s{};
    s.skill_idx   = 100;
    s.name        = "TestSkill";
    s.skill_kind  = SkillKind::Jinbub;
    s.skill_range = 5;
    s.phy_attack  = 99;
    mgr.add(s);
    EXPECT_EQ(mgr.size(), 1u);
    EXPECT_TRUE(mgr.exists(100));
    EXPECT_EQ(mgr.get(100).name, "TestSkill");
    EXPECT_EQ(mgr.get(100).phy_attack, 99u);
}

TEST(SkillManager, AddThrowsOnDuplicate) {
    SkillManager mgr;
    mgr.init();
    // skill_idx=1 (BasicStrike) is already in the default table.
    SkillInfo dup{};
    dup.skill_idx = 1;
    dup.name      = "BasicStrikeDuplicate";
    EXPECT_THROW(mgr.add(dup), std::invalid_argument);
    // Size unchanged.
    EXPECT_EQ(mgr.size(), 4u);
}

TEST(SkillManager, InitResetsTable) {
    SkillManager mgr;
    mgr.init();
    EXPECT_EQ(mgr.size(), 4u);
    mgr.add(SkillInfo{ .skill_idx = 999 });
    EXPECT_EQ(mgr.size(), 5u);
    mgr.init();   // resets
    EXPECT_EQ(mgr.size(), 4u);
    EXPECT_FALSE(mgr.exists(999));
}

TEST(SkillManager, ClearEmptiesTable) {
    SkillManager mgr;
    mgr.init();
    mgr.clear();
    EXPECT_EQ(mgr.size(), 0u);
    EXPECT_FALSE(mgr.exists(1));
}

TEST(SkillManager, SkillsAreInsertionOrder) {
    SkillManager mgr;
    mgr.init();
    const auto& v = mgr.skills();
    ASSERT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0].skill_idx, 1u);
    EXPECT_EQ(v[1].skill_idx, 2u);
    EXPECT_EQ(v[2].skill_idx, 3u);
    EXPECT_EQ(v[3].skill_idx, 4u);
}

TEST(SkillManager, GetReturnsReferenceIntoInternalStorage) {
    // 1:1 with the legacy CSkillManager* return type: get() must
    // return a reference, not a copy, so callers can mutate via the
    // handle (the modern port preserves this so D1.3 bin reloads
    // can update a skill in place).
    SkillManager mgr;
    mgr.init();
    auto& s = const_cast<SkillInfo&>(mgr.get(1));
    const auto* before = &s;
    auto& s2 = const_cast<SkillInfo&>(mgr.get(1));
    EXPECT_EQ(before, &s2);  // same address
}
