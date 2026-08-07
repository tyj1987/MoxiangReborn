// calc_shop_item_option_runtime_test.cpp
//
// Verifies apply_calc_shop_item_option() (the runtime orchestrator
// for the legacy CShopItemManager::CalcShopItemOption) applies the
// data-plane side effects to:
//   - ShopItemManager::protect_item_idx (1:1 with legacy m_ProtectItemIdx)
//   - PlayerHook callbacks (1:1 with legacy m_pPlayer->SetExtra*SlotCount)
//
// Locks the ProtectItemIdx update path, the four expanded-slot
// hooks (InvenExtend / PyogukExtend / MugongExtend / CharacterSlot
// and their HK_LOCAL variants), and the early-return-on-error
// behavior when the data plane refuses.

#include <mxh/game/shop_item_option.hpp>
#include <mxh/server/calc_shop_item_option.hpp>
#include <mxh/server/calc_shop_item_option_runtime.hpp>
#include <mxh/server/legacy_shop_item_kind.hpp>
#include <mxh/server/shop_item_manager.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

using mxh::game::ShopItemOption;
using mxh::server::CalcShopItemOptionEnv;
using mxh::server::CalcShopItemOptionInfo;
using mxh::server::CalcShopItemOptionPlayerHook;
using mxh::server::CalcShopItemOptionSideEffects;
using mxh::server::CalcShopItemOptionStatus;
using mxh::server::IncantationId;
using mxh::server::LEGACY_SHOP_ITEM_CHARM;
using mxh::server::LEGACY_SHOP_ITEM_DECORATION;
using mxh::server::LEGACY_SHOP_ITEM_HERB;
using mxh::server::LEGACY_SHOP_ITEM_INCANTATION;
using mxh::server::LEGACY_SHOP_ITEM_MAKEUP;
using mxh::server::LEGACY_SHOP_ITEM_SUNDRIES;
using mxh::server::ShopItemManager;
using mxh::server::apply_calc_shop_item_option;

class TestEnv final : public CalcShopItemOptionEnv {
public:
    bool rate_active = true;
    bool event_rate_active(std::uint16_t rate_id) const noexcept override {
        (void)rate_id;
        return rate_active;
    }
};

// Recording hook that captures each callback invocation.
class RecordingHook final : public CalcShopItemOptionPlayerHook {
public:
    std::vector<std::string> calls;

    void on_expand_inven_slot() noexcept override { calls.push_back("inven"); }
    void on_expand_pyoguk_slot() noexcept override { calls.push_back("pyoguk"); }
    void on_expand_mugong_slot() noexcept override { calls.push_back("mugong"); }
    void on_expand_character_slot() noexcept override { calls.push_back("character"); }
};

CalcShopItemOptionInfo charm_info(std::uint16_t gengol = 10) {
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_CHARM;
    info.ItemIdx  = 99999;
    info.ItemType = 10;
    info.GenGol   = gengol;
    return info;
}

CalcShopItemOptionInfo cheruyk_inc_info(std::uint32_t item_idx,
                                        std::int8_t che_ryuk) {
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = item_idx;
    info.ItemType = 10;
    info.CheRyuk  = static_cast<std::uint16_t>(che_ryuk);
    return info;
}

CalcShopItemOptionInfo inven_extend_info() {
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = static_cast<std::uint32_t>(IncantationId::InvenExtend);
    return info;
}

CalcShopItemOptionInfo pyoguk_extend_info() {
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = static_cast<std::uint32_t>(IncantationId::PyogukExtend);
    return info;
}

CalcShopItemOptionInfo mugong_extend_info() {
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = static_cast<std::uint32_t>(IncantationId::MugongExtend);
    return info;
}

CalcShopItemOptionInfo character_slot_info() {
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = static_cast<std::uint32_t>(IncantationId::CharacterSlot);
    return info;
}

}  // namespace

// ----- Charm: stat mutation only, no side effects -----

