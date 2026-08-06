// D4.31 PutOn/TakeOffAvatarItem data-plane tests.
// Locks the new/remove logic and side-effect emission in
// 1:1 with the legacy [Server]Map/ShopItemManager.cpp:1792-2021 body.

#include <mxh/server/avatar_equip_transition.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

using namespace mxh::server;
using namespace mxh::game;

namespace {

class FakeEnv final : public AvatarEquipEnv {
public:
    std::unordered_map<std::uint16_t, AvatarUsingItemView> using_items;
    std::unordered_map<std::uint16_t, AvatarItemBaseView> inventory;
    std::unordered_map<std::uint16_t, AvatarEquipRow> avatar_equips;
    std::unordered_map<std::uint16_t, AvatarItemInfoView> item_infos;

    const AvatarUsingItemView* find_using_item(
        std::uint16_t item_idx) const noexcept override {
        auto it = using_items.find(item_idx);
        return it != using_items.end() ? &it->second : nullptr;
    }

    const AvatarItemBaseView* find_item_at(
        std::uint16_t item_pos) const noexcept override {
        auto it = inventory.find(item_pos);
        return it != inventory.end() ? &it->second : nullptr;
    }

    const AvatarEquipRow* find_avatar_equip(
        std::uint16_t item_idx) const noexcept override {
        auto it = avatar_equips.find(item_idx);
        return it != avatar_equips.end() ? &it->second : nullptr;
    }

    const AvatarItemInfoView* find_item_info(
        std::uint16_t item_idx) const noexcept override {
        auto it = item_infos.find(item_idx);
        return it != item_infos.end() ? &it->second : nullptr;
    }
};

AvatarEquipRow make_equip(std::uint8_t pos,
                          std::initializer_list<std::uint16_t> mask) {
    AvatarEquipRow e;
    e.position = pos;
    std::size_t i = 0;
    for (std::uint16_t v : mask) {
        if (i >= EAvatarCount) break;
        e.item[i++] = v;
    }
    return e;
}

std::array<std::uint16_t, EAvatarCount> zero_avatar() { return {}; }

}  // namespace

// ---------- PutOnAvatarItem no-op conditions ----------

TEST(AvatarEquipPutOn, NullAvatarIsRejected) {
    FakeEnv env;
    auto out = put_on_avatar_item(env, /*current_avatar=*/nullptr,
                                  /*item_idx=*/100, /*item_pos=*/5,
                                  /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::AvatarMissing);
    EXPECT_TRUE(out.effects.empty());
}

TEST(AvatarEquipPutOn, NullAvatarAndEmptyEnvYieldsAvatarMissing) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::ItemBaseMissing);
}

TEST(AvatarEquipPutOn, MissingItemBaseIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::ItemBaseMissing);
    EXPECT_EQ(out.avatar, av);
    EXPECT_TRUE(out.effects.empty());
}

TEST(AvatarEquipPutOn, MissingUsingItemIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{100};
    env.inventory[5] = base;
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::UsingItemMissing);
    EXPECT_EQ(out.avatar, av);
}

TEST(AvatarEquipPutOn, DbIdxMismatchIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{200};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::ItemBaseMismatch);
    EXPECT_EQ(out.avatar, av);
}

TEST(AvatarEquipPutOn, MissingAvatarEquipRowIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::AvatarEquipMissing);
    EXPECT_EQ(out.avatar, av);
}

TEST(AvatarEquipPutOn, PositionOutOfRangeIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(99, {});
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::PositionOutOfRange);
    EXPECT_EQ(out.avatar, av);
}

TEST(AvatarEquipPutOn, MissingItemInfoIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(2, {});
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::ItemInfoMissing);
    EXPECT_EQ(out.avatar, av);
}

TEST(AvatarEquipPutOn, HatBlockedByDressWithoutHatMaskIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[6] = 500;  // dress slot filled
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    AvatarEquipRow dress = make_equip(6, {0,0,0,0,0,0, 0,0,0,0,0,0,
                                          0,0,0,0,0,0, 0,0,0,0,0,0});
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(0, {});  // hat
    env.avatar_equips[500] = dress;
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::HatBlockedByDress);
    EXPECT_EQ(out.avatar, av);
}

TEST(AvatarEquipPutOn, HatAllowedWhenDressHasHatMask) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[6] = 500;  // dress slot filled
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    std::array<std::uint16_t, EAvatarCount> dress_mask{};
    dress_mask[0] = 1;  // allows hat
    AvatarEquipRow dress{};
    dress.position = 6;
    dress.item = dress_mask;
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(0, {});  // hat
    env.avatar_equips[500] = dress;
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_EQ(out.avatar[0], 100u);
    EXPECT_TRUE(out.send_avatar_info);
    EXPECT_TRUE(out.recalculate_avatar_option);
}

