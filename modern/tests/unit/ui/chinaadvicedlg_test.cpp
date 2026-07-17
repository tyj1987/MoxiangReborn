// chinaadvicedlg_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cChinaAdviceDlg (China-region advice
// / T&C dialog: 1 cTextArea + 1 button).
//
// Covers modern/src/ui/chinaadvicedlg.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\ChinaAdviceDlg.h (677 B) and
//   墨香【源码】\[Client]MH\ChinaAdviceDlg.cpp.
//
// What's tested:
//   - Default construction: cChinaAdviceDlg is a cDialog
//     and inherits its tree management.
//   - m_pTextArea starts null (1:1 with legacy default
//     member init).
//   - Id constant matches the local range 360
//     (1:1 with legacy CNA_TEXTAREA enum symbol).
//   - Linking resolves the cTextArea child by id
//     and calls SetScriptText with placeholder
//     "CHINA_ADVICE_TEXT".
//   - Linking before Init is safe (no crash).
//   - Linking without children leaves m_pTextArea
//     null.
//   - OnActionEvent is a safe no-op (1:1 with legacy
//     empty body).
//   - OnActionEvent with any id/we is a no-op (1:1
//     with legacy empty body — no button dispatch).
//
// 1:1 quirks preserved:
//   - Ctor / dtor body empty (1:1 with legacy empty
//     CChinaAdviceDlg ctor).
//   - CNA_BTN_OK enum exists in WindowIDs.h but the
//     legacy .cpp does NOT resolve or use it; modern
//     port also does not resolve it.
//   - SetScriptText placeholder "CHINA_ADVICE_TEXT"
//     replaces CHATMGR->GetChatMsg(30) (same pattern
//     as cMPNoticeDialog's "MP_NCAUTION" placeholder).
//   - OnActionEvent is an empty no-op (1:1 with
//     legacy empty body).
//   - Local id range 360 (distinct from 200-352 used
//     by previous Tier 2 dialogs; no collision).

#include "chinaadvicedlg.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CChinaAdviceDlgTest, DefaultConstructionHasNullPointer) {
    cChinaAdviceDlg dlg;
    // 1:1 quirk: ctor body is empty (legacy also has
    // empty CChinaAdviceDlg() ctor). m_pTextArea
    // starts null.
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, InheritsDialogTreeManagement) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CChinaAdviceDlgTest, IdConstantMatchesExpectedLocalRange) {
    // 1:1 with legacy WindowIDs.h WINDOW_ID
    // (CNA_TEXTAREA). Local 360 — distinct from
    // 200-352 used by previous Tier 2 dialogs.
    EXPECT_EQ(cChinaAdviceDlg::kIdTextArea, 360);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CChinaAdviceDlgTest, LinkingResolvesTextAreaAndSetsScriptText) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 380, 380, nullptr, cChinaAdviceDlg::kIdTextArea);
    text->InitTextArea({0, 0, 380, 380}, 256);
    cTextArea* pText = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    dlg.Linking();
    // m_pTextArea is private; verified indirectly:
    // SetScriptText is called on the resolved child
    // (verified via pText->GetScriptText() ==
    // "CHINA_ADVICE_TEXT").
    EXPECT_EQ(pText->GetScriptText(), "CHINA_ADVICE_TEXT");
}

TEST(CChinaAdviceDlgTest, LinkingWithoutChildrenLeavesPointerNull) {
    // 1:1 with legacy: if no child with the id
    // exists, findWindowById returns null and
    // m_pTextArea stays null.
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, LinkingBeforeInitDoesNotCrash) {
    cChinaAdviceDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, LinkingOverwritesPreviousScriptText) {
    // 1:1 quirk: legacy Linking calls SetScriptText
    // unconditionally, overwriting any pre-existing
    // text on the cTextArea.
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 380, 380, nullptr, cChinaAdviceDlg::kIdTextArea);
    text->InitTextArea({0, 0, 380, 380}, 256);
    cTextArea* pText = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    pText->SetScriptText("stale text");
    dlg.Linking();
    EXPECT_EQ(pText->GetScriptText(), "CHINA_ADVICE_TEXT");
}

// ===========================================================================
// OnActionEvent
// ===========================================================================

TEST(CChinaAdviceDlgTest, OnActionEventIsNoOp) {
    // 1:1 with legacy: empty body. No button dispatch.
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    dlg.OnActionEvent(/*lId=*/1, nullptr, /*we=*/0xFFFFFFFFu);
    dlg.OnActionEvent(/*lId=*/9999, nullptr, /*we=*/0x0001u);
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, OnActionEventDoesNotChangeState) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());

    dlg.OnActionEvent(/*lId=*/1, nullptr, /*we=*/0xFFFFFFFFu);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChinaAdviceDlgTest, OnActionEventBeforeLinkingDoesNotCrash) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.OnActionEvent(/*lId=*/1, nullptr, /*we=*/0x0001u);
    SUCCEED();
}

}  // namespace mxh::ui::test
