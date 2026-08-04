// guildnoticedlg_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cGuildNoticeDlg (guild notice editor
// dialog: 1 cTextArea + 2 cButton).
//
// Covers modern/src/ui/guildnoticedlg.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\GuildNoticeDlg.h (310 B) and
//   墨香【源码】\[Client]MH\GuildNoticeDlg.cpp.
//
// What's tested:
//   - Default construction: cGuildNoticeDlg is a cDialog
//     and inherits its tree management.
//   - m_pNoticeText starts null (1:1 with legacy
//     default member init).
//   - Id constants match the local range 350-352
//     (1:1 with legacy GNotice_TEXTREA /
//     GNotice_SENDOKBTN / GNotice_CANCELBTN enum
//     symbols).
//   - Linking resolves the cTextArea child by id
//     and calls SetEnterAllow(FALSE) +
//     SetScriptText("") on it.
//   - Linking before Init is safe (no crash).
//   - Linking without children leaves m_pNoticeText
//     null.
//   - SetActive override calls base SetActive (the
//     modern cDialog updates its own state — we
//     verify via isVisible()).
//   - SetActive val=true pre-fills cTextArea with
//     SetScriptText("") (TODO GUILDMGR::GetGuildNotice
//     path is documented; the SetScriptText("") call
//     is the safe no-op while GUILDMGR is unported).
//   - SetActive val=false does not touch
//     m_pNoticeText (1:1 with legacy `if(val == TRUE)`
//     guard).
//   - SetActive without Linking is safe.
//   - OnActionEvent with non-WE_BTNCLICK we is no-op
//     (1:1 with legacy gate).
//   - OnActionEvent with WE_BTNCLICK + SEND id is
//     TODO (GUILDMGR not ported, R-12.x deferred).
//   - OnActionEvent with WE_BTNCLICK + CANCEL id is
//     TODO (SetActive dispatch would re-enter
//     GUILDMGR path, also deferred).
//   - OnActionEvent with unknown id is safe (1:1
//     with legacy switch fallthrough).
//   - OnActionEvent before Linking is safe.
//
// 1:1 quirks preserved:
//   - Ctor / dtor body empty (1:1 with legacy empty
//     CGuildNoticeDlg ctor).
//   - Linking only resolves cTextArea; the 2 button
//     children are handled by id in OnActionEvent
//     (legacy uses GetWindowForID inside switch
//     case; modern port uses the constants directly).
//   - SetActive override: notice pre-fill happens
//     BEFORE base SetActive (matches legacy call
//     order). 1:1 quirk: the modern port's
//     SetScriptText("") is a safe no-op while
//     GUILDMGR is unported (the legacy guard
//     `if(GUILDMGR->GetGuildNotice())` is also
//     impossible without GUILDMGR).
//   - 1:1 quirk: legacy typo'd OnActionEvent as
//     "OnActionEvnet" — modern port uses correct
//     spelling.
//   - Local id range 350-352 (distinct from
//     200-321 used by previous Tier 2 dialogs; no
//     collision).

#include "guildnoticedlg.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"
#include "cbutton.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CGuildNoticeDlgTest, DefaultConstructionHasNullPointer) {
    cGuildNoticeDlg dlg;
    // 1:1 quirk: ctor body is empty (legacy also has
    // empty CGuildNoticeDlg() ctor). m_pNoticeText
    // starts null.
    SUCCEED();
}

TEST(CGuildNoticeDlgTest, InheritsDialogTreeManagement) {
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CGuildNoticeDlgTest, IdConstantsAreDistinct) {
    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (GNotice_TEXTREA / GNotice_SENDOKBTN /
    // GNotice_CANCELBTN). Local range 350-352.
    EXPECT_NE(cGuildNoticeDlg::kIdNoticeText,
              cGuildNoticeDlg::kIdSendOkBtn);
    EXPECT_NE(cGuildNoticeDlg::kIdNoticeText,
              cGuildNoticeDlg::kIdCancelBtn);
    EXPECT_NE(cGuildNoticeDlg::kIdSendOkBtn,
              cGuildNoticeDlg::kIdCancelBtn);
}

TEST(CGuildNoticeDlgTest, IdConstantsMatchExpectedLocalRange) {
    // 1:1 with legacy enum symbols (numeric value is
    // a local range, distinct from 200-321 used by
    // previous Tier 2 dialogs).
    EXPECT_EQ(cGuildNoticeDlg::kIdNoticeText, 350);
    EXPECT_EQ(cGuildNoticeDlg::kIdSendOkBtn, 351);
    EXPECT_EQ(cGuildNoticeDlg::kIdCancelBtn, 352);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CGuildNoticeDlgTest, LinkingResolvesTextArea) {
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    // Add a cTextArea child with the legacy id.
    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 380, 380, nullptr, cGuildNoticeDlg::kIdNoticeText);
    text->InitTextArea({0, 0, 380, 380}, 256);
    cTextArea* pText = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    dlg.Linking();
    // m_pNoticeText is private; verified indirectly:
    // SetActive calls SetScriptText("") on it
    // (verified via pText->GetScriptText() == "").
    pText->SetScriptText("pre-existing text");
    dlg.SetActive(true);
    EXPECT_EQ(pText->GetScriptText(), "");
}