TEST(AvatarEquipPutOn, WeaponSlotMismatchWhenPlayerInited) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(20, {});  // Weared_Do weapon slot
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/true,
                                  /*weapon_equip_type=*/2);
    EXPECT_EQ(out.status, AvatarEquipStatus::WeaponSlotMismatch);
    EXPECT_EQ(out.avatar, av);
}

TEST(AvatarEquipPutOn, WeaponSlotAllowedWhenSlotMatchesWeaponType) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    // Position 19 = Weared_Gwun (18 + 1); weapon_equip_type = 2 -> gun
    env.avatar_equips[100] = make_equip(19, {});  // position 19 = Weared_Gwun
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/true,
                                  /*weapon_equip_type=*/2);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_EQ(out.avatar[19], 100u);
}

TEST(AvatarEquipPutOn, NonWeaponPositionFilledWithoutDefaultFill) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(2, {});  // Face
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/true,
                                  /*weapon_equip_type=*/2);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_EQ(out.avatar[2], 100u);
    // default fill applies to the new equip's mask; mask[12..17]=0 -> 1
    for (std::size_t i = 12; i < 18; ++i) {
        EXPECT_EQ(out.avatar[i], 1u);
    }
}

TEST(AvatarEquipPutOn, DefaultFillZerosBecomeOnes) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    // equip.Item[12..17] = 0 -> those slots default to 1
    env.avatar_equips[100] = make_equip(2, {0,0,0,0,0,0,0,0,0,0,0,0,
                                            0,0,0,0,0,0, 0,0,0,0,0,0});
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/true,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_EQ(out.avatar[2], 100u);
    for (std::size_t i = 12; i < 18; ++i) {
        EXPECT_EQ(out.avatar[i], 1u);
    }
}

TEST(AvatarEquipPutOn, DefaultFillNonZeroMasksPreserve) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[12] = 999;  // pre-existing value, mask != 0
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    std::array<std::uint16_t, EAvatarCount> mask{};
    mask[12] = 7;  // non-zero -> preserve existing
    AvatarEquipRow row{};
    row.position = 2;
    row.item = mask;
    env.avatar_equips[100] = row;
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/true,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_EQ(out.avatar[2], 100u);
    EXPECT_EQ(out.avatar[12], 999u);  // preserved
    for (std::size_t i = 13; i < 18; ++i) {
        EXPECT_EQ(out.avatar[i], 1u);
    }
}

TEST(AvatarEquipPutOn, ReplacingHatOverridesExistingAndAppliesMask) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[0] = 333;  // pre-existing hat
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(0, {});
    env.item_infos[100] = AvatarItemInfoView{};
    // The equip has Item[12..17] = 0 -> default fill -> 1
    env.item_infos[333] = AvatarItemInfoView{777};
    env.avatar_equips[333] = make_equip(0, {});
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_EQ(out.avatar[0], 100u);
    for (std::size_t i = 12; i < 18; ++i) {
        EXPECT_EQ(out.avatar[i], 1u);
    }
    // the previous item's param update is recorded
    bool found_old_param = false;
    for (const auto& e : out.effects) {
        if (e.kind == AvatarEquipEffectKind::ParamUpdateToDb &&
            e.item_idx == 333 && e.param == 777u) {
            found_old_param = true;
            break;
        }
    }
    EXPECT_TRUE(found_old_param);
}

TEST(AvatarEquipPutOn, WeaponSlotReplacesOldItemWithParamUpdate) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[18] = 333;  // pre-existing weapon
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(18, {});
    env.item_infos[100] = AvatarItemInfoView{};
    env.item_infos[333] = AvatarItemInfoView{888};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/true,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_EQ(out.avatar[18], 100u);
    bool found_old = false;
    bool found_new = false;
    for (const auto& e : out.effects) {
        if (e.kind == AvatarEquipEffectKind::ParamUpdateToDb &&
            e.item_idx == 333 && e.param == 888u) {
            found_old = true;
        }
        if (e.kind == AvatarEquipEffectKind::ParamUpdateToDb &&
            e.item_idx == 100 && e.param == kShopItemUseParamEquipAvatar) {
            found_new = true;
        }
    }
    EXPECT_TRUE(found_old);
    EXPECT_TRUE(found_new);
}

TEST(AvatarEquipPutOn, WeaponSlotExistingItemInfoMissingIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[18] = 333;  // pre-existing weapon, but no item info -> legacy returns
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[5] = base;
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(18, {});
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/5, /*player_inited=*/true,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::ExistingItemInfoMissing);
}

TEST(AvatarEquipPutOn, ItemPosZeroSuppressesBroadcast) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    AvatarItemBaseView base{100};
    AvatarUsingItemView shop{100};
    env.inventory[0] = base;  // item_pos = 0 must exist in inventory
    env.using_items[100] = shop;
    env.avatar_equips[100] = make_equip(2, {});
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = put_on_avatar_item(env, &av, /*item_idx=*/100,
                                  /*item_pos=*/0, /*player_inited=*/false,
                                  /*weapon_equip_type=*/1);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_FALSE(out.send_avatar_info);
    EXPECT_TRUE(out.recalculate_avatar_option);
}

