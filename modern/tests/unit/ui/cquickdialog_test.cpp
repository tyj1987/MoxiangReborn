#include "cquickdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
class QuickInventory final : public mxh::services::IInventoryService {
public:
 bool present=false;
 const mxh::game::ItemBase* getItem(std::uint16_t) const noexcept override{return nullptr;}
 std::uint16_t occupiedSlotCount() const noexcept override{return 0;}
 std::uint16_t totalCapacity() const noexcept override{return 80;}
 const mxh::game::ItemBase* getWearedItem(std::uint8_t) const noexcept override{return nullptr;}
 bool isWearedSlotOccupied(std::uint8_t) const noexcept override{return false;}
 std::optional<std::uint16_t> findItemByIconIdx(std::uint16_t) const noexcept override{return present?std::optional<std::uint16_t>(1):std::nullopt;}
 bool hasItem(std::uint16_t) const noexcept override{return present;}
};
class QuickSkills final : public mxh::services::ISkillService {
public:
 bool learned=false;
 std::uint32_t learnedSkillCount() const noexcept override{return learned?1:0;}
 std::uint32_t getLearnedSkillAt(std::uint32_t) const noexcept override{return 7;}
 bool isLearned(std::uint32_t) const noexcept override{return learned;}
 std::optional<std::uint8_t> getSkillLevel(std::uint32_t) const noexcept override{return learned?std::optional<std::uint8_t>(1):std::nullopt;}
 std::optional<std::uint8_t> getQuickSlotBinding(std::uint32_t) const noexcept override{return std::nullopt;}
};
TEST(QuickDialog, BindsAndActivatesCurrentPage){cQuickDialog d;EXPECT_TRUE(d.Bind(0,2,QuickKind::Skill,1001));d.SelectPage(0);QuickSlot got{};d.SetActivateCallback([](QuickSlot s,void*p){*static_cast<QuickSlot*>(p)=s;},&got);EXPECT_TRUE(d.Activate(2));EXPECT_EQ(got.id,1001u);EXPECT_EQ(got.kind,QuickKind::Skill);}
TEST(QuickDialog, PagesAndInvalidSlotsAreIsolated){cQuickDialog d;EXPECT_TRUE(d.Bind(1,0,QuickKind::Item,22));d.SelectPage(0);EXPECT_FALSE(d.Activate(0));d.SelectPage(1);EXPECT_TRUE(d.Activate(0));EXPECT_FALSE(d.Bind(3,0,QuickKind::Item,1));}
TEST(QuickDialog, RemovesSlots){cQuickDialog d;d.Bind(0,0,QuickKind::Ability,7);EXPECT_TRUE(d.Remove(0,0));EXPECT_FALSE(d.Get(0,0).has_value());EXPECT_FALSE(d.Remove(0,0));}
TEST(QuickDialog, ServiceBackedBindingsRejectUnknownState){cQuickDialog d;QuickInventory inv;QuickSkills skills;d.SetInventoryService(&inv);d.SetSkillService(&skills);EXPECT_FALSE(d.BindItemFromInventory(0,0,10));EXPECT_FALSE(d.BindSkillFromService(0,1,7));inv.present=true;skills.learned=true;EXPECT_TRUE(d.BindItemFromInventory(0,0,10));EXPECT_TRUE(d.BindSkillFromService(0,1,7));EXPECT_EQ(d.Get(0,0)->kind,QuickKind::Item);EXPECT_EQ(d.Get(0,1)->kind,QuickKind::Skill);}
