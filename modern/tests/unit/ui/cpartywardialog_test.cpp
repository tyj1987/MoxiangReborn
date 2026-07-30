//
// Unit tests for mxh::ui::cPartyWarDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//  * Constants: kMemberCount=14, kTeamSize=7
//  * Default construction state: locks false
//  * Init + Linking
//  * ShowPartyWarDlg bMaster=0/1, button visibility, clears 14 checks
//  * HidePartyWarDlg: deactivate + reset locks
//  * NoChangeCheckBoxState toggle + returns previous state
//  * SetPartyMemberName sets text + updates team when nIndex==0 or 7
//  * Add/RemovePartyWarMember check/uncheck
//  * SetLock/SetUnLock button visibility state machine
//  * SetTime formats mm:ss
//  * IsStartButtonActive / IsCancelButtonActive reflect child-window state
//

#include "mxh/ui/cpartywardialog.hpp"
#include "mxh/ui/cbutton.hpp"
#include "mxh/ui/ccheckbox.hpp"
#include "mxh/ui/cstatic.hpp"
#include "mxh/ui/ctextarea.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

using mxh::ui::cButton;
using mxh::ui::cCheckBox;
using mxh::ui::cPartyWarDialog;
using mxh::ui::cStatic;
using mxh::ui::cTextArea;

namespace {

struct Harness {
    cPartyWarDialog dlg;
    cCheckBox    check[cPartyWarDialog::kMemberCount];
    cStatic      stat[cPartyWarDialog::kMemberCount];
    cButton     btnLock, btnUnLock, btnStart, btnCancel;
    cTextArea   title;
    cTextArea   team1;
    cTextArea   team2;
    cTextArea   time;

    Harness() {
        cPartyWarDialog::ChildWindows w;
        for (std::size_t i = 0; i < cPartyWarDialog::kMemberCount; ++i) {
            w.memberCheck[i] = &check[i];
            w.memberStatic[i] = &stat[i];
        }
        w.btnLock = &btnLock;
        w.btnUnLock = &btnUnLock;
        w.btnStart = &btnStart;
        w.btnCancel = &btnCancel;
        w.title = &title;
        w.team1 = &team1;
        w.team2 = &team2;
        w.time = &time;
        dlg.SetChildWindowsForTest(w);
    }
};

} // namespace


TEST(CPartyWarDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(cPartyWarDialog::kMemberCount, 14u);
    EXPECT_EQ(cPartyWarDialog::kTeamSize, 7u);
}

TEST(CPartyWarDialog, DefaultConstructionState) {
    Harness h;
    EXPECT_FALSE(h.dlg.IsPartyLocked());
    EXPECT_FALSE(h.dlg.IsEnemyLocked());
}

TEST(CPartyWarDialog, InitStoresPosition) {
    Harness h;
    int img = 7;
    h.dlg.Init(200, 150, 320, 240, &img, 42);
    EXPECT_EQ(h.dlg.absX(), 200);
    EXPECT_EQ(h.dlg.absY(), 150);
}

TEST(CPartyWarDialog, LinkingAcceptsNull) {
    Harness h;
    h.dlg.Linking();
}


TEST(CPartyWarDialog, ShowPartyWarDlgMasterEnablesButtonsAndClearsChecks) {
    Harness h;
    h.dlg.ShowPartyWarDlg(true);
    EXPECT_TRUE(h.btnLock.isActive());
    EXPECT_TRUE(h.btnUnLock.isActive());
    EXPECT_FALSE(h.btnStart.isActive());
    EXPECT_TRUE(h.btnCancel.isActive());
    EXPECT_FALSE(h.dlg.IsStartButtonActive());
    EXPECT_TRUE(h.dlg.IsCancelButtonActive());
    for (std::size_t i = 0; i < cPartyWarDialog::kMemberCount; ++i) {
        EXPECT_FALSE(h.check[i].isActive());
        EXPECT_FALSE(h.stat[i].isActive());
    }
}

TEST(CPartyWarDialog, ShowPartyWarDlgNonMasterDisablesMasterButtons) {
    Harness h;
    h.dlg.ShowPartyWarDlg(false);
    EXPECT_FALSE(h.btnLock.isActive());
    EXPECT_FALSE(h.btnUnLock.isActive());
    EXPECT_FALSE(h.btnCancel.isActive());
}


TEST(CPartyWarDialog, HidePartyWarDlgResetsLocks) {
    Harness h;
    h.dlg.SetLock(true, true);
    EXPECT_TRUE(h.dlg.IsPartyLocked());
    EXPECT_FALSE(h.dlg.IsEnemyLocked());
    h.dlg.HidePartyWarDlg();
    EXPECT_FALSE(h.dlg.IsPartyLocked());
    EXPECT_FALSE(h.dlg.IsEnemyLocked());
}

TEST(CPartyWarDialog, NoChangeCheckBoxStateTogglesReturnsPrevious) {
    Harness h;
    h.check[0].SetChecked(false);
    EXPECT_FALSE(h.dlg.NoChangeCheckBoxState(0)); // returns previous = false
    h.check[0].SetChecked(true);
    EXPECT_TRUE(h.dlg.NoChangeCheckBoxState(0)); // returns previous = true
}

