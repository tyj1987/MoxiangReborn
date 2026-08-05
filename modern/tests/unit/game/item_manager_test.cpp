// 1:1 tests for ItemManager + resolve_item_effect_with_manager.
//
// Covers the post-D6.x ItemManager surface (init_from_bin / add / get /
// try_get / exists / size / clear / duplicate detection) plus the R-8
// item_effects lookup path: when the manager has the row, real
// LifeRecover / NaeRyukRecover are read instead of the linear-scale
// placeholder.

#include "mxh/game/item_effects.hpp"
#include "mxh/game/item_list_types.hpp"
#include "mxh/game/item_manager.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <utility>

using mxh::game::ItemEffect;
using mxh::game::ItemInfo;
using mxh::game::ItemManager;
using mxh::game::ItemNotFound;
using mxh::game::resolve_item_effect;
using mxh::game::resolve_item_effect_with_manager;

namespace {
// Construct a 1-field ItemInfo.  Most tests only care about ItemIdx,
// LifeRecover, LifeRecoverRate, NaeRyukRecover, NaeRyukRecoverRate.
ItemInfo mk(std::uint16_t idx, std::uint16_t life, float life_rate,
              std::uint16_t mp, float mp_rate) {
    ItemInfo it{};
    it.ItemIdx = idx;
    it.LifeRecover = life;
    it.LifeRecoverRate = life_rate;
    it.NaeRyukRecover = mp;
    it.NaeRyukRecoverRate = mp_rate;
    return it;
}
}

TEST(ItemManager, DefaultsToEmpty) {
    ItemManager m;
    EXPECT_EQ(m.size(), 0u);
    EXPECT_FALSE(m.exists(1));
    ItemInfo out;
    EXPECT_FALSE(m.try_get(1, out));
}

TEST(ItemManager, AddUpdatesSizeAndIndex) {
    ItemManager m;
    m.add(mk(7, 100, 0.0f, 0, 0.0f));
    m.add(mk(13, 0, 0.0f, 50, 0.0f));
    EXPECT_EQ(m.size(), 2u);
    EXPECT_TRUE(m.exists(7));
    EXPECT_TRUE(m.exists(13));
}

TEST(ItemManager, DuplicateItemIdxRejected) {
    ItemManager m;
    m.add(mk(7, 100, 0.0f, 0, 0.0f));
    EXPECT_THROW(m.add(mk(7, 999, 0.0f, 0, 0.0f)), std::invalid_argument);
}

TEST(ItemManager, GetMissingItemThrows) {
    ItemManager m;
    m.add(mk(7, 100, 0.0f, 0, 0.0f));
    EXPECT_THROW(m.get(999), ItemNotFound);
}

TEST(ItemManager, TryGetReturnsFalseOnMiss) {
    ItemManager m;
    m.add(mk(7, 100, 0.0f, 0, 0.0f));
    ItemInfo out;
    EXPECT_FALSE(m.try_get(999, out));
    EXPECT_TRUE(m.try_get(7, out));
    EXPECT_EQ(out.ItemIdx, 7u);
    EXPECT_EQ(out.LifeRecover, 100u);
}

TEST(ItemManager, ClearEmptiesTable) {
    ItemManager m;
    m.add(mk(7, 100, 0.0f, 0, 0.0f));
    m.add(mk(8, 200, 0.0f, 0, 0.0f));
    m.clear();
    EXPECT_EQ(m.size(), 0u);
    EXPECT_FALSE(m.exists(7));
}

TEST(ItemManager, ItemsPreservesInsertionOrder) {
    ItemManager m;
    m.add(mk(5, 100, 0.0f, 0, 0.0f));
    m.add(mk(2, 200, 0.0f, 0, 0.0f));
    m.add(mk(9, 300, 0.0f, 0, 0.0f));
    auto& v = m.items();
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0].ItemIdx, 5u);
    EXPECT_EQ(v[1].ItemIdx, 2u);
    EXPECT_EQ(v[2].ItemIdx, 9u);
}

// --- resolve_item_effect_with_manager (R-8) ---

TEST(ItemEffectWithManager, EmptyManagerFallsBackToLinearScale) {
    ItemManager m;
    // No rows -- should match the legacy placeholder behaviour.
    auto e_mgr = resolve_item_effect_with_manager(50, m);
    auto e_legacy = resolve_item_effect(50);
    EXPECT_EQ(e_mgr.hp_delta, e_legacy.hp_delta);
    EXPECT_EQ(e_mgr.mp_delta, e_legacy.mp_delta);
}

TEST(ItemEffectWithManager, MissingItemFallsBackToLinearScale) {
    ItemManager m;
    // Manager has 1 row but we query a different idx.
    m.add(mk(1234, 99, 0.0f, 99, 0.0f));
    auto e_mgr = resolve_item_effect_with_manager(50, m);
    auto e_legacy = resolve_item_effect(50);
    EXPECT_EQ(e_mgr.hp_delta, e_legacy.hp_delta);
}

TEST(ItemEffectWithManager, RealLifeRecoverFromManager) {
    ItemManager m;
    // Slot 1: idx=10, LifeRecover=100, no rate.
    m.add(mk(10, 100, 0.0f, 0, 0.0f));
    auto e = resolve_item_effect_with_manager(10, m);
    EXPECT_EQ(e.hp_delta, 100);
    EXPECT_EQ(e.mp_delta, 0);
}

TEST(ItemEffectWithManager, RealNaeRyukRecoverFromManager) {
    ItemManager m;
    m.add(mk(20, 0, 0.0f, 75, 0.0f));
    auto e = resolve_item_effect_with_manager(20, m);
    EXPECT_EQ(e.hp_delta, 0);
    EXPECT_EQ(e.mp_delta, 75);
}

TEST(ItemEffectWithManager, RateFieldProducesExtraDelta) {
    ItemManager m;
    // LifeRecover=50, LifeRecoverRate=0.5 -> 50 + 0.5*10000 = 5050
    m.add(mk(30, 50, 0.5f, 0, 0.0f));
    auto e = resolve_item_effect_with_manager(30, m);
    EXPECT_EQ(e.hp_delta, 5050);
}
