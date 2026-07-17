// mpregistdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cMPRegistDialog (MP practice
// registration dialog: 2 cTextArea + 1 cStatic + 1 cIconDialog
// children).
//
// Covers modern/src/ui/mpregistdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\MPRegistDialog.h (1098 B) and
//   墨香【源码】\[Client]MH\MPRegistDialog.cpp (3860 B).
//
// What's tested:
//   - Default construction: cMPRegistDialog is a
//     cIconDialog (4 children fields are 1:1 with
//     legacy CMPRegistDialog member layout).
//   - Local id range is 562-568 (1:1 with legacy
//     WindowIDs.h MP_REGISTDLG, MP_RMUGONGICON,
//     MP_RMUGONGINFO, MP_RPRACTICEINFO, MP_RFEE,
//     MP_ROKBTN, MP_RCANCELBTN).
//   - Linking without a resource loader: 4 children
//     fields are null (modern has no resource loader
//     hook — Linking is a 1:1 signature match but
//     findWindowById returns null for missing
//     children, like the legacy's GetWindowForID on
//     a dialog that wasn't built from a .bin).
//   - Linking with manual child insertion: when 4
//     children are Inserted via InsertChild(...)
//     into the dialog tree at the matching ids,
//     Linking resolves them. Note: 1:1 quirk — the
//     modern cIconDialog child (m_pMugongIconDlg)
//     gets a 1-cell layout configured (modern has
//     no resource loader to set it up; legacy relies
//     on the .bin's <Child id=MP_RMUGONGICON
//     width=... height=...> to size the cell).
//   - SetActive override (1:1 with legacy):
//     * On val == TRUE: forwards to cDialog::SetActive
//       (no extra reset).
//     * On val == FALSE: resets m_MugongInfo (CHATMGR
//       msg 662 placeholder "MP_REGIST_CLEAR"),
//       m_pMugongIconDlg (DeleteIcon(0, &pIcon)),
//       m_PracticeInfo (empty), m_Fee (SetStaticValue(0)),
//       then calls cDialog::SetActive(val).
//   - FakeMoveIcon: returns false unconditionally
//     (5-singleton TODO + cItem / cSkillInfo not
//     ported). When the 5 singletons + cMugongBase +
//     cSkillInfo get their modern equivalents, this
//     should be replaced with the real body from
//     the legacy.
//   - SetSuryunMugongInfo: 1:1 with legacy sprintf
//     format "Mugong: %s (Sung %u)" (placeholder
//     format until CHATMGR->GetChatMsg(661) is
//     ported). Null mugongName is rendered as
//     "(null)" (1:1 quirk: legacy would crash on
//     null, modern is defensive).
//   - SetPracticeInfo: 1:1 with legacy sprintf
//     format "Practice: Sung %u, %u min, Kind %d,
//     Num %d". LTime = limitime / 60000 (ms → min,
//     1:1 with legacy). SetStaticValue(fee) on m_Fee.
//   - AddLink: 1:1 with legacy — DeleteIcon(0) if
//     not addable, then AddIcon(0, picon, TRUE) (the
//     legacy bOnlyLink=TRUE path). cIconDialog::IsAddable
//     returns true on a fresh cell, so the first
//     AddLink call skips the DeleteIcon(0) branch.
//   - GetMugong: returns nullptr unconditionally
//     (TODO until CMugongBase port — R-12.x deferred).
//
// 1:1 quirks preserved:
//   - Ctor body is empty (1:1 with legacy
//     CMPRegistDialog ctor; legacy m_type =
//     WT_MPREGISTDIALOG drop, modern cWindow doesn't
//     have m_type).
//   - SetDragOverIconType(WT_MUGONG) call in legacy
//     Linking is a 1:1 quirk drop (modern cIconDialog
//     has no such API; cPetWearedExDialog and
//     cWearedExDialog also drop this at port time).
//   - The 1-cell layout configured in modern Linking
//     is a 1:1 stand-in for the legacy's .bin-defined
//     cell (modern has no .bin loader).
//   - SetActive on val == TRUE only forwards to base
//     (legacy CMPRegistDialog::SetActive has the same
//     `if(val==FALSE)` guard before the reset).
//   - SetSuryunMugongInfo's null mugongName is
//     rendered as "(null)" (defensive; legacy would
//     have crashed on snprintf("%s", NULL)).
//   - AddLink's DeleteIcon(0, &pIcon) is replaced by
//     DeleteIcon(0) (outIcon=nullptr default) — same
//     1:1 behavior because the legacy doesn't read
//     *outIcon afterwards either; the variable is
//     declared but unused after the call.
//   - GetMugong returns nullptr (TODO until
//     CMugongBase port — same constraint as
//     cPetWearedExDialog::CheckDuplication and
//     cWearedExDialog's Titan-vs-normal branch).

