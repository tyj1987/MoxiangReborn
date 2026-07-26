#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <vector>
namespace mxh::ui {
struct ShopEntry { std::uint16_t item_id{}; std::uint32_t price{}; std::uint16_t quantity{1}; };
class cItemShopDialog final : public cDialog {
public:
 using PurchaseCallback=std::function<bool(const ShopEntry&,std::uint16_t)>;
 void SetEntries(std::vector<ShopEntry> entries){m_entries=std::move(entries);}
 void SetMoney(std::uint32_t money) noexcept {m_money=money;}
 std::uint32_t GetMoney() const noexcept{return m_money;}
 void SetPurchaseCallback(PurchaseCallback cb){m_purchase=std::move(cb);}
 bool Buy(std::size_t index,std::uint16_t quantity=1);
 std::uint32_t TotalPrice(std::size_t index,std::uint16_t quantity=1) const noexcept;
 const std::vector<ShopEntry>& Entries() const noexcept{return m_entries;}
private:
 std::vector<ShopEntry> m_entries; std::uint32_t m_money{}; PurchaseCallback m_purchase;
};
}