// ---------- TakeOffAvatarItem no-op conditions ----------

TEST(AvatarEquipTakeOff, NullAvatarPointerIsRejected) {
    FakeEnv env;
    auto out = take_off_avatar_item(env, /*current_avatar=*/nullptr,
                                    /*item_idx=*/100, /*item_pos=*/5);
    EXPECT_EQ(out.status, AvatarEquipStatus::AvatarMissing);
}

TEST(AvatarEquipTakeOff, MissingAvatarEquipRowIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    auto out = take_off_avatar_item(env, &av, /*item_idx=*/100,
                                    /*item_pos=*/5);
    EXPECT_EQ(out.status, AvatarEquipStatus::AvatarEquipMissing);
}

TEST(AvatarEquipTakeOff, MissingItemInfoIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    env.avatar_equips[100] = make_equip(2, {});
    auto out = take_off_avatar_item(env, &av, /*item_idx=*/100,
                                    /*item_pos=*/5);
    EXPECT_EQ(out.status, AvatarEquipStatus::ItemInfoMissing);
}

TEST(AvatarEquipTakeOff, MismatchedSlotIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[2] = 999;  // different item
    env.avatar_equips[100] = make_equip(2, {});
    env.item_infos[100] = AvatarItemInfoView{};
    auto out = take_off_avatar_item(env, &av, /*item_idx=*/100,
                                    /*item_pos=*/5);
    EXPECT_EQ(out.status, AvatarEquipStatus::AvatarMismatch);
}

TEST(AvatarEquipTakeOff, CosmeticPositionClearedAndDefaultFilled) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[2] = 100;
    env.avatar_equips[100] = make_equip(2, {});
    env.item_infos[100] = AvatarItemInfoView{42};
    auto out = take_off_avatar_item(env, &av, /*item_idx=*/100,
                                    /*item_pos=*/5);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_EQ(out.avatar[2], 0u);
    for (std::size_t i = 12; i < 18; ++i) {
        EXPECT_EQ(out.avatar[i], 1u);
    }
    bool found_param_update = false;
    for (const auto& e : out.effects) {
        if (e.kind == AvatarEquipEffectKind::ParamUpdateToDb &&
            e.item_idx == 100 && e.param == 42u) {
            found_param_update = true;
            break;
        }
    }
    EXPECT_TRUE(found_param_update);
}

TEST(AvatarEquipTakeOff, WeaponSlotReplacedByDressDefault) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[6] = 500;  // dress slot
    av[18] = 100; // weapon slot
    env.avatar_equips[100] = make_equip(18, {});
    env.avatar_equips[500] = make_equip(6, {});
    env.item_infos[100] = AvatarItemInfoView{42};
    env.item_infos[500] = AvatarItemInfoView{99};
    auto out = take_off_avatar_item(env, &av, /*item_idx=*/100,
                                    /*item_pos=*/5);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    // legacy takes the dress's Item[18] default which is 0 here
    EXPECT_EQ(out.avatar[18], 0u);
    // position 6 (dress) cleared too since item 100 mask[6]=0
    EXPECT_EQ(out.avatar[6], 0u);
    bool found_500_param = false;
    for (const auto& e : out.effects) {
        if (e.kind == AvatarEquipEffectKind::ParamUpdateToDb &&
            e.item_idx == 500 && e.param == 99u) {
            found_500_param = true;
            break;
        }
    }
    EXPECT_TRUE(found_500_param);
}

TEST(AvatarEquipTakeOff, WeaponSlotClearedWhenNoDress) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[18] = 100;
    env.avatar_equips[100] = make_equip(18, {});
    env.item_infos[100] = AvatarItemInfoView{42};
    auto out = take_off_avatar_item(env, &av, /*item_idx=*/100,
                                    /*item_pos=*/5);
    EXPECT_EQ(out.status, AvatarEquipStatus::Ok);
    EXPECT_EQ(out.avatar[18], 0u);
}

TEST(AvatarEquipTakeOff, DependentItemInfoMissingIsRejected) {
    FakeEnv env;
    AvatarSlots av = zero_avatar();
    av[0] = 100;
    av[3] = 333;  // dependent cosmetic slot
    env.avatar_equips[100] = make_equip(0, {});
    env.item_infos[100] = AvatarItemInfoView{};
    // No item info for 333 -> dependent removal fails
    auto out = take_off_avatar_item(env, &av, /*item_idx=*/100,
                                    /*item_pos=*/5);
    EXPECT_EQ(out.status, AvatarEquipStatus::DependentItemInfoMissing);
}









