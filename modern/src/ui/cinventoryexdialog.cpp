#include "cinventoryexdialog.hpp"
#include <utility>
namespace mxh::ui {
cInventoryExDialog::cInventoryExDialog():m_slots(kSlotCount){}
bool cInventoryExDialog::AddItem(std::uint16_t id,std::int32_t durability){for(auto& slot:m_slots)if(!slot){slot=InventoryItem{id,durability,false};return true;}return false;}
bool cInventoryExDialog::DeleteItem(std::size_t p){if(p>=m_slots.size()||!m_slots[p])return false;m_slots[p].reset();return true;}
bool cInventoryExDialog::MoveItem(std::size_t f,std::size_t t){if(f>=m_slots.size()||t>=m_slots.size()||f==t||!m_slots[f]||m_slots[f]->locked|| (m_slots[t]&&m_slots[t]->locked))return false;std::swap(m_slots[f],m_slots[t]);return true;}
bool cInventoryExDialog::IsExist(std::size_t p)const noexcept{return p<m_slots.size()&&m_slots[p].has_value();}
std::optional<InventoryItem> cInventoryExDialog::GetItemForPos(std::size_t p)const{return p<m_slots.size()?m_slots[p]:std::nullopt;}
std::size_t cInventoryExDialog::GetBlankNum()const noexcept{std::size_t n=0;for(const auto& s:m_slots)n+=!s.has_value();return n;}
bool cInventoryExDialog::SetItemLocked(std::size_t p,bool locked)noexcept{if(p>=m_slots.size()||!m_slots[p])return false;m_slots[p]->locked=locked;return true;}
bool cInventoryExDialog::UpdateItemDurabilityAdd(std::size_t p,int d){if(p>=m_slots.size()||!m_slots[p])return false;m_slots[p]->durability+=d;return true;}
void cInventoryExDialog::ReleaseInventory()noexcept{for(auto& s:m_slots)s.reset();m_money=0;m_state=InventoryState::Default;}
}


