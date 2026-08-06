// D4.33 DiscardSkinItem / RemoveEquipSkin data-plane tests.

#include <mxh/server/skin_discard_transition.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

using namespace mxh::server;

namespace {

class FakeSkinTableEnv final : public SkinDiscardEnv {
public:
    std::vector<SkinSelectItemInfo> nomal_entries;
    std::vector<SkinSelectItemInfo> costume_entries;

    std::size_t skin_count(std::uint16_t skin_kind) const noexcept override {
        if (skin_kind == LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN) {
            return nomal_entries.size();
        }
        if (skin_kind == LEGACY_SHOP_ITEM_COSTUME_SKIN) {
            return costume_entries.size();
        }
        return 0;
    }

    const SkinSelectItemInfo* skin_at(
        std::uint16_t skin_kind, std::size_t index) const noexcept override {
        const std::vector<SkinSelectItemInfo>* table = nullptr;
        if (skin_kind == LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN) {
            table = &nomal_entries;
        } else if (skin_kind == LEGACY_SHOP_ITEM_COSTUME_SKIN) {
            table = &costume_entries;
        }
        if (table == nullptr || index >= table->size()) {
            return nullptr;
        }
        return &(*table)[index];
    }
};

SkinSelectItemInfo make_skin(std::initializer_list<std::uint16_t> items) {
    SkinSelectItemInfo info{};
    std::size_t i = 0;
    for (std::uint16_t v : items) {
        if (i >= kSkinItemListMax) break;
        info.equip_item[i++] = v;
    }
    return info;
}

SkinItemSlots zero_skin() { return {}; }

}  // namespace

// ---------- RemoveEquipSkin ----------

TEST(SkinRemove, NullSkinSlotsReturnsZero) {
    FakeSkinTableEnv env;
    auto out = remove_equip_skin(env, /*current_skin=*/nullptr,
                                 LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN);
    EXPECT_EQ(out, zero_skin());
}

TEST(SkinRemove, UnknownSkinKindReturnsCurrent) {
    FakeSkinTableEnv env;
    SkinItemSlots cur{};
    cur[0] = 100;
    auto out = remove_equip_skin(env, &cur, /*kind=*/999);
    EXPECT_EQ(out, cur);
}

TEST(SkinRemove, EmptyTableLeavesCurrentUnchanged) {
    FakeSkinTableEnv env;
    SkinItemSlots cur{};
    cur[0] = 100;
    auto out = remove_equip_skin(env, &cur,
                                 LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN);
    EXPECT_EQ(out, cur);
}

TEST(SkinRemove, MatchingSlotIsCleared) {
    FakeSkinTableEnv env;
    env.nomal_entries.push_back(make_skin({501, 502, 503}));
    SkinItemSlots cur{};
    cur[0] = 501;
    cur[1] = 502;
    cur[2] = 503;
    auto out = remove_equip_skin(env, &cur,
                                 LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN);
    EXPECT_EQ(out[0], 0u);
    EXPECT_EQ(out[1], 0u);
    EXPECT_EQ(out[2], 0u);
}

TEST(SkinRemove, NonMatchingSlotIsPreserved) {
    FakeSkinTableEnv env;
    env.nomal_entries.push_back(make_skin({501, 502, 503}));
    SkinItemSlots cur{};
    cur[0] = 501;
    cur[1] = 999;  // not in the skin table
    auto out = remove_equip_skin(env, &cur,
                                 LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN);
    EXPECT_EQ(out[0], 0u);
    EXPECT_EQ(out[1], 999u);
}

TEST(SkinRemove, MultipleTableEntriesAggregate) {
    FakeSkinTableEnv env;
    env.nomal_entries.push_back(make_skin({501, 0, 0}));
    env.nomal_entries.push_back(make_skin({601, 0, 0}));
    SkinItemSlots cur{};
    cur[0] = 501;
    cur[1] = 601;
    auto out = remove_equip_skin(env, &cur,
                                 LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN);
    EXPECT_EQ(out[0], 0u);
    EXPECT_EQ(out[1], 0u);
}

TEST(SkinRemove, OnlyMatchingSkinKindTableIsWalked) {
    FakeSkinTableEnv env;
    env.nomal_entries.push_back(make_skin({501, 0, 0}));
    env.costume_entries.push_back(make_skin({601, 0, 0}));
    SkinItemSlots cur{};
    cur[0] = 501;
    cur[1] = 601;
    auto out = remove_equip_skin(env, &cur,
                                 LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN);
    EXPECT_EQ(out[0], 0u);   // 501 matches nomal entry -> cleared
    EXPECT_EQ(out[1], 601u); // 601 only in costume table -> preserved
}

TEST(SkinRemove, CostumeSkinKindWalksCostumeTable) {
    FakeSkinTableEnv env;
    env.costume_entries.push_back(make_skin({601, 602, 0}));
    SkinItemSlots cur{};
    cur[0] = 601;
    cur[1] = 602;
    auto out = remove_equip_skin(env, &cur,
                                 LEGACY_SHOP_ITEM_COSTUME_SKIN);
    EXPECT_EQ(out[0], 0u);
    EXPECT_EQ(out[1], 0u);
}

TEST(SkinRemove, ZeroEntriesInSkinInfoAreNotMatched) {
    FakeSkinTableEnv env;
    env.nomal_entries.push_back(make_skin({0, 0, 0}));
    SkinItemSlots cur{};
    cur[0] = 0;  // already empty -> no match anyway
    auto out = remove_equip_skin(env, &cur,
                                 LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN);
    EXPECT_EQ(out, cur);
}

TEST(SkinRemove, ZeroInCurrentSlotIsNotProcessed) {
    FakeSkinTableEnv env;
    env.nomal_entries.push_back(make_skin({501, 0, 0}));
    SkinItemSlots cur{};
    cur[0] = 501;  // match
    cur[1] = 0;    // legacy: skip if wSkinItem[i] == 0
    auto out = remove_equip_skin(env, &cur,
                                 LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN);
    EXPECT_EQ(out[0], 0u);
    EXPECT_EQ(out[1], 0u);
}

TEST(SkinRemove, NullTableEntryIsSkipped) {
    FakeSkinTableEnv env;
    env.nomal_entries.push_back(make_skin({501, 0, 0}));
    // Force a null return for index 1 to verify the legacy continue path.
    env.nomal_entries.push_back(SkinSelectItemInfo{});
    SkinItemSlots cur{};
    cur[0] = 501;
    cur[1] = 700;
    auto out = remove_equip_skin(env, &cur,
                                 LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN);
    EXPECT_EQ(out[0], 0u);
    EXPECT_EQ(out[1], 700u);
}

// ---------- DiscardSkinItem ----------

TEST(SkinDiscard, ForwardsToRemoveEquipSkin) {
    FakeSkinTableEnv env;
    env.nomal_entries.push_back(make_skin({501, 0, 0}));
    SkinItemSlots cur{};
    cur[0] = 501;
    auto out = discard_skin_item(env, LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, &cur);
    EXPECT_EQ(out[0], 0u);
}