#include "mpregistdialog.hpp"
#include "ctextarea.hpp"
#include "cstatic.hpp"
#include "cIconDialog.hpp"
#include "cWindow.hpp"
#include "cDialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction + constants
// ===========================================================================

TEST(CMPRegistDialogTest, DefaultConstructionIsValid) {
    cMPRegistDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy also has
    // empty CMPRegistDialog() ctor; the m_type =
    // WT_MPREGISTDIALOG assignment is dropped since
    // modern cWindow doesn't have m_type).
    SUCCEED();
}

TEST(CMPRegistDialogTest, InheritsIconDialogCellLayout) {
    // 1:1 with legacy CMPRegistDialog : public cIconDialog.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    // Legacy has 1 cIconDialog child (MP_RMUGONGICON),
    // which holds 1 cell (the practice slot). Modern
    // port configures 1 cell on Linking.
    dlg.SetCellNum(1);
    EXPECT_EQ(dlg.GetCellNum(), 1u);
}

TEST(CMPRegistDialogTest, LocalIdConstantsMatchExpectedRange) {
    // 1:1 with legacy WindowIDs.h MP_REGISTDLG (562),
    // MP_RMUGONGICON (563), MP_RMUGONGINFO (564),
    // MP_RPRACTICEINFO (565), MP_RFEE (566),
    // MP_ROKBTN (567), MP_RCANCELBTN (568). The
    // reverse mapping from mpmissiondialog's
    // kIdMission=570 puts the MP_REGIST series at
    // 562-568.
    EXPECT_EQ(cMPRegistDialog::kRegistDlgId,    562);
    EXPECT_EQ(cMPRegistDialog::kMugongIconId,   563);
    EXPECT_EQ(cMPRegistDialog::kMugongInfoId,   564);
    EXPECT_EQ(cMPRegistDialog::kPracticeInfoId, 565);
    EXPECT_EQ(cMPRegistDialog::kFeeId,          566);
    EXPECT_EQ(cMPRegistDialog::kOkBtnId,        567);
    EXPECT_EQ(cMPRegistDialog::kCancelBtnId,    568);
}

TEST(CMPRegistDialogTest, ChatMsgIdsMatchLegacy) {
    // 1:1 with legacy CHATMGR->GetChatMsg(660) +
    // (661) + (662) — the actual msg strings are
    // localized; the modern port uses placeholder
    // text until CHATMGR is ported.
    EXPECT_EQ(cMPRegistDialog::kSuryunMugongInfoChatMsgId, 661);
    EXPECT_EQ(cMPRegistDialog::kPracticeInfoChatMsgId,     660);
    EXPECT_EQ(cMPRegistDialog::kClearInfoChatMsgId,        662);
}

// ===========================================================================
// Linking (1:1 with legacy CMPRegistDialog::Linking)
// ===========================================================================

namespace {

// Helper: insert a child window at a given id so
// findWindowById can resolve it. cWindow has no
// SetID method — the widget id is set at Init()
// time and is read-only after that. The helper
// calls Init() with the target id, then Add()s
// the child to the parent dialog. This is the same
// pattern used by mpnoticedialog_test / mpmissiondialog_test
// (see those test files for the matching idiom).
template <typename T>
void InsertChildById(cDialog* parent, std::int32_t id, std::unique_ptr<T> child) {
    child->Init(0, 0, 0, 0, nullptr, id);
    parent->Add(std::move(child));
}

}  // namespace

TEST(CMPRegistDialogTest, LinkingWithoutChildrenLeavesAllFieldsNull) {
    // 1:1 quirk: the modern port has no resource
    // loader hook (the legacy's <Child id=MP_RFEE
    // ...> .bin directives). When the dialog is
    // constructed without manually inserting
    // children, findWindowById returns null for all
    // 4 ids, and Linking() leaves the 4 child
    // fields null. This is identical to the legacy's
    // GetWindowForID behavior on a dialog that
    // wasn't built from a .bin.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();

    EXPECT_EQ(dlg.GetMugongInfo(), nullptr);
    EXPECT_EQ(dlg.GetPracticeInfo(), nullptr);
    EXPECT_EQ(dlg.GetFee(), nullptr);
    EXPECT_EQ(dlg.GetMugongIconDlg(), nullptr);
}

