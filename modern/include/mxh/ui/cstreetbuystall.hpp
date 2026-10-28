#pragma once
#include "cdialog.hpp"
#include <array>
#include <cstdint>
namespace mxh::ui {
struct BuyRegInfo { std::uint16_t type=0; std::uint16_t item=0; std::uint16_t itemClass=0; std::uint16_t volume=0; std::uint32_t money=0; };
enum class BuyStallDlgState : std::uint8_t { NotOpened=0, Opened=1, Sell=2 };
class cIcon; class cIconGridDialog; class cEditBox; class cButton;
class cStreetBuyStall : public cDialog {
public:
static constexpr std::size_t kStallSlotCount=5; static constexpr std::size_t kTitleMaxLen=66; static constexpr const char* kDefaultMoneyText="0";
static constexpr std::int32_t kIdDialog=379; static constexpr std::int32_t kIdIconGrid=380; static constexpr std::int32_t kIdTitleEdit=381; static constexpr std::int32_t kIdEnter=382; static constexpr std::int32_t kIdRegistBtn=384; static constexpr std::int32_t kIdSellBtn=383; static constexpr std::int32_t kIdMoneyEdit=386;
cStreetBuyStall(); ~cStreetBuyStall() override; cStreetBuyStall(const cStreetBuyStall&)=delete; cStreetBuyStall& operator=(const cStreetBuyStall&)=delete;
void Linking(); void ShowSellStall(); void ShowBuyStall(); void OnCloseStall(bool=false); void OnMoneyEditClick(); void OnTitleEditClick(); bool FakeMoveIcon(std::int32_t,std::int32_t,cIcon*); void FakeMoveItem(std::int32_t,std::int32_t,cIcon*); void SetDisable(bool) noexcept override; void FakeDeleteItem(std::uint16_t); void FakeRegistItem(const BuyRegInfo&,void*);
void OnActionEvnet(std::int32_t,void*,std::uint32_t); std::uint32_t ActionEvent(std::int32_t,std::int32_t,std::uint32_t) noexcept override; std::uint32_t ActionEventWindow(std::int32_t,std::int32_t,std::uint32_t) noexcept; void SetActive(bool) noexcept override; void OnRegBtn(); bool OnSellBtn();
void RegistMoney(); void RegistMoney(std::uint16_t,std::uint32_t); void RegistTitle(char*,bool); void EditTitle(char*); void GetTitle(char*); BuyRegInfo GetBuyRegInfo(std::uint16_t) const noexcept; int GetCurSelectedItemNum() const noexcept; void SetCurSelectedItemNum(int) noexcept; void* GetCurSelectedItem(); std::uint32_t GetCurSelectedItemIdx(); void* GetItem(std::uint16_t); std::uint32_t GetItemMoney(std::uint16_t) const noexcept; bool AddItem(std::uint16_t,cIcon*); void DeleteItem(std::uint16_t); void DeleteItemAll(bool=false); void ResetItemInfo(std::uint16_t,std::uint16_t); void ChangeItemStatus(std::uint16_t,std::uint16_t,std::uint32_t); bool SelectedItemCheck(); bool MoneyEditCheck();
BuyStallDlgState GetDlgState() const noexcept; std::uint32_t GetStallOwnerId() const noexcept; void SetStallOwnerId(std::uint32_t) noexcept; void ResetDlgData(); bool ShowDivideBox(std::uint16_t); void* GetData() const noexcept; void SetData(void*) noexcept; cIconGridDialog* GetGridDialog() const noexcept; std::uint16_t GetSellItemPos() const noexcept; void* GetSellItem() const noexcept; void DelSellItem() noexcept; bool SellError(std::uint16_t=1218);
using CheckCallback=bool(*)(void*); using ItemCallback=void*(*)(std::uint16_t,void*); using ShowCallback=void(*)(BuyStallDlgState,void*); using CloseCallback=void(*)(bool,void*); void SetSelectedItemCheckCallbackForTest(CheckCallback,void*); void SetMoneyEditCheckCallbackForTest(CheckCallback,void*); void SetGetItemCallbackForTest(ItemCallback,void*); void SetShowCallbackForTest(ShowCallback,void*); void SetCloseCallbackForTest(CloseCallback,void*);
static constexpr std::uint32_t kWeNull=0;
private:
BuyStallDlgState m_state=BuyStallDlgState::NotOpened; cEditBox* m_titleEdit=nullptr; cEditBox* m_moneyEdit=nullptr; cButton* m_enterBtn=nullptr; cIconGridDialog* m_grid=nullptr; cButton* m_regBtn=nullptr; cButton* m_sellBtn=nullptr; std::array<std::uint32_t,kStallSlotCount> m_money{}; std::array<BuyRegInfo,kStallSlotCount> m_reg{}; BuyRegInfo m_fake{}; std::uint32_t m_totalMoney=0; char m_oldTitle[kTitleMaxLen+1]{}; int m_selected=-1; std::uint32_t m_owner=0; void* m_data=nullptr; void* m_sellItem=nullptr; std::uint16_t m_sellPos=0; CheckCallback m_selCb=nullptr; void* m_selUser=nullptr; CheckCallback m_moneyCb=nullptr; void* m_moneyUser=nullptr; ItemCallback m_itemCb=nullptr; void* m_itemUser=nullptr; ShowCallback m_showCb=nullptr; void* m_showUser=nullptr; CloseCallback m_closeCb=nullptr; void* m_closeUser=nullptr; };
}

