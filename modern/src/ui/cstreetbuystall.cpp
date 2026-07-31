#include "cstreetbuystall.hpp"
#include <algorithm>
#include <cstring>
namespace mxh::ui {
cStreetBuyStall::cStreetBuyStall()=default; cStreetBuyStall::~cStreetBuyStall()=default;
void cStreetBuyStall::Linking() {}
void cStreetBuyStall::SetDisable(bool v) noexcept { cDialog::SetDisable(v); }
void cStreetBuyStall::ShowSellStall(){m_state=BuyStallDlgState::Opened; if(m_showCb)m_showCb(m_state,m_showUser);}
void cStreetBuyStall::ShowBuyStall(){m_state=BuyStallDlgState::Sell; if(m_showCb)m_showCb(m_state,m_showUser);}
void cStreetBuyStall::OnCloseStall(bool b){m_state=BuyStallDlgState::NotOpened; if(m_closeCb)m_closeCb(b,m_closeUser);}
void cStreetBuyStall::OnMoneyEditClick(){}
void cStreetBuyStall::OnTitleEditClick(){}
bool cStreetBuyStall::FakeMoveIcon(std::int32_t,std::int32_t,cIcon*){return false;}
void cStreetBuyStall::FakeMoveItem(std::int32_t,std::int32_t,cIcon*){}
void cStreetBuyStall::FakeDeleteItem(std::uint16_t p){DeleteItem(p);}
void cStreetBuyStall::FakeRegistItem(const BuyRegInfo& r,void*){m_fake=r;}
void cStreetBuyStall::OnActionEvnet(std::int32_t,void*,std::uint32_t){}
std::uint32_t cStreetBuyStall::ActionEvent(std::int32_t,std::int32_t,std::uint32_t) noexcept{return kWeNull;}
std::uint32_t cStreetBuyStall::ActionEventWindow(std::int32_t,std::int32_t,std::uint32_t) noexcept{return kWeNull;}
void cStreetBuyStall::SetActive(bool v) noexcept{cDialog::SetActive(v);}
void cStreetBuyStall::OnRegBtn(){}
bool cStreetBuyStall::OnSellBtn(){return false;}
void cStreetBuyStall::RegistMoney(){}
void cStreetBuyStall::RegistMoney(std::uint16_t p,std::uint32_t m){if(p<kStallSlotCount)m_money[p]=m;}
void cStreetBuyStall::RegistTitle(char* t,bool ok){if(ok&&t){std::strncpy(m_oldTitle,t,kTitleMaxLen);m_oldTitle[kTitleMaxLen]=0;}}
void cStreetBuyStall::EditTitle(char* t){if(t)std::strncpy(t,m_oldTitle,kTitleMaxLen+1);}
void cStreetBuyStall::GetTitle(char* t){if(t)std::strncpy(t,m_oldTitle,kTitleMaxLen+1);}
BuyRegInfo cStreetBuyStall::GetBuyRegInfo(std::uint16_t p) const noexcept{return p<kStallSlotCount?m_reg[p]:BuyRegInfo{};}
int cStreetBuyStall::GetCurSelectedItemNum() const noexcept{return m_selected;}
void cStreetBuyStall::SetCurSelectedItemNum(int n) noexcept{m_selected=n;}
void* cStreetBuyStall::GetCurSelectedItem(){return m_selected>=0?GetItem(static_cast<std::uint16_t>(m_selected)):nullptr;}
std::uint32_t cStreetBuyStall::GetCurSelectedItemIdx(){return m_selected>=0?m_reg[static_cast<std::size_t>(m_selected)].item:0;}
void* cStreetBuyStall::GetItem(std::uint16_t p){return p<kStallSlotCount&&m_itemCb?m_itemCb(p,m_itemUser):nullptr;}
std::uint32_t cStreetBuyStall::GetItemMoney(std::uint16_t p) const noexcept{return p<kStallSlotCount?m_money[p]:0;}
bool cStreetBuyStall::AddItem(std::uint16_t p,cIcon*){return p<kStallSlotCount;}
void cStreetBuyStall::DeleteItem(std::uint16_t p){if(p<kStallSlotCount){m_money[p]=0;m_reg[p]=BuyRegInfo{};}}
void cStreetBuyStall::DeleteItemAll(bool){for(std::size_t i=0;i<kStallSlotCount;++i)DeleteItem(static_cast<std::uint16_t>(i));}
void cStreetBuyStall::ResetItemInfo(std::uint16_t p,std::uint16_t v){if(p<kStallSlotCount)m_reg[p].volume=v;}
void cStreetBuyStall::ChangeItemStatus(std::uint16_t p,std::uint16_t v,std::uint32_t m){if(p<kStallSlotCount){m_reg[p].volume=v;m_reg[p].money=m;}}
bool cStreetBuyStall::SelectedItemCheck(){return m_selCb?m_selCb(m_selUser):false;}
bool cStreetBuyStall::MoneyEditCheck(){return m_moneyCb?m_moneyCb(m_moneyUser):false;}
BuyStallDlgState cStreetBuyStall::GetDlgState() const noexcept{return m_state;}
std::uint32_t cStreetBuyStall::GetStallOwnerId() const noexcept{return m_owner;}
void cStreetBuyStall::SetStallOwnerId(std::uint32_t v) noexcept{m_owner=v;}
void cStreetBuyStall::ResetDlgData(){m_state=BuyStallDlgState::NotOpened;m_owner=0;m_data=nullptr;m_selected=-1;DeleteItemAll();}
bool cStreetBuyStall::ShowDivideBox(std::uint16_t){return false;}
void* cStreetBuyStall::GetData() const noexcept{return m_data;}
void cStreetBuyStall::SetData(void* p) noexcept{m_data=p;}
cIconGridDialog* cStreetBuyStall::GetGridDialog() const noexcept{return m_grid;}
std::uint16_t cStreetBuyStall::GetSellItemPos() const noexcept{return m_sellPos;}
void* cStreetBuyStall::GetSellItem() const noexcept{return m_sellItem;}
void cStreetBuyStall::DelSellItem() noexcept{m_sellItem=nullptr;}
bool cStreetBuyStall::SellError(std::uint16_t){return false;}
void cStreetBuyStall::SetSelectedItemCheckCallbackForTest(CheckCallback c,void* u){m_selCb=c;m_selUser=u;}
void cStreetBuyStall::SetMoneyEditCheckCallbackForTest(CheckCallback c,void* u){m_moneyCb=c;m_moneyUser=u;}
void cStreetBuyStall::SetGetItemCallbackForTest(ItemCallback c,void* u){m_itemCb=c;m_itemUser=u;}
void cStreetBuyStall::SetShowCallbackForTest(ShowCallback c,void* u){m_showCb=c;m_showUser=u;}
void cStreetBuyStall::SetCloseCallbackForTest(CloseCallback c,void* u){m_closeCb=c;m_closeUser=u;}
}