TEST(CMPRegistDialogTest, LinkingResolvesAllFourChildren) {
    // When 4 children are inserted at the matching
    // ids, Linking resolves all of them. This is the
    // 1:1 happy-path test: build the dialog tree
    // the way the .bin loader would, then verify
    // Linking stores the right pointers in the
    // right member fields.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    // Insert 4 children at the 4 ids Linking will
    // resolve.
    auto mugongInfo    = std::make_unique<cTextArea>();
    auto practiceInfo  = std::make_unique<cTextArea>();
    auto fee           = std::make_unique<cStatic>();
    auto mugongIconDlg = std::make_unique<cIconDialog>();
    InsertChildById(&dlg, cMPRegistDialog::kMugongInfoId,   std::move(mugongInfo));
    InsertChildById(&dlg, cMPRegistDialog::kPracticeInfoId, std::move(practiceInfo));
    InsertChildById(&dlg, cMPRegistDialog::kFeeId,          std::move(fee));
    InsertChildById(&dlg, cMPRegistDialog::kMugongIconId,   std::move(mugongIconDlg));

    dlg.Linking();

    EXPECT_NE(dlg.GetMugongInfo(),   nullptr);
    EXPECT_NE(dlg.GetPracticeInfo(), nullptr);
    EXPECT_NE(dlg.GetFee(),          nullptr);
    EXPECT_NE(dlg.GetMugongIconDlg(), nullptr);
}

TEST(CMPRegistDialogTest, LinkingConfiguresIconCellWhenMissing) {
    // 1:1 quirk: modern cIconDialog has no resource
    // loader, so Linking configures a 1-cell layout
    // (0, 0, 0, 0) on the inner mugong icon dialog
    // when its GetCellNum() == 0. The legacy's
    // equivalent is the .bin's <Child id=MP_RMUGONGICON
    // width=W height=H> directive.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto mugongIconDlg = std::make_unique<cIconDialog>();
    InsertChildById(&dlg, cMPRegistDialog::kMugongIconId, std::move(mugongIconDlg));

    EXPECT_EQ(dlg.GetMugongIconDlg(), nullptr);  // pre-Linking
    dlg.Linking();
    ASSERT_NE(dlg.GetMugongIconDlg(), nullptr);
    EXPECT_EQ(dlg.GetMugongIconDlg()->GetCellNum(), 1u);
}

TEST(CMPRegistDialogTest, LinkingDoesNotOverwriteExistingIconCell) {
    // 1:1 quirk: when the inner cIconDialog already
    // has cells configured (e.g. from a prior
    // resource loader pass), Linking leaves them
    // alone. The "if (GetCellNum() == 0) ..." guard
    // is the modern port's stand-in for the legacy's
    // .bin directives never re-sizing an existing
    // cell layout.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto mugongIconDlg = std::make_unique<cIconDialog>();
    mugongIconDlg->SetCellNum(3);
    InsertChildById(&dlg, cMPRegistDialog::kMugongIconId, std::move(mugongIconDlg));

    dlg.Linking();
    ASSERT_NE(dlg.GetMugongIconDlg(), nullptr);
    EXPECT_EQ(dlg.GetMugongIconDlg()->GetCellNum(), 3u);
}

// ===========================================================================
// SetActive (1:1 with legacy CMPRegistDialog::SetActive)
// ===========================================================================

TEST(CMPRegistDialogTest, SetActiveTrueDoesNotResetChildren) {
    // 1:1 with legacy: the legacy's SetActive body
    // is `if(val==FALSE) { reset; } cDialog::SetActive(val);`
    // On val == TRUE, the body is a no-op except
    // for the base call.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto practiceInfo = std::make_unique<cTextArea>();
    cTextArea* piRaw = practiceInfo.get();
    InsertChildById(&dlg, cMPRegistDialog::kPracticeInfoId, std::move(practiceInfo));
    dlg.Linking();
    ASSERT_NE(dlg.GetPracticeInfo(), nullptr);

    // Pre-condition: practiceInfo has non-empty text.
    piRaw->SetScriptText("Practice: Sung 5, 60 min");
    EXPECT_FALSE(piRaw->GetScriptText().empty());

    dlg.SetActive(true);

    // Post-condition: practiceInfo text is unchanged.
    EXPECT_EQ(piRaw->GetScriptText(), "Practice: Sung 5, 60 min");
}

