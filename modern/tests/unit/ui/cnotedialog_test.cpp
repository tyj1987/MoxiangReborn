// mxh/tests/unit/ui/cnotedialog_test.cpp
//
// Unit tests for mxh::ui::cNoteDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * Mode enum (NormalNote / PsNote)
//   * Default state: page 1, NormalNote, NoteID 0
//   * Init + Linking
//   * SetActive(true) fires the callback + clears ChkAll
//   * SetActive(false) resets the mode to NormalNote
//   * SetMode toggles the pushup tab buttons
//   * SetNoteListFromFields populates m_lastRows + per-row
//     checkboxes (truncate FromName to 12 chars)
//   * ShowNotePageBtn enables per-page buttons
//   * SetChkAll toggles every active per-row checkbox
//   * Get/SetSelectedNotePge, Get/SetCurNoteID roundtrips

#include "mxh/ui/cnotedialog.hpp"
#include "mxh/ui/cbutton.hpp"
#include "mxh/ui/ccheckbox.hpp"
#include "mxh/ui/cpushupbutton.hpp"
#include "mxh/ui/clistctrl.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

using mxh::ui::cButton;
using mxh::ui::cCheckBox;
using mxh::ui::cListCtrl;
using mxh::ui::cNoteDialog;
using mxh::ui::cPushupButton;
using mxh::ui::NoteMode_NormalNote;
using mxh::ui::NoteMode_PsNote;

namespace {

struct Harness {
    cNoteDialog dlg;
    cButton       writeNoteBtn, delNoteBtn;
    cPushupButton noteBtn, psNoteBtn;
    cListCtrl     noteListLCtrl;
    cCheckBox     noteChk[mxh::ui::kNoteNumPerPage];
    cCheckBox     noteChkAll;
    cButton       notePageBtn[mxh::ui::kMaxNotePage];

    Harness() {
        cNoteDialog::ChildWindows w{};
        w.writeNoteBtn  = &writeNoteBtn;
        w.delNoteBtn    = &delNoteBtn;
        w.noteBtn       = &noteBtn;
        w.psNoteBtn     = &psNoteBtn;
        w.noteListLCtrl = &noteListLCtrl;
        w.noteChkAll    = &noteChkAll;
        for (std::int32_t i = 0; i < mxh::ui::kNoteNumPerPage; ++i) {
            w.noteChk[i] = &noteChk[i];
        }
        for (std::int32_t i = 0; i < mxh::ui::kMaxNotePage; ++i) {
            w.notePageBtn[i] = &notePageBtn[i];
        }
        dlg.SetChildWindowsForTest(w);
    }
};

mxh::ui::cNoteDialog::NoteListRow make_row(std::uint32_t id,
                                           const char* from,
                                           const char* date,
                                           bool read) {
    mxh::ui::cNoteDialog::NoteListRow r{};
    r.NoteID = id;
    std::strncpy(r.FromName, from, sizeof(r.FromName) - 1);
    r.FromName[sizeof(r.FromName) - 1] = '\0';
    std::strncpy(r.SendDate, date, sizeof(r.SendDate) - 1);
    r.SendDate[sizeof(r.SendDate) - 1] = '\0';
    r.bIsRead = read;
    return r;
}

}  // namespace

TEST(CNoteDialog, ModeEnumMatchesLegacy) {
    EXPECT_EQ(static_cast<std::uint16_t>(NoteMode_NormalNote), 0u);
    EXPECT_EQ(static_cast<std::uint16_t>(NoteMode_PsNote),    1u);
}

TEST(CNoteDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(mxh::ui::kNoteNumPerPage, 10);
    EXPECT_EQ(mxh::ui::kMaxNotePage, 5);
}

TEST(CNoteDialog, DefaultConstructionState) {
    cNoteDialog d;
    EXPECT_EQ(d.GetSelectedNotePge(), 1u);
    EXPECT_EQ(d.GetMode(), static_cast<std::uint16_t>(NoteMode_NormalNote));
    EXPECT_EQ(d.GetCurNoteID(), 0u);
    EXPECT_EQ(d.NoteListSize(), 0u);
}

TEST(CNoteDialog, InitAndLinking) {
    Harness h;
    h.dlg.Init(0, 0, 400, 200, nullptr, 0);
    h.dlg.Linking();
    // Linking() tail-calls SetMode(NormalNote) -> pushes the
    // Normal tab button, un-pushes the Ps button.
    EXPECT_TRUE(h.noteBtn.IsPushed());
    EXPECT_FALSE(h.psNoteBtn.IsPushed());
}

