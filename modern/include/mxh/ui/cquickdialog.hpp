#pragma once
#include "mxh/ui/cDialog.hpp"
#include "mxh/services/IInventoryService.hpp"
#include "mxh/services/ISkillService.hpp"
#include <cstdint>
#include <optional>
#include <vector>
namespace mxh::ui {
enum class QuickKind : std::uint8_t { Empty, Item, Skill, Ability };
struct QuickSlot { QuickKind kind{QuickKind::Empty}; std::uint32_t id{}; };
class cQuickDialog final : public cDialog {
public:
 static constexpr std::size_t kPageCount=3,kSlotsPerPage=12;
 cQuickDialog();
 bool Bind(std::size_t page,std::size_t slot,QuickKind kind,std::uint32_t id);
 void SetInventoryService(const mxh::services::IInventoryService* service) noexcept { m_inventory=service; }
 void SetSkillService(const mxh::services::ISkillService* service) noexcept { m_skills=service; }
 bool BindItemFromInventory(std::size_t page,std::size_t slot,std::uint16_t iconIdx);
 bool BindSkillFromService(std::size_t page,std::size_t slot,std::uint32_t skillIdx);
 bool Remove(std::size_t page,std::size_t slot);
 std::optional<QuickSlot> Get(std::size_t page,std::size_t slot) const;
 void SelectPage(std::size_t page) noexcept {if(page<kPageCount)m_page=page;}
 std::size_t SelectedPage()const noexcept{return m_page;}
 void SetActivateCallback(void(*cb)(QuickSlot,void*),void* user=nullptr)noexcept{m_cb=cb;m_user=user;}
 bool Activate(std::size_t slot);
private: std::vector<QuickSlot> m_slots; std::size_t m_page{}; void(*m_cb)(QuickSlot,void*){}; void* m_user{};
 const mxh::services::IInventoryService* m_inventory{};
 const mxh::services::ISkillService* m_skills{};
};
}