TEST(CMPRegistDialogTest, SetActiveFalseClearsPracticeInfo) {
    // 1:1 with legacy: on val == FALSE, the legacy
    // sets m_PracticeInfo->SetScriptText(""). The
    // modern port does the same.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto practiceInfo = std::make_unique<cTextArea>();
    cTextArea* piRaw = practiceInfo.get();
    InsertChildById(&dlg, cMPRegistDialog::kPracticeInfoId, std::move(practiceInfo));
    dlg.Linking();
    ASSERT_NE(dlg.GetPracticeInfo(), nullptr);

    piRaw->SetScriptText("Practice: Sung 5, 60 min");
    dlg.SetActive(false);
    EXPECT_EQ(piRaw->GetScriptText(), "");
}

TEST(CMPRegistDialogTest, SetActiveFalseSetsMugongInfoToClearPlaceholder) {
    // 1:1 with legacy CHATMGR->GetChatMsg(662)
    // (placeholder "MP_REGIST_CLEAR" in modern port
    // until CHATMGR is ported).
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto mugongInfo = std::make_unique<cTextArea>();
    cTextArea* miRaw = mugongInfo.get();
    InsertChildById(&dlg, cMPRegistDialog::kMugongInfoId, std::move(mugongInfo));
    dlg.Linking();
    ASSERT_NE(dlg.GetMugongInfo(), nullptr);

    miRaw->SetScriptText("Mugong: foo (Sung 3)");
    dlg.SetActive(false);
    EXPECT_EQ(miRaw->GetScriptText(), "MP_REGIST_CLEAR");
}

TEST(CMPRegistDialogTest, SetActiveFalseResetsFeeToZero) {
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto fee = std::make_unique<cStatic>();
    cStatic* feeRaw = fee.get();
    InsertChildById(&dlg, cMPRegistDialog::kFeeId, std::move(fee));
    dlg.Linking();
    ASSERT_NE(dlg.GetFee(), nullptr);

    feeRaw->SetStaticValue(12345);
    dlg.SetActive(false);
    EXPECT_EQ(feeRaw->GetStaticValue(), 0);
}

TEST(CMPRegistDialogTest, SetActiveFalseDeletesIcon) {
    // 1:1 with legacy: on val == FALSE, the legacy
    // calls m_pMugongIconDlg->DeleteIcon(0, &pIcon)
    // to drop any registered practice mugong.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto mugongIconDlg = std::make_unique<cIconDialog>();
    cIconDialog* midRaw = mugongIconDlg.get();
    InsertChildById(&dlg, cMPRegistDialog::kMugongIconId, std::move(mugongIconDlg));
    dlg.Linking();
    ASSERT_NE(dlg.GetMugongIconDlg(), nullptr);

    // Add an icon to cell 0.
    dlg.AddLink(reinterpret_cast<cIcon*>(0x1));
    // Cell 0 is now occupied.
    EXPECT_TRUE(midRaw->GetIconForIdx(0) != nullptr);

    dlg.SetActive(false);
    // Cell 0 should be empty now.
    EXPECT_EQ(midRaw->GetIconForIdx(0), nullptr);
}

TEST(CMPRegistDialogTest, SetActivePropagatesToBase) {
    // 1:1 with legacy: cDialog::SetActive(val) is
    // always called, regardless of val.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());

    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CMPRegistDialogTest, SetActiveFalseOnUnlinkedDialogIsSafe) {
    // 1:1 quirk: when no children are inserted,
    // Linking() leaves the 4 fields null. SetActive
    // (val == FALSE) must not crash on null fields.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // No children inserted — all 4 fields are null.
    dlg.SetActive(false);
    SUCCEED();  // did not crash
}

// ===========================================================================
// FakeMoveIcon (1:1 with legacy, 5-singleton TODO)
// ===========================================================================

TEST(CMPRegistDialogTest, FakeMoveIconReturnsFalse) {
    // 1:1 with legacy: the legacy body has 14 lines
    // of singleton dispatch (SURYUNMGR / HERO /
    // WINDOWMGR / CHATMGR / OBJECTSTATEMGR + CMugongBase
    // / cSkillInfo). None are ported. The modern
    // port returns false unconditionally (matching
    // the legacy's "invalid drop → return FALSE"
    // branch at the very end). When the 5 singletons
    // + cMugongBase + cSkillInfo are ported, the
    // real body should fill in.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();

    cIcon* icon = reinterpret_cast<cIcon*>(0x1);
    EXPECT_FALSE(dlg.FakeMoveIcon(100, 200, icon));
    EXPECT_FALSE(dlg.FakeMoveIcon(0, 0, nullptr));
    EXPECT_FALSE(dlg.FakeMoveIcon(100, 200, nullptr));
}

