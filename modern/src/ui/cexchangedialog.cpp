#include "cexchangedialog.hpp"
#include "cdealdialog.hpp"  // for mxh::ui::DealItem (used to drive ITradeService::completeTrade)
namespace mxh::ui {
static bool valid(ExchangeSlot x){return x.item_id!=0&&x.quantity!=0;}
bool cExchangeDialog::SetOwn(std::size_t s,ExchangeSlot x){if(s>=kSlots||m_completed||m_cancelled||m_own_confirmed||!valid(x))return false;if(m_inventory_service&&!m_inventory_service->hasItem(x.item_id))return false;m_own[s]=x;m_other_confirmed=false;return true;}
bool cExchangeDialog::SetOther(std::size_t s,ExchangeSlot x){if(s>=kSlots||m_completed||m_cancelled||m_other_confirmed||!valid(x))return false;m_other[s]=x;m_own_confirmed=false;return true;}
bool cExchangeDialog::ClearOwn(std::size_t s){if(s>=kSlots||m_completed||m_cancelled||m_own_confirmed||!valid(m_own[s]))return false;m_own[s]={};return true;}
bool cExchangeDialog::ClearOther(std::size_t s){if(s>=kSlots||m_completed||m_cancelled||m_other_confirmed||!valid(m_other[s]))return false;m_other[s]={};return true;}
bool cExchangeDialog::SetOwnConfirmed(bool v)noexcept{if(m_completed||m_cancelled)return false;m_own_confirmed=v;if(!v)m_other_confirmed=false;return true;}
bool cExchangeDialog::SetOtherConfirmed(bool v)noexcept{if(m_completed||m_cancelled)return false;m_other_confirmed=v;if(!v)m_own_confirmed=false;return true;}
bool cExchangeDialog::CanComplete()const noexcept{bool a=false,b=false;for(auto x:m_own)a|=valid(x);for(auto x:m_other)b|=valid(x);return a&&b&&m_own_confirmed&&m_other_confirmed&&!m_completed&&!m_cancelled;}
bool cExchangeDialog::Complete(){if(!CanComplete())return false;if(m_trade_service){std::vector<mxh::ui::DealItem> own_items,other_items;for(const auto&x:m_own)if(valid(x))own_items.push_back({x.item_id,x.quantity});for(const auto&x:m_other)if(valid(x))other_items.push_back({x.item_id,x.quantity});if(!m_trade_service->completeTrade(own_items,other_items,0u))return false;}m_completed=true;return true;}
void cExchangeDialog::Cancel()noexcept{if(!m_completed){m_cancelled=true;m_own.assign(kSlots,{});m_other.assign(kSlots,{});m_own_confirmed=false;m_other_confirmed=false;}}
}
