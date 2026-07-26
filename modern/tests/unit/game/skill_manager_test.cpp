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
    EXPECT_EQ(s.SkillIdx, 1u);
    EXPECT_STREQ(s.SkillName, "BasicStrike");
    EXPECT_EQ(s.SkillKind, static_cast<std::uint16_t>(SkillKind::OuterMugong));
    EXPECT_EQ(s.UpPhyAttack[0], 15.0f);
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
    EXPECT_EQ(out.SkillIdx, 0u);
    EXPECT_STREQ(out.SkillName, "");
}

TEST(SkillManager, TryGetPopulatesOnHit) {
    SkillManager mgr;
    mgr.init();
    SkillInfo out{};
    EXPECT_TRUE(mgr.try_get(3, out));
    EXPECT_EQ(out.SkillIdx, 3u);
    EXPECT_STREQ(out.SkillName, "HealSelf");
    EXPECT_EQ(out.SkillKind, static_cast<std::uint16_t>(SkillKind::Simbub));
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
    s.SkillIdx   = 100;
    const char name[] = "TestSkill";
    for (std::size_t i = 0; i < sizeof(name); ++i) s.SkillName[i] = name[i];
    s.SkillKind    = static_cast<std::uint16_t>(SkillKind::Jinbub);
    s.SkillRange   = 5;
    s.UpPhyAttack[0] = 99.0f;
    mgr.add(s);
    EXPECT_EQ(mgr.size(), 1u);
    EXPECT_TRUE(mgr.exists(100));
    EXPECT_STREQ(mgr.get(100).SkillName, "TestSkill");
    EXPECT_EQ(mgr.get(100).UpPhyAttack[0], 99.0f);
}

TEST(SkillManager, AddThrowsOnDuplicate) {
    SkillManager mgr;
    mgr.init();
    // skill_idx=1 (BasicStrike) is already in the default table.
    SkillInfo dup{};
    dup.SkillIdx = 1;
    const char name[] = "BasicStrikeDuplicate";
    for (std::size_t i = 0; i < sizeof(name); ++i) dup.SkillName[i] = name[i];
    EXPECT_THROW(mgr.add(dup), std::invalid_argument);
    // Size unchanged.
    EXPECT_EQ(mgr.size(), 4u);
}

TEST(SkillManager, InitResetsTable) {
    SkillManager mgr;
    mgr.init();
    EXPECT_EQ(mgr.size(), 4u);
    SkillInfo extra{};
    extra.SkillIdx = 999;
    mgr.add(extra);
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
    EXPECT_EQ(v[0].SkillIdx, 1u);
    EXPECT_EQ(v[1].SkillIdx, 2u);
    EXPECT_EQ(v[2].SkillIdx, 3u);
    EXPECT_EQ(v[3].SkillIdx, 4u);
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