// ===========================================================================
// SetSuryunMugongInfo (1:1 with legacy sprintf)
// ===========================================================================

TEST(CMPRegistDialogTest, SetSuryunMugongInfoFormatsString) {
    // 1:1 with legacy: sprintf via placeholder
    // format "Mugong: %s (Sung %u)" + SetScriptText
    // on m_MugongInfo.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto mugongInfo = std::make_unique<cTextArea>();
    cTextArea* miRaw = mugongInfo.get();
    InsertChildById(&dlg, cMPRegistDialog::kMugongInfoId, std::move(mugongInfo));
    dlg.Linking();

    dlg.SetSuryunMugongInfo("Heavenly-Slash", 5);
    EXPECT_EQ(miRaw->GetScriptText(), "Mugong: Heavenly-Slash (Sung 5)");
}

TEST(CMPRegistDialogTest, SetSuryunMugongInfoNullNameHandled) {
    // 1:1 quirk: defensive — null mugongName is
    // rendered as "(null)" rather than crashing on
    // snprintf("%s", NULL).
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto mugongInfo = std::make_unique<cTextArea>();
    cTextArea* miRaw = mugongInfo.get();
    InsertChildById(&dlg, cMPRegistDialog::kMugongInfoId, std::move(mugongInfo));
    dlg.Linking();

    dlg.SetSuryunMugongInfo(nullptr, 3);
    EXPECT_EQ(miRaw->GetScriptText(), "Mugong: (null) (Sung 3)");
}

TEST(CMPRegistDialogTest, SetSuryunMugongInfoOnUnlinkedDialogIsSafe) {
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // No m_MugongInfo field — SetSuryunMugongInfo
    // must not crash.
    dlg.SetSuryunMugongInfo("Test", 1);
    SUCCEED();
}

// ===========================================================================
// SetPracticeInfo (1:1 with legacy sprintf + SetStaticValue)
// ===========================================================================

TEST(CMPRegistDialogTest, SetPracticeInfoComputesMinutes) {
    // 1:1 with legacy: LTime = limitime / 60000
    // (ms → min conversion).
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto practiceInfo = std::make_unique<cTextArea>();
    cTextArea* piRaw = practiceInfo.get();
    InsertChildById(&dlg, cMPRegistDialog::kPracticeInfoId, std::move(practiceInfo));
    dlg.Linking();

    // 3,600,000 ms = 60 min
    dlg.SetPracticeInfo(/*sung=*/5, /*limitime=*/3600000u,
                        /*kind=*/10, /*num=*/3, /*fee=*/5000u);
    EXPECT_EQ(piRaw->GetScriptText(),
              "Practice: Sung 5, 60 min, Kind 10, Num 3");
}

TEST(CMPRegistDialogTest, SetPracticeInfoSetsFee) {
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto fee = std::make_unique<cStatic>();
    cStatic* feeRaw = fee.get();
    auto practiceInfo = std::make_unique<cTextArea>();
    cTextArea* piRaw = practiceInfo.get();
    InsertChildById(&dlg, cMPRegistDialog::kFeeId,         std::move(fee));
    InsertChildById(&dlg, cMPRegistDialog::kPracticeInfoId, std::move(practiceInfo));
    dlg.Linking();

    // 1:1 with legacy: SetPracticeInfo's MONEYTYPE fee
    // is forwarded to m_Fee->SetStaticValue. The modern
    // cStatic::SetStaticValue stores the int32_t as
    // text via snprintf("%d", v) and GetStaticValue
    // returns atoi of that text, so feeRaw->GetStaticValue()
    // is 12345 after the call. (m_PracticeInfo must
    // be linked first because SetPracticeInfo early-
    // returns on `!m_PracticeInfo` — same 1:1 quirk
    // as the legacy where SetPracticeInfo's sprintf
    // target is m_PracticeInfo, not m_Fee.)
    dlg.SetPracticeInfo(/*sung=*/3, /*limitime=*/600000u,
                        /*kind=*/5, /*num=*/1, /*fee=*/12345u);
    EXPECT_EQ(feeRaw->GetStaticValue(), 12345);
    EXPECT_FALSE(piRaw->GetScriptText().empty());
}

