#include "cmovedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MoveDialog, SelectsAndConfirmsDestination){cMoveDialog d;d.AddMoveInfo({1,"Town",true});d.AddMoveInfo({2,"Saved",false});d.SetTownMoveView(true);EXPECT_TRUE(d.SelectMoveIdx(0));MovePoint selected{};d.SetMoveCallback([&](const auto&p){selected=p;return true;});EXPECT_TRUE(d.MapMoveOK());EXPECT_EQ(selected.db_id,1u);}
TEST(MoveDialog, FiltersTownAndSavedViews){cMoveDialog d;d.AddMoveInfo({1,"Town",true});d.AddMoveInfo({2,"Saved",false});d.SetTownMoveView(true);EXPECT_FALSE(d.SelectMoveIdx(1));d.SetTownMoveView(false);EXPECT_TRUE(d.SelectMoveIdx(1));}
TEST(MoveDialog, UpdatesDeletesAndRejectsMissingSelection){cMoveDialog d;d.AddMoveInfo({1,"Old",false});EXPECT_TRUE(d.UpdateMoveInfo({1,"New",false}));EXPECT_EQ(d.Points()[0].name,"New");EXPECT_TRUE(d.DeleteMoveInfo(1));EXPECT_FALSE(d.MapMoveOK());}