TEST(ApplyCalcShopItemOption, CharmItemMutatesStatsAndAppliesNoHooks) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    auto info = charm_info(/*gengol=*/10);

    auto out = apply_calc_shop_item_option(
        mgr, stats, info.ItemIdx, /*b_add=*/true, /*param=*/0,
        info, env, hook);

    EXPECT_EQ(stats.Gengol, 10);
    EXPECT_FALSE(out.protect_item_idx_updated);
    EXPECT_FALSE(out.inven_slot_expanded);
    EXPECT_FALSE(out.pyoguk_slot_expanded);
    EXPECT_FALSE(out.mugong_slot_expanded);
    EXPECT_FALSE(out.character_slot_expanded);
    EXPECT_TRUE(hook.calls.empty());
    EXPECT_EQ(mgr.protect_item_idx(), 0u);
}

TEST(ApplyCalcShopItemOption, CharmItemBAddFalseSubtractsStats) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    stats.Gengol = 20;
    TestEnv env;
    RecordingHook hook;
    auto info = charm_info(/*gengol=*/10);

    auto out = apply_calc_shop_item_option(
        mgr, stats, info.ItemIdx, /*b_add=*/false, 0, info, env, hook);

    EXPECT_EQ(stats.Gengol, 10);
    EXPECT_FALSE(out.protect_item_idx_updated);
    EXPECT_TRUE(hook.calls.empty());
}

TEST(ApplyCalcShopItemOption, CharmItemClampsToZeroOnUnderflow) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    stats.Gengol = 5;
    TestEnv env;
    RecordingHook hook;
    auto info = charm_info(/*gengol=*/10);

    apply_calc_shop_item_option(
        mgr, stats, info.ItemIdx, /*b_add=*/false, 0, info, env, hook);

    EXPECT_EQ(stats.Gengol, 0);  // legacy: clamp-to-zero
}

// ----- ProtectItemIdx dispatch (CheRyuk incantation) -----

TEST(ApplyCalcShopItemOption, CheruykIncantationSetsProtectItemIdx) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    constexpr std::uint32_t kIdx = 55101u;
    auto info = cheruyk_inc_info(kIdx, /*che_ryuk=*/3);

    auto out = apply_calc_shop_item_option(
        mgr, stats, kIdx, /*b_add=*/true, /*param=*/0,
        info, env, hook);

    EXPECT_TRUE(out.protect_item_idx_updated);
    EXPECT_EQ(out.protect_item_idx_after, kIdx);
    EXPECT_EQ(mgr.protect_item_idx(), kIdx);
    EXPECT_EQ(stats.ProtectCount, 3);
    EXPECT_TRUE(hook.calls.empty());
}

TEST(ApplyCalcShopItemOption, CheruykIncantationClearsProtectItemIdx) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    mgr.set_protect_item_idx(55101);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    constexpr std::uint32_t kIdx = 55101u;
    auto info = cheruyk_inc_info(kIdx, /*che_ryuk=*/3);

    auto out = apply_calc_shop_item_option(
        mgr, stats, kIdx, /*b_add=*/false, 0, info, env, hook);

    EXPECT_TRUE(out.protect_item_idx_updated);
    EXPECT_EQ(out.protect_item_idx_after, 0u);
    EXPECT_EQ(mgr.protect_item_idx(), 0u);
    EXPECT_TRUE(hook.calls.empty());
}

TEST(ApplyCalcShopItemOption, ProtectItemIdxUnchangedWhenAlreadyAtValue) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    mgr.set_protect_item_idx(55101);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    constexpr std::uint32_t kIdx = 55101u;
    auto info = cheruyk_inc_info(kIdx, /*che_ryuk=*/3);

    auto out = apply_calc_shop_item_option(
        mgr, stats, kIdx, /*b_add=*/true, 0, info, env, hook);

    EXPECT_FALSE(out.protect_item_idx_updated);  // already at value
    EXPECT_EQ(out.protect_item_idx_after, kIdx);
    EXPECT_EQ(mgr.protect_item_idx(), kIdx);
}

// ----- Locale-bounded incantation -> player hook dispatch -----

