#pragma once
#include "mxh/ui/cDialog.hpp"
#include "mxh/services/IInventoryService.hpp"
#include <cstdint>
#include <optional>
#include <vector>
namespace mxh::ui {
struct InventoryItem { std::uint16_t item_id{}; std::int32_t durability{}; bool locked{}; };
enum class InventoryState : std::uint8_t { Default, Upgrade, Deal };
class cInventoryExDialog final : public cDialog {
public:
 static constexpr std::size_t kSlotCount = 60;
 cInventoryExDialog();
 void SetMoney(std::uint32_t money) noexcept { m_money=money; }
 std::uint32_t GetMoney() const noexcept { return m_money; }
 bool AddItem(std::uint16_t item_id, std::int32_t durability=0);
 bool DeleteItem(std::size_t position);
 bool MoveItem(std::size_t from, std::size_t to);
 bool IsExist(std::size_t position) const noexcept;
 std::optional<InventoryItem> GetItemForPos(std::size_t position) const;
 std::size_t GetBlankNum() const noexcept;
 bool SetItemLocked(std::size_t position, bool locked) noexcept;
 bool UpdateItemDurabilityAdd(std::size_t position, int delta);
 void SetState(InventoryState state) noexcept { m_state=state; }
 InventoryState GetState() const noexcept { return m_state; }
 void ReleaseInventory() noexcept;
 void SetInventoryService(const mxh::services::IInventoryService* service) noexcept { m_inventory=service; }
 const mxh::services::IInventoryService* inventoryService() const noexcept { return m_inventory; }
 // Refresh the visible 60-slot panel from the live inventory snapshot.
 void RefreshFromInventoryService();
private:
 std::vector<std::optional<InventoryItem>> m_slots;
 std::uint32_t m_money{};
 InventoryState m_state{InventoryState::Default};
 const mxh::services::IInventoryService* m_inventory{};
};
}