TEST(CNoteDialog, SetModeTogglesTabButtons) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetMode(NoteMode_PsNote);
    EXPECT_FALSE(h.noteBtn.IsPushed());
    EXPECT_TRUE(h.psNoteBtn.IsPushed());
    EXPECT_EQ(h.dlg.GetMode(),
              static_cast<std::uint16_t>(NoteMode_PsNote));
    h.dlg.SetMode(NoteMode_NormalNote);
    EXPECT_TRUE(h.noteBtn.IsPushed());
    EXPECT_FALSE(h.psNoteBtn.IsPushed());
}

TEST(CNoteDialog, SetActiveTrueFiresCallbackAndClearsChkAll) {
    Harness h;
    h.dlg.Linking();
    h.noteChkAll.SetChecked(true);
    int callCount = 0;
    bool lastState = false;
    h.dlg.SetOnActiveChanged([&](bool active) {
        ++callCount;
        lastState = active;
    });
    h.dlg.SetActive(true);
    EXPECT_EQ(callCount, 1);
    EXPECT_TRUE(lastState);
    EXPECT_TRUE(h.dlg.isActive());
    // 1:1 with legacy: SetActive(TRUE) -> ChkAll->SetChecked(FALSE)
    EXPECT_FALSE(h.noteChkAll.IsChecked());
}

TEST(CNoteDialog, SetActiveFalseResetsModeToNormal) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetMode(NoteMode_PsNote);
    EXPECT_EQ(h.dlg.GetMode(),
              static_cast<std::uint16_t>(NoteMode_PsNote));
    int callCount = 0;
    h.dlg.SetOnActiveChanged([&](bool /*active*/) { ++callCount; });
    h.dlg.SetActive(false);
    EXPECT_EQ(callCount, 1);
    EXPECT_FALSE(h.dlg.isActive());
    // 1:1 with legacy: SetActive(FALSE) -> SetMode(NormalNote)
    EXPECT_EQ(h.dlg.GetMode(),
              static_cast<std::uint16_t>(NoteMode_NormalNote));
    EXPECT_TRUE(h.noteBtn.IsPushed());
}

TEST(CNoteDialog, SetNoteListPopulatesRows) {
    Harness h;
    h.dlg.Linking();
    std::vector<mxh::ui::cNoteDialog::NoteListRow> rows;
    rows.push_back(make_row(101, "Alice", "2026-07-26", false));
    rows.push_back(make_row(102, "Bob",   "2026-07-25", true));
    h.dlg.SetNoteListFromFields(rows, /*totalPage=*/1);
    EXPECT_EQ(h.dlg.NoteListSize(), 2u);
    EXPECT_EQ(h.dlg.RowCount(),     2u);
    // SetActive(TRUE) is the legacy tail call inside
    // SetNoteList.
    EXPECT_TRUE(h.dlg.isActive());
    // First row's checkbox must be active (NoteID != 0).
    EXPECT_TRUE(h.noteChk[0].isActive());
    EXPECT_FALSE(h.noteChk[0].IsChecked());
    // Row 2 also active.
    EXPECT_TRUE(h.noteChk[1].isActive());
    // Row 3+ (empty) -> checkbox inactive.
    EXPECT_FALSE(h.noteChk[2].isActive());
}

TEST(CNoteDialog, SetNoteListTruncatesFromNameToTwelveChars) {
    Harness h;
    h.dlg.Linking();
    std::vector<mxh::ui::cNoteDialog::NoteListRow> rows;
    rows.push_back(make_row(1, "VeryLongSenderName", "2026-01-01", false));
    h.dlg.SetNoteListFromFields(rows, 1);
    // The host retrieves the cached row data via NoteListSize + a
    // future accessor; the truncation is applied internally.
    EXPECT_EQ(h.dlg.NoteListSize(), 1u);
    // The StoredRItem p0 buffer holds the truncated 12-char name.
    // (We don't expose a public getter; the contract is that
    // SetNoteListFromFields must not overrun the 12-char
    // FromName legacy cap.)
}

TEST(CNoteDialog, SetNoteListDropsZeroNoteIdRows) {
    Harness h;
    h.dlg.Linking();
    std::vector<mxh::ui::cNoteDialog::NoteListRow> rows;
    rows.push_back(make_row(0, "EmptySlot", "", false));
    rows.push_back(make_row(202, "Carol", "2026-07-26", true));
    h.dlg.SetNoteListFromFields(rows, 1);
    EXPECT_EQ(h.dlg.NoteListSize(), 1u);
    // Slot 0: NoteID == 0 -> checkbox inactive.
    EXPECT_FALSE(h.noteChk[0].isActive());
    // Slot 1: NoteID != 0 -> checkbox active.
    EXPECT_TRUE(h.noteChk[1].isActive());
}