TEST(CGuildNoticeDlgTest, LinkingConfiguresEnterAllowFalse) {
    // 1:1 quirk: legacy Linking calls
    // SetEnterAllow(FALSE) on the cTextArea. Modern
    // cTextArea has SetEnterAllow (added in 0.13.30
    // alongside this port).
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 380, 380, nullptr, cGuildNoticeDlg::kIdNoticeText);
    text->InitTextArea({0, 0, 380, 380}, 256);
    cTextArea* pText = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    EXPECT_TRUE(pText->IsEnterAllow());  // default true
    dlg.Linking();
    EXPECT_FALSE(pText->IsEnterAllow());  // legacy sets FALSE
}

TEST(CGuildNoticeDlgTest, LinkingWithoutChildrenLeavesPointerNull) {
    // 1:1 with legacy: if no child with the id
    // exists, GetWindowForID returns null and
    // m_pNoticeText stays null. SetActive without
    // m_pNoticeText is safe.
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetActive(true);
    SUCCEED();
}

TEST(CGuildNoticeDlgTest, LinkingBeforeInitDoesNotCrash) {
    cGuildNoticeDlg dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetActive override
// ===========================================================================

TEST(CGuildNoticeDlgTest, SetActiveTrueUpdatesBaseState) {
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildNoticeDlgTest, SetActiveFalseUpdatesBaseState) {
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGuildNoticeDlgTest, SetActiveTrueClearsNoticeText) {
    // 1:1 quirk: legacy SetActive(val=TRUE) calls
    // m_pNoticeText->SetScriptText(GUILDMGR->GetGuildNotice()).
    // While GUILDMGR is unported, the modern port
    // performs SetScriptText("") (a safe no-op
    // placeholder). When GUILDMGR is ported, this
    // becomes the real notice string.
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 380, 380, nullptr, cGuildNoticeDlg::kIdNoticeText);
    text->InitTextArea({0, 0, 380, 380}, 256);
    cTextArea* pText = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));
    dlg.Linking();

    pText->SetScriptText("stale text");
    dlg.SetActive(true);
    EXPECT_EQ(pText->GetScriptText(), "");
}

TEST(CGuildNoticeDlgTest, SetActiveFalseDoesNotTouchNoticeText) {
    // 1:1 with legacy: the `if (val == TRUE)` guard
    // means SetActive(FALSE) does not touch
    // m_pNoticeText. Pre-existing text survives.
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 380, 380, nullptr, cGuildNoticeDlg::kIdNoticeText);
    text->InitTextArea({0, 0, 380, 380}, 256);
    cTextArea* pText = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));
    dlg.Linking();

    pText->SetScriptText("preserved text");
    dlg.SetActive(false);
    EXPECT_EQ(pText->GetScriptText(), "preserved text");
}