TEST(CMPRegistDialogTest, SetPracticeInfoZeroLimitIsZeroMinutes) {
    // 1:1 quirk: limitime == 0 → LTime = 0. The
    // legacy would render "0 min"; the modern port
    // does the same.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto practiceInfo = std::make_unique<cTextArea>();
    cTextArea* piRaw = practiceInfo.get();
    InsertChildById(&dlg, cMPRegistDialog::kPracticeInfoId, std::move(practiceInfo));
    dlg.Linking();

    dlg.SetPracticeInfo(/*sung=*/1, /*limitime=*/0u,
                        /*kind=*/1, /*num=*/1, /*fee=*/0u);
    EXPECT_EQ(piRaw->GetScriptText(),
              "Practice: Sung 1, 0 min, Kind 1, Num 1");
    EXPECT_EQ(piRaw->GetScriptText().find("0 min") != std::string::npos, true);
}

TEST(CMPRegistDialogTest, SetPracticeInfoOnUnlinkedDialogIsSafe) {
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // No m_PracticeInfo + m_Fee fields — SetPracticeInfo
    // must not crash.
    dlg.SetPracticeInfo(1, 60000u, 1, 1, 100u);
    SUCCEED();
}

// ===========================================================================
// AddLink (1:1 with legacy AddLink)
// ===========================================================================

TEST(CMPRegistDialogTest, AddLinkToEmptyCellSucceeds) {
    // 1:1 with legacy: on an empty cell 0, IsAddable(0)
    // returns true, so the legacy skips the
    // DeleteIcon(0) branch and just calls
    // AddIcon(0, picon, TRUE). The modern port
    // does the same.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto mugongIconDlg = std::make_unique<cIconDialog>();
    cIconDialog* midRaw = mugongIconDlg.get();
    InsertChildById(&dlg, cMPRegistDialog::kMugongIconId, std::move(mugongIconDlg));
    dlg.Linking();

    cIcon* icon = reinterpret_cast<cIcon*>(0x1);
    dlg.AddLink(icon);
    EXPECT_EQ(midRaw->GetIconForIdx(0), icon);
}

TEST(CMPRegistDialogTest, AddLinkToOccupiedCellReplaces) {
    // 1:1 with legacy: if cell 0 is not addable
    // (occupied), DeleteIcon(0) first, then
    // AddIcon(0, picon, TRUE). The new icon replaces
    // the old one (1:1 quirk: legacy doesn't ask the
    // user "are you sure?", it just replaces).
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto mugongIconDlg = std::make_unique<cIconDialog>();
    cIconDialog* midRaw = mugongIconDlg.get();
    InsertChildById(&dlg, cMPRegistDialog::kMugongIconId, std::move(mugongIconDlg));
    dlg.Linking();

    cIcon* first  = reinterpret_cast<cIcon*>(0x1);
    cIcon* second = reinterpret_cast<cIcon*>(0x2);
    dlg.AddLink(first);
    dlg.AddLink(second);
    EXPECT_EQ(midRaw->GetIconForIdx(0), second);
}

TEST(CMPRegistDialogTest, AddLinkOnUnlinkedDialogIsSafe) {
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // No m_pMugongIconDlg — AddLink must not crash.
    dlg.AddLink(reinterpret_cast<cIcon*>(0x1));
    SUCCEED();
}

// ===========================================================================
// GetMugong (1:1 with legacy, CMugongBase TODO)
// ===========================================================================

TEST(CMPRegistDialogTest, GetMugongReturnsNull) {
    // 1:1 quirk: legacy casts cell 0 to CMugongBase*.
    // CMugongBase is not yet ported (R-12.x deferred,
    // same constraint as cPetWearedExDialog::CheckDuplication
    // and cWearedExDialog's Titan-vs-normal branch),
    // so the modern port returns nullptr unconditionally.
    // When CMugongBase is ported, this should be
    // replaced with the real cast.
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto mugongIconDlg = std::make_unique<cIconDialog>();
    InsertChildById(&dlg, cMPRegistDialog::kMugongIconId, std::move(mugongIconDlg));
    dlg.Linking();
    dlg.AddLink(reinterpret_cast<cIcon*>(0x1));

    EXPECT_EQ(dlg.GetMugong(), nullptr);
}

TEST(CMPRegistDialogTest, GetMugongOnUnlinkedDialogReturnsNull) {
    cMPRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetMugong(), nullptr);
}

}  // namespace mxh::ui::test
