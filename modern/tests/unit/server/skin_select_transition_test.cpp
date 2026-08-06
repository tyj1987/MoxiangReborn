// D4.32 PutSkinSelectItem data-plane tests.
// Locks the validation + per-slot mapping + costume-dress shoes override
// in 1:1 with legacy [Server]Map/ShopItemManager.cpp:2638-2704.

#include <mxh/server/skin_select_transition.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <unordered_map>

using namespace mxh::server;
using namespace mxh::server;

namespace {

class FakeSkinEnv final : public SkinSelectEnv {
public:
    std::unordered_map<std::uint64_t, SkinSelectItemInfo> skins;
    std::unordered_map<std::uint16_t, SkinItemInfoView> item_infos;

    static std::uint64_t key(std::uint16_t kind, std::uint32_t index) {
        return (static_cast<std::uint64_t>(kind) << 32) |
               static_cast<std::uint64_t>(index);
    }

    const SkinSelectItemInfo* find_skin(
        std::uint16_t skin_kind,
        std::uint32_t skin_index) const noexcept override {
        auto it = skins.find(key(skin_kind, skin_index));
        return it != skins.end() ? &it->second : nullptr;
    }

    const SkinItemInfoView* find_item_info(
        std::uint16_t item_idx) const noexcept override {
        auto it = item_infos.find(item_idx);
        return it != item_infos.end() ? &it->second : nullptr;
    }
};

SkinItemSlots zero_skin() { return {}; }

SkinSelectItemInfo make_skin(std::uint32_t limit_level,
                             std::initializer_list<std::uint16_t> items) {
    SkinSelectItemInfo info{};
    info.dw_limit_level = limit_level;
    std::size_t i = 0;
    for (std::uint16_t v : items) {
        if (i >= kSkinItemListMax) break;
        info.equip_item[i++] = v;
    }
    return info;
}

}  // namespace

// ---------- validation paths ----------

TEST(SkinSelectPut, NullSkinSlotsIsRejected) {
    FakeSkinEnv env;
    auto out = put_skin_select_item(env, /*current_skin=*/nullptr,
                                    /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Fail);
}

TEST(SkinSelectPut, ZeroSkinIndexIsRejected) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/0,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Fail);
    EXPECT_EQ(out.skin_item, cur);
}

TEST(SkinSelectPut, MissingSkinInfoIsRejected) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/100,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Fail);
}

TEST(SkinSelectPut, LevelFailWhenPlayerUnderLimitForNomalClothes) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, 1)] =
        make_skin(/*limit_level=*/30, {0, 0, 0});
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/10,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::LevelFail);
    EXPECT_EQ(out.skin_item, cur);
}

TEST(SkinSelectPut, LevelNotCheckedForCostumeSkin) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_COSTUME_SKIN, 1)] =
        make_skin(/*limit_level=*/30, {501, 502, 503});
    env.item_infos[501] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Hat)};
    env.item_infos[502] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Mask)};
    env.item_infos[503] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Shoes)};
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_COSTUME_SKIN,
                                    /*player_level=*/1,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Success);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Hat)],
              501u);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Mask)],
              502u);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Shoes)],
              503u);
}

TEST(SkinSelectPut, DelayFailTakesPrecedence) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, 1)] =
        make_skin(/*limit_level=*/10, {501, 0, 0});
    env.item_infos[501] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Hat)};
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/true);
    EXPECT_EQ(out.result, SkinSelectResult::DelayFail);
    EXPECT_EQ(out.skin_item, cur);
}

// ---------- happy-path mapping ----------

TEST(SkinSelectPut, HatAndMaskMapCorrectly) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, 1)] =
        make_skin(/*limit_level=*/10, {501, 502, 0});
    env.item_infos[501] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Hat)};
    env.item_infos[502] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Mask)};
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Success);
    EXPECT_EQ(out.skin_item[0], 501u);
    EXPECT_EQ(out.skin_item[1], 502u);
    EXPECT_TRUE(out.slot_written);
}

TEST(SkinSelectPut, HeadbandPart3DTypeMapsToHat) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, 1)] =
        make_skin(/*limit_level=*/10, {501, 0, 0});
    env.item_infos[501] = SkinItemInfoView{LEGACY_PART3D_HEADBAND};
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Success);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Hat)],
              501u);
}

TEST(SkinSelectPut, CostumeDressOverridesShoes) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    cur[static_cast<std::size_t>(SkinEquipSlot::Shoes)] = 999;
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_COSTUME_SKIN, 1)] =
        make_skin(/*limit_level=*/0, {601, 0, 0});
    env.item_infos[601] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Dress)};
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_COSTUME_SKIN,
                                    /*player_level=*/1,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Success);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Dress)],
              601u);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Shoes)],
              0u);  // costume dress clears shoes
}

TEST(SkinSelectPut, NomalClothesDressDoesNotOverrideShoes) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    cur[static_cast<std::size_t>(SkinEquipSlot::Shoes)] = 999;
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, 1)] =
        make_skin(/*limit_level=*/10, {601, 0, 0});
    env.item_infos[601] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Dress)};
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Success);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Dress)],
              601u);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Shoes)],
              999u);  // preserved
}

TEST(SkinSelectPut, ZeroEquipItemIsSkipped) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, 1)] =
        make_skin(/*limit_level=*/10, {0, 502, 0});
    env.item_infos[502] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Dress)};
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Success);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Dress)],
              502u);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Hat)],
              0u);  // no write to slot 0
}

TEST(SkinSelectPut, UnknownPart3DTypeDefaultsToHatSlot) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, 1)] =
        make_skin(/*limit_level=*/10, {501, 0, 0});
    env.item_infos[501] = SkinItemInfoView{99};  // unmapped
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Success);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Hat)],
              501u);
}

TEST(SkinSelectPut, MissingItemInfoIsSkipped) {
    FakeSkinEnv env;
    SkinItemSlots cur = zero_skin();
    env.skins[FakeSkinEnv::key(LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, 1)] =
        make_skin(/*limit_level=*/10, {501, 502, 0});
    env.item_infos[502] = SkinItemInfoView{
        static_cast<std::uint16_t>(SkinEquipSlot::Dress)};
    // 501 has no item info -> skipped
    auto out = put_skin_select_item(env, &cur, /*skin_index=*/1,
                                    LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN,
                                    /*player_level=*/50,
                                    /*skin_delay_active=*/false);
    EXPECT_EQ(out.result, SkinSelectResult::Success);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Hat)],
              0u);
    EXPECT_EQ(out.skin_item[static_cast<std::size_t>(SkinEquipSlot::Dress)],
              502u);
}
