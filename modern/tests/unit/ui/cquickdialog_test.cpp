#include "cquickdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(QuickDialog, BindsAndActivatesCurrentPage){cQuickDialog d;EXPECT_TRUE(d.Bind(0,2,QuickKind::Skill,1001));d.SelectPage(0);QuickSlot got{};d.SetActivateCallback([](QuickSlot s,void*p){*static_cast<QuickSlot*>(p)=s;},&got);EXPECT_TRUE(d.Activate(2));EXPECT_EQ(got.id,1001u);EXPECT_EQ(got.kind,QuickKind::Skill);}
TEST(QuickDialog, PagesAndInvalidSlotsAreIsolated){cQuickDialog d;EXPECT_TRUE(d.Bind(1,0,QuickKind::Item,22));d.SelectPage(0);EXPECT_FALSE(d.Activate(0));d.SelectPage(1);EXPECT_TRUE(d.Activate(0));EXPECT_FALSE(d.Bind(3,0,QuickKind::Item,1));}
TEST(QuickDialog, RemovesSlots){cQuickDialog d;d.Bind(0,0,QuickKind::Ability,7);EXPECT_TRUE(d.Remove(0,0));EXPECT_FALSE(d.Get(0,0).has_value());EXPECT_FALSE(d.Remove(0,0));}
