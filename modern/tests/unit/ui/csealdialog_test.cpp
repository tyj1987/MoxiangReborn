#include "csealdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(SealDialog, SealsUnsealedItem){cSealDialog d;d.SetMode(SealMode::Seal);EXPECT_TRUE(d.SetItem({1,false}));SealMode mode{};d.SetCallback([&](auto&,auto m){mode=m;return true;});EXPECT_TRUE(d.Execute());EXPECT_TRUE(d.Done());EXPECT_TRUE(d.Item()->sealed);EXPECT_EQ(mode,SealMode::Seal);}
TEST(SealDialog, UnsealRequiresSealedItem){cSealDialog d;d.SetMode(SealMode::Unseal);EXPECT_FALSE(d.SetItem({1,false}));EXPECT_TRUE(d.SetItem({1,true}));EXPECT_TRUE(d.Execute());EXPECT_FALSE(d.Item()->sealed);}
TEST(SealDialog, CallbackFailureLeavesPending){cSealDialog d;d.SetItem({1,false});d.SetCallback([](auto&,auto){return false;});EXPECT_FALSE(d.Execute());EXPECT_FALSE(d.Done());}
