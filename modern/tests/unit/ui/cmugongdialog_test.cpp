#include "cmugongdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MugongDialog, SetsSelectsAndEnablesSkill){cMugongDialog d;EXPECT_TRUE(d.SetSlot(0,{100,"Slash",false}));EXPECT_TRUE(d.Select(0));EXPECT_TRUE(d.SetEnabled(0,true));ASSERT_NE(d.Selected(),nullptr);EXPECT_TRUE(d.Selected()->enabled);}
TEST(MugongDialog, RejectsInvalidSlotsAndEntries){cMugongDialog d;EXPECT_FALSE(d.SetSlot(12,{1,"Bad",true}));EXPECT_FALSE(d.SetSlot(0,{0,"",true}));EXPECT_FALSE(d.Select(0));}
TEST(MugongDialog, ClearsSelectedSlot){cMugongDialog d;d.SetSlot(0,{1,"Slash",true});d.Select(0);EXPECT_TRUE(d.ClearSlot(0));EXPECT_EQ(d.Selected(),nullptr);EXPECT_FALSE(d.ClearSlot(0));}
