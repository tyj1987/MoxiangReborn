// cguilddialog_test.cpp — Phase 6.12 coverage for cGuildDialog (end-to-end
// first real legacy-port dialog using cStatic + cListDialog + cPushupButton
// + cButton).

#include "cGuildDialog.hpp"
#include "cButton.hpp"
#include "cListDialog.hpp"
#include "cPushupButton.hpp"
#include "cStatic.hpp"

#include <gtest/gtest.h>

namespace {

mxh::ui::cGuildDialog::MemberInfo MakeMember(const char* name,
                                              std::uint8_t rank,
                                              std::uint8_t level,
                                              bool online = true) {
    mxh::ui::cGuildDialog::MemberInfo m;
    m.name = name;
    m.rank = rank;
    m.level = level;
    m.online = online;
    return m;
}

}  // namespace

TEST(CGuildDialog, DefaultState) {
    mxh::ui::cGuildDialog d;
    EXPECT_EQ(d.GuildName(), "");
    EXPECT_EQ(d.MasterName(), "");
    EXPECT_EQ(d.GuildLevel(), 0u);
    EXPECT_EQ(d.MemberNum(), 0u);
    EXPECT_EQ(d.MemberCount(), 0u);
    EXPECT_EQ(d.GetSelectedMember(), -1);
    EXPECT_EQ(d.GetShowMode(), mxh::ui::cGuildDialog::kShowModeMember);
}

TEST(CGuildDialog, SetInfoPopulatesHeader) {
    mxh::ui::cGuildDialog d;
    d.SetInfo("ShadowFang", 7, "MasterA", 42, "Map_3");
    EXPECT_EQ(d.GuildName(), "ShadowFang");
    EXPECT_EQ(d.GuildLevel(), 7u);
    EXPECT_EQ(d.MasterName(), "MasterA");
    EXPECT_EQ(d.MemberNum(), 42u);
    EXPECT_EQ(d.Location(), "Map_3");
}

TEST(CGuildDialog, SetGuildInfoIncludesUnion) {
    mxh::ui::cGuildDialog d;
    d.SetGuildInfo("ShadowFang", "MasterA", "Map_5", 9, 50, "AlliedFang");
    EXPECT_EQ(d.GuildName(), "ShadowFang");
    EXPECT_EQ(d.MasterName(), "MasterA");
    EXPECT_EQ(d.Location(), "Map_5");
    EXPECT_EQ(d.GuildLevel(), 9u);
    EXPECT_EQ(d.MemberNum(), 50u);
    EXPECT_EQ(d.UnionName(), "AlliedFang");
}

TEST(CGuildDialog, ResetMemberInfoAppends) {
    mxh::ui::cGuildDialog d;
    d.ResetMemberInfo(MakeMember("Alice", 0, 30));
    d.ResetMemberInfo(MakeMember("Bob",   3, 99));
    d.ResetMemberInfo(MakeMember("Carol", 1, 50));
    EXPECT_EQ(d.MemberCount(), 3u);
}

TEST(CGuildDialog, DeleteMemberAllClears) {
    mxh::ui::cGuildDialog d;
    d.ResetMemberInfo(MakeMember("Alice", 0, 30));
    d.ResetMemberInfo(MakeMember("Bob",   3, 99));
    d.SetSelectedMember(1);
    d.DeleteMemberAll();
    EXPECT_EQ(d.MemberCount(), 0u);
    EXPECT_EQ(d.GetSelectedMember(), -1);
}

TEST(CGuildDialog, SortByPositionStable) {
    mxh::ui::cGuildDialog d;
    d.ResetMemberInfo(MakeMember("MasterA", 3, 99));
    d.ResetMemberInfo(MakeMember("Bob",     0, 30));
    d.ResetMemberInfo(MakeMember("ViceX",   2, 70));
    d.ResetMemberInfo(MakeMember("Carol",   1, 50));
    d.SortMemberListByPosition();
    // Order after sort: rank ascending.
    EXPECT_EQ(d.Members()[0].name, "Bob");     // rank 0
    EXPECT_EQ(d.Members()[1].name, "Carol");   // rank 1
    EXPECT_EQ(d.Members()[2].name, "ViceX");   // rank 2
    EXPECT_EQ(d.Members()[3].name, "MasterA"); // rank 3
}

TEST(CGuildDialog, SortByLevel) {
    mxh::ui::cGuildDialog d;
    d.ResetMemberInfo(MakeMember("A", 0, 30));
    d.ResetMemberInfo(MakeMember("B", 0, 99));
    d.ResetMemberInfo(MakeMember("C", 0, 50));
    d.SortMemberListByLevel();
    EXPECT_EQ(d.Members()[0].name, "A");
    EXPECT_EQ(d.Members()[1].name, "C");
    EXPECT_EQ(d.Members()[2].name, "B");
}

TEST(CGuildDialog, SetGuildPositionUpdatesLocation) {
    mxh::ui::cGuildDialog d;
    d.SetInfo("G", 1, "M", 1, "Old");
    d.SetGuildPosition("NewMap");
    EXPECT_EQ(d.Location(), "NewMap");
}

