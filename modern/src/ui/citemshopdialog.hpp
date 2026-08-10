#pragma once
#include "mxh/ui/cDialog.hpp"
#include "mxh/services/IItemShopService.hpp"
#include <cstdint>
#include <functional>
#include <vector>
namespace mxh::ui {
using ShopEntry = mxh::services::ShopEntry;
class cItemShopDialog final : public cDialog {
public:
 using PurchaseCallback=std::function<bool(const ShopEntry&,std::uint16_t)>;
 void SetEntries(std::vector<ShopEntry> entries){m_entries=std::move(entries);}
 void SetMoney(std::uint32_t money) noexcept {m_money=money;}
 std::uint32_t GetMoney() const noexcept;
 void SetPurchaseCallback(PurchaseCallback cb){m_purchase=std::move(cb);}
 // IItemShopService is the modern economy/catalog source of truth. When set,
 // Buy() validates against service->hasEnoughMoney() (instead of the local
 // m_money snapshot) and the catalog lookup prefers service->getShopEntry()
 // for index validation. The local m_entries/m_money remain valid fallbacks
 // so existing visual-render code paths keep working in unit tests and the
 // legacy NPC types that have not been wired to a shop service yet.
 void SetShopService(mxh::services::IItemShopService* service) noexcept {m_shop_service=service;}
 mxh::services::IItemShopService* GetShopService() const noexcept {return m_shop_service;}
 bool Buy(std::size_t index,std::uint16_t quantity=1);
 std::uint32_t TotalPrice(std::size_t index,std::uint16_t quantity=1) const noexcept;
 const std::vector<ShopEntry>& Entries() const noexcept{return m_entries;}
private:
 std::vector<ShopEntry> m_entries; std::uint32_t m_money{}; PurchaseCallback m_purchase; mxh::services::IItemShopService* m_shop_service{};
};
}
