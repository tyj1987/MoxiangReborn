#pragma once

#include <mxh/server/shop_item_manager.hpp>
#include <mxh/server/calc_shop_item_option.hpp>
#include <mxh/server/calc_shop_item_option_runtime.hpp>
#include <stdexcept>

namespace mxh::server {

enum class ShopRestoreLocale { China, Korea, Japan, HongKong, Thailand };
enum class ShopRestoreStatus { Restored, InvalidItem, AlreadyPresent, MissingItemInfo,
                               Expired, DiscardFailed, InsertFailed };

// Required runtime bindings; no default successful database or gameplay hooks.
// Source: [Server]Map/ShopItemManager.cpp:3493-3619. The caller owns the
// player identity, persistence transaction and error handling for these effects.
class ShopRestoreEffects {
public:
    virtual ~ShopRestoreEffects() = default;
    virtual void update_use_info(const game::ItemBase&, std::uint32_t param,
                                 std::uint32_t remaining) = 0;
    virtual bool discard_item(const game::ItemBase&) = 0;
    virtual void delete_use_info(const game::ShopItemBase&) = 0;
    virtual void log_expired(const game::ShopItemBase&) = 0;
    virtual void calculate_option(std::uint16_t item, std::uint32_t parameter) = 0;
    virtual void add_dup_parameter(const game::ItemInfo* info) = 0;
};

// Binds restoration to the real option calculator and duplicate counters.
// Persistence, inventory and logging must still be implemented by the caller.
// References must describe the same player passed to restore_used_shop_item.
class ShopRestoreCalculatedEffects : public ShopRestoreEffects {
public:
    ShopRestoreCalculatedEffects(ShopItemManager& manager, const game::ItemManager& catalog,
        game::ShopItemOption& options, const CalcShopItemOptionEnv& environment,
        CalcShopItemOptionPlayerHook& player, const DupParamLookup& duplicates)
        : m_manager(manager), m_catalog(catalog), m_options(options),
          m_environment(environment), m_player(player), m_duplicates(duplicates) {}

    void add_dup_parameter(const game::ItemInfo* info) final {
        if (!info) return;
        const double rate = info->LifeRecoverRate;
        if (!std::isfinite(rate) || rate < 0 || rate > 4294967295.0)
            throw std::runtime_error("invalid shop duplicate-table index");
        DupParamIndices indices{info->AllPlus_Value, info->MugongNum, info->MugongType,
                                info->LifeRecover, static_cast<std::uint32_t>(rate), rate != 0};
        auto counters = m_manager.dup_counters();
        SundrySideEffects side_effects{};
        add_dup_param(counters, indices, m_duplicates, side_effects);
        m_manager.set_dup_counters(counters);
        if (side_effects.set_b_street_stall) m_options.bStreetStall = 1;
    }

    void calculate_option(std::uint16_t item, std::uint32_t parameter) final {
        // Re-query the post-conversion icon, exactly as the original calculator
        // does; StatePoint_30 restoration may have changed it to StatePoint.
        game::ItemInfo info{};
        if (!m_catalog.try_get(item, info)) return; // original missing-info early return
        const auto input = shop_option_info(info);
        if (!input) throw std::runtime_error("invalid shop option fire metadata");
        apply_calc_shop_item_option(m_manager, m_options, item, true, parameter,
                                    *input, m_environment, m_player);
    }
private:
    ShopItemManager& m_manager;
    const game::ItemManager& m_catalog;
    game::ShopItemOption& m_options;
    const CalcShopItemOptionEnv& m_environment;
    CalcShopItemOptionPlayerHook& m_player;
    const DupParamLookup& m_duplicates;
};

inline bool restored_expansion_skips_options(std::uint16_t item, ShopRestoreLocale locale) {
    if (locale == ShopRestoreLocale::China || locale == ShopRestoreLocale::Korea) return false;
    switch (static_cast<IncantationId>(item)) {
    case IncantationId::MugongExtend: case IncantationId::PyogukExtend:
    case IncantationId::InvenExtend: case IncantationId::CharacterSlot: return true;
    case IncantationId::MugongExtend2: case IncantationId::PyogukExtend2:
    case IncantationId::InvenExtend2: case IncantationId::CharacterSlot2:
        return locale == ShopRestoreLocale::HongKong;
    default: return false;
    }
}

inline ShopRestoreStatus restore_used_shop_item(
    ShopItemManager& manager, const game::ItemManager& catalog,
    game::ShopItemOption& options, ShopRestoreEffects& effects,
    game::ItemBase* item, std::uint32_t param, game::PackedTime begin,
    std::uint32_t remaining, std::uint32_t now_ms, game::PackedTime now,
    ShopRestoreLocale locale) {
    if (!item || item->wIconIdx == 0) return ShopRestoreStatus::InvalidItem;
    if (manager.has_using_item(item->wIconIdx)) return ShopRestoreStatus::AlreadyPresent;
    game::ItemInfo info{};
    const bool known = catalog.try_get(item->wIconIdx, info);
    constexpr auto skill = static_cast<std::uint16_t>(IncantationId::SkPointRedist);
    constexpr auto state = static_cast<std::uint16_t>(IncantationId::StatePoint);
    if (!known) {
        if (item->wIconIdx == skill) {
            options.SkillPoint = param;
            options.UseSkillPoint = remaining;
        } else if (item->wIconIdx == state) {
            options.StatePoint = static_cast<std::uint16_t>(param);
            options.UseStatePoint = static_cast<std::uint16_t>(remaining);
        } else return ShopRestoreStatus::MissingItemInfo;
    } else if (item->wIconIdx == 55321u) { // eIncantation_StatePoint_30
        options.StatePoint = static_cast<std::uint16_t>(remaining);
        options.UseStatePoint = static_cast<std::uint16_t>(30 - options.StatePoint);
        item->wIconIdx = state;
        effects.update_use_info(*item, options.StatePoint, options.UseStatePoint);
    }
    UsingShopItemEntry entry{};
    entry.ItemIdx = item->wIconIdx;
    entry.Data.ShopItem.ItemBase = *item;
    entry.Data.ShopItem.Param = param;
    entry.Data.ShopItem.BeginTime = begin;
    entry.Data.ShopItem.Remaintime = remaining;
    entry.Data.LastCheckTime = now_ms;
    if (known && info.SellPrice == game::SHOP_ITEM_PARAM_STORED_TIME && now.value > remaining) {
        if (info.ItemType == 11 && !effects.discard_item(*item)) return ShopRestoreStatus::DiscardFailed;
        effects.delete_use_info(entry.Data.ShopItem);
        effects.log_expired(entry.Data.ShopItem);
        return ShopRestoreStatus::Expired;
    }
    if (!manager.add_using_item(entry)) return ShopRestoreStatus::InsertFailed;
    if (restored_expansion_skips_options(item->wIconIdx, locale)) return ShopRestoreStatus::Restored;
    if (known && info.ItemKind == LEGACY_SHOP_ITEM_INCANTATION && info.CheRyuk) {
        effects.calculate_option(item->wIconIdx, param);
    } else {
        if (known && info.ItemKind == LEGACY_SHOP_ITEM_CHARM && info.MeleeAttackMin && remaining == 0)
            return ShopRestoreStatus::Restored;
        effects.calculate_option(item->wIconIdx, remaining);
    }
    effects.add_dup_parameter(known ? &info : nullptr);
    return ShopRestoreStatus::Restored;
}

} // namespace mxh::server
