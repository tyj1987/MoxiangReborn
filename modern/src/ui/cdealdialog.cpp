#include "cdealdialog.hpp"
namespace mxh::ui {
bool cDealDialog::AddOwnItem(DealItem i){if(m_confirmed||m_cancelled||i.item_id==0||i.quantity==0)return false;m_own_items.push_back(i);return true;}
bool cDealDialog::AddOtherItem(DealItem i){if(m_confirmed||m_cancelled||i.item_id==0||i.quantity==0)return false;m_other_items.push_back(i);return true;}
bool cDealDialog::Confirm(){if(m_cancelled||m_confirmed||m_own_items.empty()&&m_other_items.empty())return false;if(m_complete&&!m_complete(m_own_items,m_other_items,NetMoney()))return false;m_confirmed=true;return true;}
void cDealDialog::Cancel()noexcept{if(!m_confirmed){m_cancelled=true;m_own_items.clear();m_other_items.clear();m_own_money=0;m_other_money=0;}}
}