TEST(CGuildNoticeDlgTest, SetActiveWithoutLinkIsSafe) {
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGuildNoticeDlgTest, SetActiveBeforeLinkingIsSafe) {
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    // No Linking call — m_pNoticeText is null.
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

// ===========================================================================
// OnActionEvent
// ===========================================================================

TEST(CGuildNoticeDlgTest, OnActionEventNonBtnClickIsNoOp) {
    // 1:1 with legacy: the `if (we & WE_BTNCLICK)`
    // gate means non-click events are silently
    // ignored. Verify the SEND id is not processed
    // when the flag is missing.
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 380, 380, nullptr, cGuildNoticeDlg::kIdNoticeText);
    text->InitTextArea({0, 0, 380, 380}, 256);
    cTextArea* pText = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));
    dlg.Linking();
    dlg.SetActive(true);

    pText->SetScriptText("test notice");
    constexpr std::uint32_t NOT_BTNCLICK = 0x0000;
    dlg.OnActionEvent(cGuildNoticeDlg::kIdSendOkBtn, nullptr, NOT_BTNCLICK);
    // Notice text preserved (no action taken).
    EXPECT_EQ(pText->GetScriptText(), "test notice");
    // Dialog still active (no SetActive(FALSE) dispatch).
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildNoticeDlgTest, OnActionEventUnknownIdIsNoOp) {
    // 1:1 with legacy: unknown ids are silently
    // ignored (switch fallthrough).
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    constexpr std::uint32_t WE_BTNCLICK = 0x0001;
    dlg.OnActionEvent(/*lId=*/9999, nullptr, WE_BTNCLICK);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildNoticeDlgTest, OnActionEventBeforeLinkingDoesNotCrash) {
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;
    dlg.OnActionEvent(cGuildNoticeDlg::kIdSendOkBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(cGuildNoticeDlg::kIdCancelBtn, nullptr, WE_BTNCLICK);
    SUCCEED();
}


// ===========================================================================
// C-Batch-2.50: host GUILDMGR callbacks (GetGuildNotice + SetGuildNotice)
// ===========================================================================

namespace {

struct GuildNoticeHost {
    int getCalls = 0;
    int setCalls = 0;
    std::string lastStoredNotice;
    const char* noticeToReturn = "stub_guild_notice";
};

const char* HostGetGuildNotice(void* userData) {
    auto* h = static_cast<GuildNoticeHost*>(userData);
    ++h->getCalls;
    return h->noticeToReturn;
}

void HostSetGuildNotice(const char* notice, void* userData) {
    auto* h = static_cast<GuildNoticeHost*>(userData);
    ++h->setCalls;
    h->lastStoredNotice = notice ? notice : "";
}

struct LinkedGuildNotice {
    cGuildNoticeDlg dlg;
    std::unique_ptr<cTextArea> noticeText;
    cTextArea* pText = nullptr;

    LinkedGuildNotice() {
        dlg.Init(0, 0, 400, 400, nullptr, 0);
        noticeText = std::make_unique<cTextArea>();
        noticeText->Init(0, 0, 380, 380, nullptr,
                         cGuildNoticeDlg::kIdNoticeText);
        noticeText->InitTextArea({0, 0, 380, 380}, 256);
        pText = noticeText.get();
        dlg.Add(std::unique_ptr<cWindow>(noticeText.release()));
        dlg.Linking();
    }
};

}  // namespace

TEST(CGuildNoticeDlgTest, MaxGuildNoticeIsLegacy150) {
    EXPECT_EQ(cGuildNoticeDlg::kMaxGuildNotice, 150);
}

TEST(CGuildNoticeDlgTest, WeBtnClickIsLegacy1) {
    EXPECT_EQ(cGuildNoticeDlg::kWeBtnClick, 0x0001u);
}

TEST(CGuildNoticeDlgTest, SetActiveTruePrefillsFromHostCallback) {
    LinkedGuildNotice ln;
    GuildNoticeHost host;
    host.noticeToReturn = "Greetings from the guild master";
    ln.dlg.SetGuildNoticeCallbacks(&HostGetGuildNotice,
                                   &HostSetGuildNotice, &host);
    ln.dlg.SetActive(true);
    EXPECT_EQ(host.getCalls, 1);
    EXPECT_EQ(ln.pText->GetScriptText(), "Greetings from the guild master");
}

TEST(CGuildNoticeDlgTest, SetActiveTrueNullResultClearsToEmpty) {
    LinkedGuildNotice ln;
    GuildNoticeHost host;
    host.noticeToReturn = nullptr;
    ln.dlg.SetGuildNoticeCallbacks(&HostGetGuildNotice,
                                   &HostSetGuildNotice, &host);
    ln.pText->SetScriptText("stale");
    ln.dlg.SetActive(true);
    EXPECT_EQ(ln.pText->GetScriptText(), "");
}

TEST(CGuildNoticeDlgTest, SetActiveTrueWithoutCallbackClearsToEmpty) {
    LinkedGuildNotice ln;
    ln.pText->SetScriptText("stale");
    ln.dlg.SetActive(true);
    EXPECT_EQ(ln.pText->GetScriptText(), "");
}

TEST(CGuildNoticeDlgTest, SetActiveFalseDoesNotInvokeGetGuildNotice) {
    LinkedGuildNotice ln;
    GuildNoticeHost host;
    ln.dlg.SetGuildNoticeCallbacks(&HostGetGuildNotice,
                                   &HostSetGuildNotice, &host);
    ln.dlg.SetActive(true);
    EXPECT_EQ(host.getCalls, 1);
    ln.dlg.SetActive(false);
    EXPECT_EQ(host.getCalls, 1);  // unchanged
}

TEST(CGuildNoticeDlgTest, OnActionEventSendDispatchesToSetGuildNotice) {
    LinkedGuildNotice ln;
    GuildNoticeHost host;
    ln.dlg.SetGuildNoticeCallbacks(&HostGetGuildNotice,
                                   &HostSetGuildNotice, &host);
    ln.dlg.SetActive(true);
    ln.pText->SetScriptText("final guild notice");
    EXPECT_EQ(host.getCalls, 1);   // pre-fill
    EXPECT_EQ(host.setCalls, 0);
    ln.dlg.OnActionEvent(cGuildNoticeDlg::kIdSendOkBtn, nullptr,
                         cGuildNoticeDlg::kWeBtnClick);
    EXPECT_EQ(host.setCalls, 1);
    EXPECT_EQ(host.lastStoredNotice, "final guild notice");
    EXPECT_EQ(ln.pText->GetScriptText(), "final guild notice");
    EXPECT_FALSE(ln.dlg.isActive());
}

TEST(CGuildNoticeDlgTest, OnActionEventSendStillClosesWhenCallbackNull) {
    LinkedGuildNotice ln;
    ln.dlg.SetActive(true);
    ln.pText->SetScriptText("any notice");
    // No SetGuildNoticeCallbacks registered
    ln.dlg.OnActionEvent(cGuildNoticeDlg::kIdSendOkBtn, nullptr,
                         cGuildNoticeDlg::kWeBtnClick);
    EXPECT_FALSE(ln.dlg.isActive());
}

TEST(CGuildNoticeDlgTest, OnActionEventSendWithoutLinkingClosesDialog) {
    cGuildNoticeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.OnActionEvent(cGuildNoticeDlg::kIdSendOkBtn, nullptr,
                      cGuildNoticeDlg::kWeBtnClick);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGuildNoticeDlgTest, OnActionEventCancelDeactivates) {
    LinkedGuildNotice ln;
    ln.dlg.SetActive(true);
    ln.dlg.OnActionEvent(cGuildNoticeDlg::kIdCancelBtn, nullptr,
                         cGuildNoticeDlg::kWeBtnClick);
    EXPECT_FALSE(ln.dlg.isActive());
}

TEST(CGuildNoticeDlgTest, OnActionEventCancelDoesNotCallSetGuildNotice) {
    LinkedGuildNotice ln;
    GuildNoticeHost host;
    ln.dlg.SetGuildNoticeCallbacks(&HostGetGuildNotice,
                                   &HostSetGuildNotice, &host);
    ln.dlg.SetActive(true);
    ln.dlg.OnActionEvent(cGuildNoticeDlg::kIdCancelBtn, nullptr,
                         cGuildNoticeDlg::kWeBtnClick);
    EXPECT_EQ(host.setCalls, 0);
    EXPECT_FALSE(ln.dlg.isActive());
}

TEST(CGuildNoticeDlgTest, OnActionEventSendReplacesCallback) {
    LinkedGuildNotice ln;
    GuildNoticeHost first;
    GuildNoticeHost second;
    ln.dlg.SetGuildNoticeCallbacks(&HostGetGuildNotice,
                                   &HostSetGuildNotice, &first);
    ln.dlg.SetGuildNoticeCallbacks(&HostGetGuildNotice,
                                   &HostSetGuildNotice, &second);
    ln.dlg.SetActive(true);
    EXPECT_EQ(first.getCalls, 0);
    EXPECT_EQ(second.getCalls, 1);
    ln.pText->SetScriptText("second notice");
    ln.dlg.OnActionEvent(cGuildNoticeDlg::kIdSendOkBtn, nullptr,
                         cGuildNoticeDlg::kWeBtnClick);
    EXPECT_EQ(first.setCalls, 0);
    EXPECT_EQ(second.setCalls, 1);
    EXPECT_EQ(second.lastStoredNotice, "second notice");
}

TEST(CGuildNoticeDlgTest, CallbacksAllowNullUserData) {
    LinkedGuildNotice ln;
    auto getFn = [](void*) -> const char* { return "noop-notice"; };
    auto setFn = [](const char*, void*) {};
    ln.dlg.SetGuildNoticeCallbacks(getFn, setFn);
    ln.dlg.SetActive(true);
    EXPECT_EQ(ln.pText->GetScriptText(), "noop-notice");
    ln.dlg.OnActionEvent(cGuildNoticeDlg::kIdSendOkBtn, nullptr,
                         cGuildNoticeDlg::kWeBtnClick);
    EXPECT_FALSE(ln.dlg.isActive());
}

}  // namespace mxh::ui::test