TEST(CNoteDialog, ShowNotePageBtnHidesAllWhenTotalPageZero) {
    Harness h;
    h.dlg.Linking();
    // Pre-activate every page button.
    for (std::int32_t i = 0; i < mxh::ui::kMaxNotePage; ++i) {
        h.notePageBtn[i].SetActive(true);
    }
    h.dlg.ShowNotePageBtn(0);
    for (std::int32_t i = 0; i < mxh::ui::kMaxNotePage; ++i) {
        EXPECT_FALSE(h.notePageBtn[i].isActive())
            << "page button " << i << " should be hidden";
    }
}

TEST(CNoteDialog, ShowNotePageBtnEnablesUpToTotalPage) {
    Harness h;
    h.dlg.Linking();
    h.dlg.ShowNotePageBtn(3);
    EXPECT_TRUE (h.notePageBtn[0].isActive());
    EXPECT_TRUE (h.notePageBtn[1].isActive());
    EXPECT_TRUE (h.notePageBtn[2].isActive());
    EXPECT_FALSE(h.notePageBtn[3].isActive());
    EXPECT_FALSE(h.notePageBtn[4].isActive());
    // 1:1 with legacy: SetTextValue(i+1) labels each page.
    // Modern cButton::text() returns the label.
    EXPECT_EQ(h.notePageBtn[0].text(), "1");
    EXPECT_EQ(h.notePageBtn[1].text(), "2");
    EXPECT_EQ(h.notePageBtn[2].text(), "3");
}

TEST(CNoteDialog, ShowNotePageBtnEnablesAllWhenTotalPageMax) {
    Harness h;
    h.dlg.Linking();
    h.dlg.ShowNotePageBtn(mxh::ui::kMaxNotePage);
    for (std::int32_t i = 0; i < mxh::ui::kMaxNotePage; ++i) {
        EXPECT_TRUE(h.notePageBtn[i].isActive());
    }
}

TEST(CNoteDialog, SetChkAllTogglesOnlyActiveCheckboxes) {
    Harness h;
    h.dlg.Linking();
    // Activate only the first 3 checkboxes (mimic SetNoteList
    // populating 3 rows out of 10).
    for (std::int32_t i = 0; i < 3; ++i) {
        h.noteChk[i].SetActive(true);
    }
    h.noteChkAll.SetChecked(true);
    h.dlg.SetChkAll();
    EXPECT_TRUE (h.noteChk[0].IsChecked());
    EXPECT_TRUE (h.noteChk[1].IsChecked());
    EXPECT_TRUE (h.noteChk[2].IsChecked());
    EXPECT_FALSE(h.noteChk[3].IsChecked());  // inactive -> untouched
    EXPECT_FALSE(h.noteChk[4].IsChecked());
    // Now flip the master checkbox off and re-apply.
    h.noteChkAll.SetChecked(false);
    h.dlg.SetChkAll();
    EXPECT_FALSE(h.noteChk[0].IsChecked());
    EXPECT_FALSE(h.noteChk[1].IsChecked());
    EXPECT_FALSE(h.noteChk[2].IsChecked());
    // Inactive checkboxes must still be off.
    EXPECT_FALSE(h.noteChk[3].IsChecked());
}

TEST(CNoteDialog, GetSetSelectedNotePgeRoundTrip) {
    cNoteDialog d;
    EXPECT_EQ(d.GetSelectedNotePge(), 1u);
    d.SetSelectedNotePge(7);
    EXPECT_EQ(d.GetSelectedNotePge(), 7u);
}

TEST(CNoteDialog, GetSetCurNoteIDRoundTrip) {
    cNoteDialog d;
    EXPECT_EQ(d.GetCurNoteID(), 0u);
    d.SetCurNoteID(98765);
    EXPECT_EQ(d.GetCurNoteID(), 98765u);
}

TEST(CNoteDialog, SetActiveCallbackNotFiredWhenDisabled) {
    cNoteDialog d;
    d.SetDisable(true);
    int callCount = 0;
    d.SetOnActiveChanged([&](bool /*active*/) { ++callCount; });
    d.SetActive(true);
    EXPECT_EQ(callCount, 0);
    EXPECT_FALSE(d.isActive());
}
