#pragma once
#include <mxh/server/avatar_equip_transition.hpp>
#include <mxh/server/avatar_equip_catalog.hpp>
#include <mxh/server/shop_item_manager.hpp>
#include <mxh/game/item_manager.hpp>
#include <span>
#include <stdexcept>
#include <unordered_map>

namespace mxh::server {
// Immutable lookup snapshot for one source-ordered transition. Recreate after
// applying its effects: later use rows must not appear available prematurely.
class AvatarEquipEnvironment final : public AvatarEquipEnv {
public:
    AvatarEquipEnvironment(const AvatarEquipCatalog& avatars,
        const game::ItemManager& items, const ShopItemManager& used,
        std::span<const game::ItemBase> inventory,
        std::span<const game::ItemBase> equipment,
        std::span<const game::ItemBase> shop) : m_avatars(avatars) {
        for(const auto& item:items.items())
            m_info.emplace(item.ItemIdx,AvatarItemInfoView{item.SellPrice});
        for(const auto& [id,entry]:used.using_items()) {
            if(id>65535) throw std::runtime_error("avatar use icon outside WORD range");
            m_used.emplace(static_cast<std::uint16_t>(id),AvatarUsingItemView{entry.Data.ShopItem.ItemBase.dwDBIdx});
        }
        const auto append=[&](std::span<const game::ItemBase> slots) {
            for(const auto& item:slots) {
                if(item.dwDBIdx==0 || item.wIconIdx==0) continue;
                if(!m_slots.emplace(item.Position,AvatarItemBaseView{item.dwDBIdx}).second)
                    throw std::runtime_error("ambiguous avatar inventory position");
            }
        };
        append(inventory); append(equipment); append(shop);
    }
    const AvatarUsingItemView* find_using_item(std::uint16_t id) const noexcept override {
        const auto it=m_used.find(id); return it==m_used.end()?nullptr:&it->second;
    }
    const AvatarItemBaseView* find_item_at(std::uint16_t pos) const noexcept override {
        const auto it=m_slots.find(pos); return it==m_slots.end()?nullptr:&it->second;
    }
    const AvatarEquipRow* find_avatar_equip(std::uint16_t id) const noexcept override {
        const auto it=m_avatars.entries.find(id); return it==m_avatars.entries.end()?nullptr:&it->second.equip;
    }
    const AvatarItemInfoView* find_item_info(std::uint16_t id) const noexcept override {
        const auto it=m_info.find(id); return it==m_info.end()?nullptr:&it->second;
    }
private:
    const AvatarEquipCatalog& m_avatars;
    std::unordered_map<std::uint32_t,AvatarItemInfoView> m_info;
    std::unordered_map<std::uint16_t,AvatarUsingItemView> m_used;
    std::unordered_map<std::uint16_t,AvatarItemBaseView> m_slots;
};
} // namespace mxh::server