TEST(CPartyWarDialog, NoChangeCheckBoxStateIgnoresInvalidIndex) {
    Harness h;
    EXPECT_FALSE(h.dlg.NoChangeCheckBoxState(-1));
    EXPECT_FALSE(h.dlg.NoChangeCheckBoxState(15));
}


TEST(CPartyWarDialog, SetPartyMemberNameSetsTextAndActivatesCheck) {
    Harness h;
    h.dlg.SetPartyMemberName("Alice", 3, true);
    EXPECT_EQ(h.stat[3].GetStaticText(), "Alice");
    EXPECT_TRUE(h.check[3].isActive());
    EXPECT_FALSE(h.check[3].IsChecked());
}

TEST(CPartyWarDialog, SetPartyMemberNameUpdatesTeam1) {
    Harness h;
    h.dlg.SetPartyMemberName("Alice", 0, true);
    EXPECT_EQ(h.team1.GetScriptText(), "Alice");
    EXPECT_EQ(h.title.GetScriptText(), "Alice VS "); // title has no team2 yet
}

TEST(CPartyWarDialog, SetPartyMemberNameUpdatesTeam2AndTitle) {
    Harness h;
    h.dlg.SetPartyMemberName("Alice", 0, true);
    h.dlg.SetPartyMemberName("Alice", 7, true);
    EXPECT_EQ(h.team2.GetScriptText(), "Alice");
    h.dlg.SetPartyMemberName("Bob", 7, true);
    EXPECT_EQ(h.team2.GetScriptText(), "Bob");
    EXPECT_EQ(h.title.GetScriptText(), "Alice VS Bob");
}

TEST(CPartyWarDialog, SetPartyMemberNameIgnoresOtherIndex) {
    Harness h;
    h.dlg.SetPartyMemberName("X", 5, true);
    EXPECT_EQ(h.stat[5].GetStaticText(), "X");
    EXPECT_EQ(h.team1.GetScriptText(), "");
    EXPECT_EQ(h.team2.GetScriptText(), "");
}


TEST(CPartyWarDialog, AddPartyWarMemberChecks) {
    Harness h;
    h.dlg.AddPartyWarMember(5);
    EXPECT_TRUE(h.check[5].IsChecked());
    h.dlg.AddPartyWarMember(-1);
    EXPECT_FALSE(h.check[0].IsChecked());
    h.dlg.AddPartyWarMember(15);
    EXPECT_FALSE(h.check[0].IsChecked());
}

TEST(CPartyWarDialog, RemovePartyWarMemberUnchecks) {
    Harness h;
    h.check[2].SetChecked(true);
    h.dlg.RemovePartyWarMember(2);
    EXPECT_FALSE(h.check[2].IsChecked());
    h.dlg.RemovePartyWarMember(-1);
    h.dlg.RemovePartyWarMember(15);
    EXPECT_FALSE(h.check[0].IsChecked());
}


TEST(CPartyWarDialog, SetLockPartyOnlyMasterEnablesCancel) {
    Harness h;
    h.dlg.SetLock(true, true);
    EXPECT_FALSE(h.dlg.IsStartButtonActive());
    EXPECT_TRUE(h.dlg.IsCancelButtonActive());
    EXPECT_TRUE(h.dlg.IsPartyLocked());
}

TEST(CPartyWarDialog, SetLockEnemyOnlyMasterEnablesCancel) {
    Harness h;
    h.dlg.SetLock(false, true);
    EXPECT_FALSE(h.dlg.IsStartButtonActive());
    EXPECT_TRUE(h.dlg.IsCancelButtonActive());
    EXPECT_TRUE(h.dlg.IsEnemyLocked());
}

TEST(CPartyWarDialog, SetLockBothTeamsLockedMasterEnablesStart) {
    Harness h;
    h.dlg.SetLock(true, true);
    h.dlg.SetLock(false, true);
    EXPECT_TRUE(h.dlg.IsStartButtonActive());
    EXPECT_FALSE(h.dlg.IsCancelButtonActive());
    EXPECT_TRUE(h.dlg.IsPartyLocked());
    EXPECT_TRUE(h.dlg.IsEnemyLocked());
}

TEST(CPartyWarDialog, SetUnLockMasterEnablesCancel) {
    Harness h;
    h.dlg.SetUnLock(true);
    EXPECT_FALSE(h.dlg.IsStartButtonActive());
    EXPECT_TRUE(h.dlg.IsCancelButtonActive());
    EXPECT_FALSE(h.dlg.IsPartyLocked());
}

TEST(CPartyWarDialog, SetTimeFormatsMMSS) {
    Harness h;
    h.dlg.SetTime(0);
    EXPECT_EQ(h.time.GetScriptText(), "0:00");
    h.dlg.SetTime(123);
    EXPECT_EQ(h.time.GetScriptText(), "2:03");
    h.dlg.SetTime(3600);
    EXPECT_EQ(h.time.GetScriptText(), "60:00"); // 1h = 1, 00:00 = 00:00
}

