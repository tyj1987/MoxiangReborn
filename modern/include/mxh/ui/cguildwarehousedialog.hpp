#pragma once
#include "mxh/ui/cDialog.hpp"
#include "mxh/services/IInventoryService.hpp"
#include <cstdint>
#include <optional>
#include <vector>
namespace mxh::ui {struct WarehouseItem{std::uint16_t item_id{};std::uint16_t quantity{};};class cGuildWarehouseDialog final:public cDialog{public:static constexpr std::size_t kSlots=60;void SetPermission(bool can_store,bool can_take)noexcept{m_can_store=can_store;m_can_take=can_take;}void SetInventoryService(mxh::services::IInventoryService* service)noexcept{m_inventory_service=service;}bool Store(std::size_t slot,WarehouseItem item);std::optional<WarehouseItem> Take(std::size_t slot);bool Move(std::size_t from,std::size_t to);void SetLocked(bool locked)noexcept{m_locked=locked;}bool IsLocked()const noexcept{return m_locked;}const std::vector<std::optional<WarehouseItem>>& Items()const noexcept{return m_items;}private:std::vector<std::optional<WarehouseItem>>m_items=std::vector<std::optional<WarehouseItem>>(kSlots);bool m_can_store{},m_can_take{},m_locked{};mxh::services::IInventoryService* m_inventory_service{};};}
