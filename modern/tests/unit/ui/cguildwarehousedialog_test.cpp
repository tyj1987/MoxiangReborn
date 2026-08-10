#include "cguildwarehousedialog.hpp"
#include "mxh/services/IInventoryService.hpp"
#include <gtest/gtest.h>

using namespace mxh::ui;
namespace {
struct WarehouseInventory final : mxh::services::IInventoryService {
    bool present{};
    const mxh::game::ItemBase* getItem(std::uint16_t) const noexcept override { return nullptr; }
    std::uint16_t occupiedSlotCount() const noexcept override { return 0; }
    std::uint16_t totalCapacity() const noexcept override { return 80; }
    const mxh::game::ItemBase* getWearedItem(std::uint8_t) const noexcept override { return nullptr; }
    bool isWearedSlotOccupied(std::uint8_t) const noexcept override { return false; }
    std::optional<std::uint16_t> findItemByIconIdx(std::uint16_t) const noexcept override { return present ? std::optional<std::uint16_t>{0} : std::nullopt; }
    bool hasItem(std::uint16_t) const noexcept override { return present; }
};
}

TEST(GuildWarehouseDialog, StoresTakesAndMovesItems){cGuildWarehouseDialog d;d.SetPermission(true,true);EXPECT_TRUE(d.Store(0,{1,5}));EXPECT_TRUE(d.Move(0,3));auto x=d.Take(3);ASSERT_TRUE(x);EXPECT_EQ(x->item_id,1);}
TEST(GuildWarehouseDialog, EnforcesPermissionsAndLock){cGuildWarehouseDialog d;d.SetPermission(true,false);EXPECT_TRUE(d.Store(0,{1,1}));EXPECT_FALSE(d.Take(0));d.SetLocked(true);EXPECT_FALSE(d.Store(1,{2,1}));}
TEST(GuildWarehouseDialog, RejectsOccupiedAndInvalidSlots){cGuildWarehouseDialog d;d.SetPermission(true,true);EXPECT_TRUE(d.Store(0,{1,1}));EXPECT_FALSE(d.Store(0,{2,1}));EXPECT_FALSE(d.Store(60,{2,1}));EXPECT_FALSE(d.Store(1,{0,1}));}
TEST(GuildWarehouseDialog, ServiceBackedStoreRequiresInventory){cGuildWarehouseDialog d;WarehouseInventory inventory;d.SetPermission(true,true);d.SetInventoryService(&inventory);EXPECT_FALSE(d.Store(0,{9,1}));inventory.present=true;EXPECT_TRUE(d.Store(0,{9,1}));}
