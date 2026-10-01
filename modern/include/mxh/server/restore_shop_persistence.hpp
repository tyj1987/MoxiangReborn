#pragma once
#include <mxh/db/modern_shop_state.hpp>
#include <mxh/server/restore_used_shop_item.hpp>
#include <algorithm>

namespace mxh::server {
// Operates on an unpublished admission candidate. The caller must keep inventory
// changes in that same candidate and persist them in the same DB transaction.
// save_shop_record does not commit a transaction or publish a player.
class ShopRestorePersistenceEffects : public ShopRestoreCalculatedEffects {
public:
    ShopRestorePersistenceEffects(ShopItemManager& manager, const game::ItemManager& catalog,
        game::ShopItemOption& options, const CalcShopItemOptionEnv& environment,
        CalcShopItemOptionPlayerHook& player, const DupParamLookup& duplicates,
        db::ModernShopState snapshot)
        : ShopRestoreCalculatedEffects(manager,catalog,options,environment,player,duplicates),
          m_snapshot(std::move(snapshot)), m_pending(m_snapshot.rows) {}

    void update_use_info(const game::ItemBase& item, std::uint32_t param,
                         std::uint32_t remaining) final {
        auto it=find_unique(item.dwDBIdx);
        it->item_id=item.wIconIdx;
        it->parameter=param;
        it->remaining_time=remaining;
    }
    void delete_use_info(const game::ShopItemBase& item) final {
        auto it=find_unique(item.ItemBase.dwDBIdx);
        if(it->item_id!=item.ItemBase.wIconIdx)
            throw std::runtime_error("shop expiry item identity mismatch");
        m_pending.used_items.erase(it);
    }
    const db::LegacyShopAppearanceRows& pending_rows() const noexcept { return m_pending; }
    void set_skin(const std::array<std::uint16_t, game::ESkinItemCount>& skin) noexcept {
        m_pending.skin = skin;
    }
    void update_avatar_parameter(std::uint16_t icon,std::uint32_t parameter) {
        auto found=m_pending.used_items.end();
        for(auto it=m_pending.used_items.begin();it!=m_pending.used_items.end();++it) {
            if(it->item_id!=icon) continue;
            if(found!=m_pending.used_items.end()) throw std::runtime_error("ambiguous avatar use icon");
            found=it;
        }
        if(found==m_pending.used_items.end()) throw std::runtime_error("missing avatar use icon");
        found->parameter=parameter;
    }
    bool save_shop_record(db::IDbAdapter& database,std::uint32_t player,std::uint32_t account) {
        // Keep the original snapshot as the concurrency token. A repeated call
        // after a changing save requires a fresh admission snapshot.
        return db::save_modern_shop_state(database,player,account,m_snapshot,m_pending);
    }
private:
    std::vector<db::LegacyUsedShopItem>::iterator find_unique(std::uint32_t id) {
        auto found=m_pending.used_items.end();
        for(auto it=m_pending.used_items.begin();it!=m_pending.used_items.end();++it) {
            if(it->database_id!=id) continue;
            if(found!=m_pending.used_items.end()) throw std::runtime_error("ambiguous shop database identity");
            found=it;
        }
        if(found==m_pending.used_items.end()) throw std::runtime_error("missing shop database identity");
        return found;
    }
    db::ModernShopState m_snapshot;
    db::LegacyShopAppearanceRows m_pending;
};
} // namespace mxh::server
