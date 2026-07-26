#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <vector>
namespace mxh::ui {
struct DealItem { std::uint16_t item_id{}; std::uint16_t quantity{}; };
class cDealDialog final : public cDialog {
public:
 using CompleteCallback=std::function<bool(const std::vector<DealItem>&,const std::vector<DealItem>&,std::uint32_t)>;
 bool AddOwnItem(DealItem item); bool AddOtherItem(DealItem item);
 void SetOtherMoney(std::uint32_t money) noexcept {m_other_money=money;}
 void SetOwnMoney(std::uint32_t money) noexcept {m_own_money=money;}
 void SetCompleteCallback(CompleteCallback cb){m_complete=std::move(cb);}
 bool Confirm(); void Cancel() noexcept;
 bool IsConfirmed() const noexcept{return m_confirmed;} bool IsCancelled() const noexcept{return m_cancelled;}
 std::uint32_t NetMoney() const noexcept{return m_other_money>m_own_money?m_other_money-m_own_money:0;}
 const std::vector<DealItem>& OwnItems()const noexcept{return m_own_items;}
 const std::vector<DealItem>& OtherItems()const noexcept{return m_other_items;}
private:
 std::vector<DealItem> m_own_items,m_other_items; std::uint32_t m_own_money{},m_other_money{}; bool m_confirmed{},m_cancelled{}; CompleteCallback m_complete;
};
}
