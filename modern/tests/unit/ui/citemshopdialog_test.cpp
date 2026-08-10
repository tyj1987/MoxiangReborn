#include "citemshopdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(ItemShopDialog, CalculatesAndCompletesPurchase){cItemShopDialog d;d.SetEntries({{101,25,1}});d.SetMoney(100);ShopEntry got{};std::uint16_t qty=0;d.SetPurchaseCallback([&](const ShopEntry&e,std::uint16_t q){got=e;qty=q;return true;});EXPECT_EQ(d.TotalPrice(0,3),75u);EXPECT_TRUE(d.Buy(0,3));EXPECT_EQ(d.GetMoney(),25u);EXPECT_EQ(got.item_id,101);EXPECT_EQ(qty,3);}
TEST(ItemShopDialog, RejectsInsufficientFundsAndInvalidRows){cItemShopDialog d;d.SetEntries({{101,25,1}});d.SetMoney(20);EXPECT_FALSE(d.Buy(0));EXPECT_EQ(d.GetMoney(),20u);EXPECT_FALSE(d.Buy(4));EXPECT_EQ(d.TotalPrice(4),0u);}
TEST(ItemShopDialog, CallbackFailureDoesNotCharge){cItemShopDialog d;d.SetEntries({{101,25,1}});d.SetMoney(50);d.SetPurchaseCallback([](const ShopEntry&,std::uint16_t){return false;});EXPECT_FALSE(d.Buy(0));EXPECT_EQ(d.GetMoney(),50u);}

#include "mxh/services/IItemShopService.hpp"
namespace {
// In-memory mock that lets tests stage catalog + money state.
struct MockShopService final : mxh::services::IItemShopService {
 std::vector<mxh::services::ShopEntry> catalog;
 std::uint32_t money = 0;
 mutable int has_enough_calls = 0;
 std::size_t shopEntryCount() const noexcept override { return catalog.size(); }
 std::optional<mxh::services::ShopEntry> getShopEntry(std::size_t i) const noexcept override {
  if (i >= catalog.size()) return std::nullopt;
  return catalog[i];
 }
 std::uint32_t playerMoney() const noexcept override { return money; }
 bool hasEnoughMoney(std::uint32_t amount) const noexcept override {
  ++has_enough_calls;
  return money >= amount;
 }
};
}

TEST(ItemShopDialog, ShopServiceConsultsCatalogAndMoney) {
  MockShopService svc;
  svc.catalog.push_back({101, 25, 1});
  svc.catalog.push_back({102, 50, 1});
  svc.money = 100;
  cItemShopDialog d;
  d.SetShopService(&svc);
  // GetMoney falls through to service->playerMoney() when bound.
  EXPECT_EQ(d.GetMoney(), 100u);
  // TotalPrice uses service->getShopEntry(0).price * qty.
  EXPECT_EQ(d.TotalPrice(0, 3), 75u);
  EXPECT_EQ(d.TotalPrice(1, 2), 100u);
  // Buy validates via service->hasEnoughMoney() before invoking the callback.
  ShopEntry got{}; std::uint16_t qty = 0;
  d.SetPurchaseCallback([&](const ShopEntry& e, std::uint16_t q) { got = e; qty = q; return true; });
  EXPECT_TRUE(d.Buy(0, 3));
  EXPECT_EQ(got.item_id, 101); EXPECT_EQ(qty, 3);
  EXPECT_EQ(svc.has_enough_calls, 1);
  // Service-bound mode does NOT mutate local m_money; service owns its state.
  EXPECT_EQ(svc.money, 100u);
}

TEST(ItemShopDialog, ShopServiceRejectsInsufficientFunds) {
  MockShopService svc;
  svc.catalog.push_back({101, 25, 1});
  svc.money = 10;
  cItemShopDialog d;
  d.SetShopService(&svc);
  int cb_calls = 0;
  d.SetPurchaseCallback([&](const ShopEntry&, std::uint16_t) { ++cb_calls; return true; });
  EXPECT_FALSE(d.Buy(0));
  EXPECT_EQ(cb_calls, 0);
  EXPECT_EQ(svc.has_enough_calls, 1);
}

TEST(ItemShopDialog, ShopServiceRejectsOutOfRangeIndex) {
  MockShopService svc;
  svc.catalog.push_back({101, 25, 1});
  svc.money = 100;
  cItemShopDialog d;
  d.SetShopService(&svc);
  int cb_calls = 0;
  d.SetPurchaseCallback([&](const ShopEntry&, std::uint16_t) { ++cb_calls; return true; });
  EXPECT_FALSE(d.Buy(5));
  EXPECT_EQ(cb_calls, 0);
  EXPECT_FALSE(d.Buy(0, 0));  // qty=0 still invalid
  EXPECT_EQ(cb_calls, 0);
}

TEST(ItemShopDialog, ShopServiceReflectsLiveEconomy) {
  MockShopService svc;
  svc.catalog.push_back({101, 25, 1});
  svc.money = 30;
  cItemShopDialog d;
  d.SetShopService(&svc);
  EXPECT_EQ(d.GetMoney(), 30u);
  // Simulate economy change (e.g. another NPC sale orquest reward) without
  // re-binding the service. GetMoney reflects the live snapshot.
  svc.money = 5;
  EXPECT_EQ(d.GetMoney(), 5u);
  EXPECT_FALSE(d.Buy(0));
}

TEST(ItemShopDialog, ShopServiceClearFallsBackToLocalSnapshot) {
  MockShopService svc;
  svc.catalog.push_back({101, 25, 1});
  svc.money = 0;
  cItemShopDialog d;
  d.SetEntries({{101, 25, 1}});
  d.SetMoney(50);
  d.SetShopService(&svc);
  EXPECT_EQ(d.GetMoney(), 0u);  // service overrides local snapshot
  d.SetShopService(nullptr);
  EXPECT_EQ(d.GetMoney(), 50u); // cleared -> local snapshot again
  EXPECT_TRUE(d.Buy(0, 2));     // local mode still works (25*2=50)
  EXPECT_EQ(d.GetMoney(), 0u);  // local mode deducts on purchase
}