#include "crarecreatedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(RareCreateDialog, ValidatesMaterialsAndCreates){cRareCreateDialog d;d.SetRecipe(99,{{1,2},{2,1}});EXPECT_TRUE(d.SetMaterial(0,{1,2}));EXPECT_TRUE(d.SetMaterial(1,{2,1}));std::uint32_t result=0;d.SetCreateCallback([&](auto id){result=id;return true;});EXPECT_TRUE(d.CanCreate());EXPECT_TRUE(d.Create());EXPECT_EQ(result,99u);}
TEST(RareCreateDialog, RejectsInsufficientMaterials){cRareCreateDialog d;d.SetRecipe(99,{{1,2}});EXPECT_FALSE(d.SetMaterial(0,{1,1}));EXPECT_FALSE(d.CanCreate());EXPECT_FALSE(d.Create());}
TEST(RareCreateDialog, CallbackFailureDoesNotCorruptRecipe){cRareCreateDialog d;d.SetRecipe(99,{{1,1}});d.SetMaterial(0,{1,1});d.SetCreateCallback([](auto){return false;});EXPECT_FALSE(d.Create());EXPECT_TRUE(d.CanCreate());}
