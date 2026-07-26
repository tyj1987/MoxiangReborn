#include "cnpcscript_dialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(NpcScriptDialog, SelectsAndExecutesOption){cNpcScriptDialog d;d.SetText("Welcome");d.SetOptions({{10,"Shop"},{20,"Leave"}});std::uint32_t action=0;d.SetActionCallback([&](auto a){action=a;return true;});EXPECT_TRUE(d.Select(0));EXPECT_TRUE(d.ExecuteSelected());EXPECT_EQ(action,10u);EXPECT_EQ(d.Text(),"Welcome");}
TEST(NpcScriptDialog, RejectsInvalidOptionsAndSelection){cNpcScriptDialog d;d.SetOptions({{0,"Bad"},{20,"Leave"}});EXPECT_FALSE(d.Select(0));EXPECT_FALSE(d.ExecuteSelected());EXPECT_TRUE(d.Select(1));}
TEST(NpcScriptDialog, CallbackFailureDoesNotChangeSelection){cNpcScriptDialog d;d.SetOptions({{20,"Leave"}});d.Select(0);d.SetActionCallback([](auto){return false;});EXPECT_FALSE(d.ExecuteSelected());EXPECT_EQ(d.SelectedIndex(),0);}
