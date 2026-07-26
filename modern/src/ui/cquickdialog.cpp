#include "cquickdialog.hpp"
namespace mxh::ui {
cQuickDialog::cQuickDialog():m_slots(kPageCount*kSlotsPerPage){}
bool cQuickDialog::Bind(std::size_t p,std::size_t s,QuickKind k,std::uint32_t id){if(p>=kPageCount||s>=kSlotsPerPage||k==QuickKind::Empty||id==0)return false;m_slots[p*kSlotsPerPage+s]={k,id};return true;}
bool cQuickDialog::Remove(std::size_t p,std::size_t s){if(p>=kPageCount||s>=kSlotsPerPage)return false;auto&i=m_slots[p*kSlotsPerPage+s];if(i.kind==QuickKind::Empty)return false;i={};return true;}
std::optional<QuickSlot> cQuickDialog::Get(std::size_t p,std::size_t s)const{if(p>=kPageCount||s>=kSlotsPerPage)return std::nullopt;auto v=m_slots[p*kSlotsPerPage+s];return v.kind==QuickKind::Empty?std::nullopt:std::optional<QuickSlot>(v);}
bool cQuickDialog::Activate(std::size_t s){auto v=Get(m_page,s);if(!v)return false;if(m_cb)m_cb(*v,m_user);return true;}
}
