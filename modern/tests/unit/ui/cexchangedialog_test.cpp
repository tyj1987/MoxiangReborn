#include "cexchangedialog.hpp"
#include <gtest/gtest.h>
#include <set>
using namespace mxh::ui;
TEST(ExchangeDialog, RequiresBothSidesToConfirm){cExchangeDialog d;EXPECT_TRUE(d.SetOwn(0,{1,2}));EXPECT_TRUE(d.SetOther(0,{2,1}));EXPECT_TRUE(d.SetOwnConfirmed(true));EXPECT_FALSE(d.CanComplete());EXPECT_TRUE(d.SetOtherConfirmed(true));EXPECT_TRUE(d.CanComplete());EXPECT_TRUE(d.Complete());}
TEST(ExchangeDialog, ChangingItemsInvalidatesConfirmation){cExchangeDialog d;d.SetOwn(0,{1,1});d.SetOther(0,{2,1});EXPECT_TRUE(d.SetOwn(1,{3,1}));EXPECT_TRUE(d.SetOwnConfirmed(true));EXPECT_TRUE(d.SetOtherConfirmed(true));EXPECT_TRUE(d.CanComplete());}
TEST(ExchangeDialog, CancelClearsBothSides){cExchangeDialog d;d.SetOwn(0,{1,1});d.SetOther(0,{2,1});d.Cancel();EXPECT_TRUE(d.IsCancelled());EXPECT_EQ(d.Own()[0].item_id,0);EXPECT_EQ(d.Other()[0].item_id,0);EXPECT_FALSE(d.SetOwn(1,{3,1}));}


#include "mxh/services/IInventoryService.hpp"
#include "cdealdialog.hpp"  // for mxh::ui::DealItem full definition
#include "mxh/services/ITradeService.hpp"
namespace {
// Minimal IInventoryService mock that answers hasItem() from a configurable set.
struct ExchangeInventory final : mxh::services::IInventoryService {
 std::set<std::uint16_t> owned;
 const mxh::game::ItemBase* getItem(std::uint16_t) const noexcept override {return nullptr;}
 std::uint16_t occupiedSlotCount() const noexcept override {return 0;}
 std::uint16_t totalCapacity() const noexcept override {return 80;}
 const mxh::game::ItemBase* getWearedItem(std::uint8_t) const noexcept override {return nullptr;}
 bool isWearedSlotOccupied(std::uint8_t) const noexcept override {return false;}
 std::optional<std::uint16_t> findItemByIconIdx(std::uint16_t idx) const noexcept override {return owned.count(idx)?std::optional<std::uint16_t>{0}:std::nullopt;}
 bool hasItem(std::uint16_t idx) const noexcept override {return owned.count(idx)!=0;}
};
// ITradeService mock that records completeTrade calls and supports reject mode.
struct ExchangeTradeService final : mxh::services::ITradeService {
 bool allow = true;
 int calls = 0;
 std::uint32_t last_net_money = 999u;
 bool completeTrade(const std::vector<mxh::ui::DealItem>& own_items,
                    const std::vector<mxh::ui::DealItem>& other_items,
                    std::uint32_t net_money) override {
  ++calls;
  last_net_money = net_money;
  last_own = own_items; last_other = other_items;
  return allow;
 }
 std::vector<mxh::ui::DealItem> last_own;
 std::vector<mxh::ui::DealItem> last_other;
};
}

TEST(ExchangeDialog, ServiceGatesOwnItemOnInventory) {
  ExchangeInventory inv;
  cExchangeDialog d;
  d.SetInventoryService(&inv);
  EXPECT_FALSE(d.SetOwn(0, {1, 2}));  // not in inventory
  inv.owned.insert(1);
  EXPECT_TRUE(d.SetOwn(0, {1, 2}));   // now in inventory
  EXPECT_FALSE(d.SetOwn(1, {2, 1}));  // still not in inventory
}

TEST(ExchangeDialog, ServiceDrivesCompleteCommit) {
  ExchangeInventory inv;
  ExchangeTradeService trade;
  inv.owned.insert(1); inv.owned.insert(2);
  cExchangeDialog d;
  d.SetInventoryService(&inv);
  d.SetTradeService(&trade);
  ASSERT_TRUE(d.SetOwn(0, {1, 1}));
  ASSERT_TRUE(d.SetOther(0, {2, 1}));
  ASSERT_TRUE(d.SetOwnConfirmed(true));
  ASSERT_TRUE(d.SetOtherConfirmed(true));
  EXPECT_TRUE(d.CanComplete());
  EXPECT_TRUE(d.Complete());
  EXPECT_EQ(trade.calls, 1);
  EXPECT_EQ(trade.last_own.size(), 1u);
  EXPECT_EQ(trade.last_other.size(), 1u);
  EXPECT_EQ(trade.last_own[0].item_id, 1);
  EXPECT_EQ(trade.last_other[0].item_id, 2);
  EXPECT_EQ(trade.last_net_money, 0u);  // exchange has no money settlement
  EXPECT_TRUE(d.IsCompleted());
}

TEST(ExchangeDialog, TradeServiceRejectionLeavesExchangeOpen) {
  ExchangeInventory inv;
  ExchangeTradeService trade;
  inv.owned.insert(1); inv.owned.insert(2);
  trade.allow = false;
  cExchangeDialog d;
  d.SetInventoryService(&inv);
  d.SetTradeService(&trade);
  d.SetOwn(0, {1, 1}); d.SetOther(0, {2, 1});
  d.SetOwnConfirmed(true); d.SetOtherConfirmed(true);
  EXPECT_FALSE(d.Complete());  // trade service says no
  EXPECT_FALSE(d.IsCompleted());
  EXPECT_EQ(trade.calls, 1);
  // CanComplete remains true (both confirmed + items set), so retry is allowed.
  EXPECT_TRUE(d.CanComplete());
  trade.allow = true;
  EXPECT_TRUE(d.Complete());
  EXPECT_EQ(trade.calls, 2);
}

TEST(ExchangeDialog, ClearServiceFallsBackToLocalInventory) {
  ExchangeInventory inv;
  inv.owned.insert(7);
  cExchangeDialog d;
  d.SetInventoryService(&inv);
  EXPECT_TRUE(d.SetOwn(0, {7, 1}));  // gated by service
  d.SetInventoryService(nullptr);
  EXPECT_TRUE(d.SetOwn(1, {99, 1})); // no service: local mode accepts (legacy behavior)
}