TEST(ApplyCalcShopItemOption, InvenExtendTriggersInvenSlotHook) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    auto info = inven_extend_info();

    auto out = apply_calc_shop_item_option(
        mgr, stats, info.ItemIdx, /*b_add=*/true, 0, info, env, hook);

    EXPECT_TRUE(out.inven_slot_expanded);
    EXPECT_FALSE(out.pyoguk_slot_expanded);
    EXPECT_FALSE(out.mugong_slot_expanded);
    EXPECT_FALSE(out.character_slot_expanded);
    EXPECT_EQ(hook.calls, std::vector<std::string>{"inven"});
}

TEST(ApplyCalcShopItemOption, PyogukExtendTriggersPyogukSlotHook) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    auto info = pyoguk_extend_info();

    auto out = apply_calc_shop_item_option(
        mgr, stats, info.ItemIdx, /*b_add=*/true, 0, info, env, hook);

    EXPECT_TRUE(out.pyoguk_slot_expanded);
    EXPECT_EQ(hook.calls, std::vector<std::string>{"pyoguk"});
}

TEST(ApplyCalcShopItemOption, MugongExtendTriggersMugongSlotHook) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    auto info = mugong_extend_info();

    auto out = apply_calc_shop_item_option(
        mgr, stats, info.ItemIdx, /*b_add=*/true, 0, info, env, hook);

    EXPECT_TRUE(out.mugong_slot_expanded);
    EXPECT_EQ(hook.calls, std::vector<std::string>{"mugong"});
}

TEST(ApplyCalcShopItemOption, CharacterSlotTriggersCharacterSlotHook) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    auto info = character_slot_info();

    auto out = apply_calc_shop_item_option(
        mgr, stats, info.ItemIdx, /*b_add=*/true, 0, info, env, hook);

    EXPECT_TRUE(out.character_slot_expanded);
    EXPECT_EQ(hook.calls, std::vector<std::string>{"character"});
}

TEST(ApplyCalcShopItemOption, InvenExtendHKVariantAlsoTriggersHook) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_INCANTATION;
    info.ItemIdx  = static_cast<std::uint32_t>(IncantationId::InvenExtend2);

    auto out = apply_calc_shop_item_option(
        mgr, stats, info.ItemIdx, /*b_add=*/true, 0, info, env, hook);

    EXPECT_TRUE(out.inven_slot_expanded);
    EXPECT_EQ(hook.calls, std::vector<std::string>{"inven"});
}

// ----- Early returns -----

TEST(ApplyCalcShopItemOption, InvalidIconAppliesNoHooks) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    auto info = charm_info();

    auto out = apply_calc_shop_item_option(
        mgr, stats, /*w_idx=*/0, /*b_add=*/true, 0, info, env, hook);

    EXPECT_EQ(stats.Gengol, 0);
    EXPECT_FALSE(out.protect_item_idx_updated);
    EXPECT_TRUE(hook.calls.empty());
    EXPECT_EQ(mgr.protect_item_idx(), 0u);
}

TEST(ApplyCalcShopItemOption, ItemInfoMissingAppliesNoHooks) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    // Empty info -> data plane returns ItemInfoMissing
    CalcShopItemOptionInfo info;

    auto out = apply_calc_shop_item_option(
        mgr, stats, /*w_idx=*/1, /*b_add=*/true, 0, info, env, hook);

    EXPECT_FALSE(out.protect_item_idx_updated);
    EXPECT_TRUE(hook.calls.empty());
    EXPECT_EQ(mgr.protect_item_idx(), 0u);
}

// ----- Other ItemKind branches do not fire player hooks -----

TEST(ApplyCalcShopItemOption, HerbItemMutatesStatsNoHooks) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    ShopItemOption stats;
    TestEnv env;
    RecordingHook hook;
    CalcShopItemOptionInfo info;
    info.ItemKind = LEGACY_SHOP_ITEM_HERB;
    info.ItemIdx  = 7777;
    info.Life     = 50;

    auto out = apply_calc_shop_item_option(
        mgr, stats, info.ItemIdx, /*b_add=*/true, 0, info, env, hook);

    EXPECT_EQ(stats.Life, 50);
    EXPECT_TRUE(hook.calls.empty());
    EXPECT_FALSE(out.protect_item_idx_updated);
}
