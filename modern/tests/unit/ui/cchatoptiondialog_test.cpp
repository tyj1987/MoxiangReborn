#include "cchatoptiondialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(ChatOptionDialog, StoresChatOptions){cChatOptionDialog d;d.SetChatEnabled(false);d.SetWhisperEnabled(false);d.SetFilterMask(7);EXPECT_FALSE(d.ChatEnabled());EXPECT_FALSE(d.WhisperEnabled());EXPECT_EQ(d.FilterMask(),7u);}
TEST(ChatOptionDialog, ResetRestoresDefaults){cChatOptionDialog d;d.SetChatEnabled(false);d.SetFilterMask(99);d.Reset();EXPECT_TRUE(d.ChatEnabled());EXPECT_TRUE(d.WhisperEnabled());EXPECT_EQ(d.FilterMask(),0u);}
