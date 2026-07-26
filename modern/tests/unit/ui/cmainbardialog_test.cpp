#include "cmainbardialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MainBarDialog, SetsQueriesAndTogglesButtons){cMainBarDialog d;EXPECT_TRUE(d.SetButton(10,true));EXPECT_TRUE(d.IsButtonVisible(10));std::uint32_t id=0;bool state=false;d.SetToggleCallback([&](auto i,bool s){id=i;state=s;});EXPECT_TRUE(d.Toggle(10));EXPECT_FALSE(d.IsButtonVisible(10));EXPECT_EQ(id,10u);EXPECT_FALSE(state);}
TEST(MainBarDialog, RejectsUnknownAndInvalidButtons){cMainBarDialog d;EXPECT_FALSE(d.SetButton(0,true));EXPECT_FALSE(d.Toggle(10));EXPECT_FALSE(d.IsButtonVisible(10));}
TEST(MainBarDialog, ControlsOverallVisibility){cMainBarDialog d;d.SetVisible(false);EXPECT_FALSE(d.IsVisible());d.SetVisible(true);EXPECT_TRUE(d.IsVisible());}