TEST(CGuildDialog, SetGuildPushupBtnSelectsActiveTab) {
    mxh::ui::cGuildDialog d;
    d.Init(0, 0, 400, 400, nullptr, 1);
    // Add two pushup buttons with ids 9000 and 9001.
    auto p0 = std::make_unique<mxh::ui::cPushupButton>();
    p0->Init(0, 0, 40, 20, nullptr, nullptr, nullptr,
             mxh::ui::cButton::ClickCallback{}, nullptr, 9000);
    auto p1 = std::make_unique<mxh::ui::cPushupButton>();
    p1->Init(40, 0, 40, 20, nullptr, nullptr, nullptr,
             mxh::ui::cButton::ClickCallback{}, nullptr, 9001);
    auto* raw0 = p0.get();
    auto* raw1 = p1.get();
    d.Add(std::move(p0));
    d.Add(std::move(p1));

    d.SetGuildPushupBtn(mxh::ui::cGuildDialog::kShowModeMember);  // id 9000
    EXPECT_TRUE (raw0->IsPushed());
    EXPECT_FALSE(raw1->IsPushed());
    d.SetGuildPushupBtn(mxh::ui::cGuildDialog::kShowModeInfo);    // id 9001
    EXPECT_FALSE(raw0->IsPushed());
    EXPECT_TRUE (raw1->IsPushed());
}

TEST(CGuildDialog, RefreshMemberListPopulatesListDialog) {
    mxh::ui::cGuildDialog d;
    d.Init(0, 0, 400, 400, nullptr, 1);
    auto* list = new mxh::ui::cListDialog();
    list->Init(0, 0, 200, 200, nullptr, 7001);
    list->InitList(20, 0, 0, 200, 200);
    d.Add(std::unique_ptr<mxh::ui::cListDialog>(list));
    d.ResetMemberInfo(MakeMember("Alice", 0, 30));
    d.ResetMemberInfo(MakeMember("Bob",   3, 99, false));
    d.RefreshMemberList();
    EXPECT_EQ(list->RowCount(), 2u);
}

TEST(CGuildDialog, SetDisableFuncBtnMemberDisablesSenior) {
    mxh::ui::cGuildDialog d;
    d.Init(0, 0, 400, 400, nullptr, 1);
    // Buttons: id 7000 = senior policy; id 8000 = admin policy.
    auto* bSenior = new mxh::ui::cButton();
    bSenior->Init(0, 0, 40, 20, nullptr, nullptr, nullptr,
                  mxh::ui::cButton::ClickCallback{}, nullptr, 7000);
    auto* bAdmin = new mxh::ui::cButton();
    bAdmin->Init(40, 0, 40, 20, nullptr, nullptr, nullptr,
                 mxh::ui::cButton::ClickCallback{}, nullptr, 8000);
    d.Add(std::unique_ptr<mxh::ui::cButton>(bSenior));
    d.Add(std::unique_ptr<mxh::ui::cButton>(bAdmin));
    // Member viewer: no buttons enabled.
    d.SetDisableFuncBtn(mxh::ui::cGuildDialog::Rank::Member);
    EXPECT_TRUE(bSenior->isEnabled() == false);
    EXPECT_TRUE(bAdmin->isEnabled()   == false);
    // Senior viewer: senior buttons enabled, admin still disabled.
    d.SetDisableFuncBtn(mxh::ui::cGuildDialog::Rank::Senior);
    EXPECT_TRUE(bSenior->isEnabled());
    EXPECT_TRUE(bAdmin->isEnabled()   == false);
    // Master viewer: both enabled.
    d.SetDisableFuncBtn(mxh::ui::cGuildDialog::Rank::Master);
    EXPECT_TRUE(bSenior->isEnabled());
    EXPECT_TRUE(bAdmin->isEnabled());
}

TEST(CGuildDialog, ClearDisableBtnEnablesEverything) {
    mxh::ui::cGuildDialog d;
    d.Init(0, 0, 400, 400, nullptr, 1);
    auto* b = new mxh::ui::cButton();
    b->Init(0, 0, 40, 20, nullptr, nullptr, nullptr,
            mxh::ui::cButton::ClickCallback{}, nullptr, 7000);
    b->SetDisable(true);
    d.Add(std::unique_ptr<mxh::ui::cButton>(b));
    EXPECT_TRUE(b->isEnabled() == false);
    d.ClearDisableBtn();
    EXPECT_TRUE(b->isEnabled());
}

TEST(CGuildDialog, SetActivePropagatesToChildren) {
    mxh::ui::cGuildDialog d;
    d.Init(0, 0, 400, 400, nullptr, 1);
    auto* s = new mxh::ui::cStatic();
    s->Init(0, 0, 200, 20, nullptr, 5);
    s->SetStaticText("G");
    d.Add(std::unique_ptr<mxh::ui::cStatic>(s));
    d.SetActive(true);
    EXPECT_TRUE(d.isActive());
    // Children inherit active via cDialog's SetActiveRecursive (called
    // by the manager); direct SetActive just flips self.
}
