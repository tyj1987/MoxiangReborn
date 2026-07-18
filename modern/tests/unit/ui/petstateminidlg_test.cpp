// petstateminidlg_test.cpp — 1:1 port verification tests for cPetStateMiniDlg.

#include "petstateminidlg.hpp"
#include "cbutton.hpp"
#include "cguagen.hpp"
#include "cstatic.hpp"
#include "cWindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cPetStateMiniDlg;
using mxh::ui::cButton;
using mxh::ui::cGuagen;
using mxh::ui::cStatic;
using mxh::ui::cWindow;
using WE = cWindow::WindowEvent;

namespace {

std::unique_ptr<cPetStateMiniDlg> MakeDialog() {
    auto d = std::make_unique<cPetStateMiniDlg>();
    d->Init(0, 0, 240, 120, nullptr, 809);
    return d;
}

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST(CPetStateMiniDlg, IdConstantsMatchLocalRange) {
    EXPECT_EQ(cPetStateMiniDlg::kIdName,         800);
    EXPECT_EQ(cPetStateMiniDlg::kIdState,        801);
    EXPECT_EQ(cPetStateMiniDlg::kIdFriend,       802);
    EXPECT_EQ(cPetStateMiniDlg::kIdStamina,      803);
    EXPECT_EQ(cPetStateMiniDlg::kIdFriendGuage,  804);
    EXPECT_EQ(cPetStateMiniDlg::kIdStaminaGuage, 805);
    EXPECT_EQ(cPetStateMiniDlg::kIdUseRestBtn,   806);
    EXPECT_EQ(cPetStateMiniDlg::kIdInvenBtn,     807);
    EXPECT_EQ(cPetStateMiniDlg::kIdToggleBtn,    808);
}

TEST(CPetStateMiniDlg, AllIdsAreDistinct) {
    int ids[] = {cPetStateMiniDlg::kIdName, cPetStateMiniDlg::kIdState,
                 cPetStateMiniDlg::kIdFriend, cPetStateMiniDlg::kIdStamina,
                 cPetStateMiniDlg::kIdFriendGuage, cPetStateMiniDlg::kIdStaminaGuage,
                 cPetStateMiniDlg::kIdUseRestBtn, cPetStateMiniDlg::kIdInvenBtn,
                 cPetStateMiniDlg::kIdToggleBtn};
    for (std::size_t i = 0; i < 9; ++i) {
        for (std::size_t j = i + 1; j < 9; ++j) {
            EXPECT_NE(ids[i], ids[j]) << "ids[" << i << "] collides with ids[" << j << "]";
        }
    }
}

TEST(CPetStateMiniDlg, DefaultConstructionHasNullPointers) {
    auto d = MakeDialog();
    EXPECT_EQ(d->GetNameTextWin(),       nullptr);
    EXPECT_EQ(d->GetUseRestTextWin(),    nullptr);
    EXPECT_EQ(d->GetFriendShipTextWin(), nullptr);
    EXPECT_EQ(d->GetStaminaTextWin(),    nullptr);
    EXPECT_EQ(d->GetFriendShipGuage(),   nullptr);
    EXPECT_EQ(d->GetStaminaGuage(),      nullptr);
    EXPECT_EQ(d->GetUseRestButton(),     nullptr);
    EXPECT_EQ(d->GetInvenButton(),       nullptr);
    EXPECT_EQ(d->GetToggleButton(),      nullptr);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CPetStateMiniDlg, LinkingMaterializesAllNineChildren) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_NE(d->GetNameTextWin(),       nullptr);
    EXPECT_NE(d->GetUseRestTextWin(),    nullptr);
    EXPECT_NE(d->GetFriendShipTextWin(), nullptr);
    EXPECT_NE(d->GetStaminaTextWin(),    nullptr);
    EXPECT_NE(d->GetFriendShipGuage(),   nullptr);
    EXPECT_NE(d->GetStaminaGuage(),      nullptr);
    EXPECT_NE(d->GetUseRestButton(),     nullptr);
    EXPECT_NE(d->GetInvenButton(),       nullptr);
    EXPECT_NE(d->GetToggleButton(),      nullptr);
}

TEST(CPetStateMiniDlg, LinkingSetsChildIds) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_EQ(d->GetNameTextWin()->id(),       cPetStateMiniDlg::kIdName);
    EXPECT_EQ(d->GetUseRestTextWin()->id(),    cPetStateMiniDlg::kIdState);
    EXPECT_EQ(d->GetFriendShipTextWin()->id(), cPetStateMiniDlg::kIdFriend);
    EXPECT_EQ(d->GetStaminaTextWin()->id(),    cPetStateMiniDlg::kIdStamina);
    EXPECT_EQ(d->GetFriendShipGuage()->id(),   cPetStateMiniDlg::kIdFriendGuage);
    EXPECT_EQ(d->GetStaminaGuage()->id(),      cPetStateMiniDlg::kIdStaminaGuage);
    EXPECT_EQ(d->GetUseRestButton()->id(),     cPetStateMiniDlg::kIdUseRestBtn);
    EXPECT_EQ(d->GetInvenButton()->id(),       cPetStateMiniDlg::kIdInvenBtn);
}

TEST(CPetStateMiniDlg, LinkingMaterializesToggleButton) {
    // (The previous test had a typo: Linking does materialize all 9,
    // including the toggle button. This is a sanity re-check.)
    auto d = MakeDialog();
    d->Linking();
    EXPECT_NE(d->GetToggleButton(), nullptr);
    EXPECT_EQ(d->GetToggleButton()->id(), cPetStateMiniDlg::kIdToggleBtn);
}

TEST(CPetStateMiniDlg, LinkingIdempotent) {
    auto d = MakeDialog();
    d->Linking();
    const cStatic* nameFirst   = d->GetNameTextWin();
    const cGuagen* friendFirst = d->GetFriendShipGuage();
    const cButton* toggleFirst = d->GetToggleButton();
    d->Linking();
    EXPECT_EQ(d->GetNameTextWin(),       nameFirst);
    EXPECT_EQ(d->GetFriendShipGuage(),   friendFirst);
    EXPECT_EQ(d->GetToggleButton(),      toggleFirst);
}

// ---------------------------------------------------------------------------
// OnActionEvent — 3 button branches
// ---------------------------------------------------------------------------

TEST(CPetStateMiniDlg, OnActionEventNonBtnClickIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    // 1:1 quirk: legacy silently returns if not WE_BTNCLICK.
    // Modern port: any value != LButtonClick (4) is no-op.
    const std::uint32_t kMouseMove =
        static_cast<std::uint32_t>(WE::MouseMove);
    d->OnActionEvent(cPetStateMiniDlg::kIdToggleBtn, nullptr, kMouseMove);
    // No crash, no observable side effect (PETMGR is stubbed no-op).
}

TEST(CPetStateMiniDlg, OnActionEventToggleBtnIsNoOpStubsCall) {
    auto d = MakeDialog();
    d->Linking();
    const std::uint32_t kLButtonClick =
        static_cast<std::uint32_t>(WE::LButtonClick);
    d->OnActionEvent(cPetStateMiniDlg::kIdToggleBtn, nullptr, kLButtonClick);
    // PETMGR->TogglePetStateDlg() stubbed no-op — must not crash.
    SUCCEED();
}

TEST(CPetStateMiniDlg, OnActionEventInvenBtnIsNoOpStubsCall) {
    auto d = MakeDialog();
    d->Linking();
    const std::uint32_t kLButtonClick =
        static_cast<std::uint32_t>(WE::LButtonClick);
    d->OnActionEvent(cPetStateMiniDlg::kIdInvenBtn, nullptr, kLButtonClick);
    // PETMGR->OpenPetInvenDlg() stubbed no-op — must not crash.
    SUCCEED();
}

TEST(CPetStateMiniDlg, OnActionEventUseRestBtnWithNoPetIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    const std::uint32_t kLButtonClick =
        static_cast<std::uint32_t>(WE::LButtonClick);
    // 1:1 quirk: legacy `if(NULL == PETMGR->GetCurSummonPet()) return;`
    // — no pet summoned → no-op. Stub returns nullptr, so this branch
    // always early-returns.
    d->OnActionEvent(cPetStateMiniDlg::kIdUseRestBtn, nullptr, kLButtonClick);
    SUCCEED();
}

TEST(CPetStateMiniDlg, OnActionEventUnknownIdIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    const std::uint32_t kLButtonClick =
        static_cast<std::uint32_t>(WE::LButtonClick);
    d->OnActionEvent(99999, nullptr, kLButtonClick);  // unknown id
    // 1:1 quirk: legacy silently ignores unknown lId (no `else` branch).
    SUCCEED();
}

TEST(CPetStateMiniDlg, OnActionEventBeforeLinkingIsSafe) {
    auto d = MakeDialog();
    // No Linking → m_pToggleBtn etc. are nullptr. OnActionEvent must
    // not crash (the lId is unknown or the button is null).
    const std::uint32_t kLButtonClick =
        static_cast<std::uint32_t>(WE::LButtonClick);
    d->OnActionEvent(cPetStateMiniDlg::kIdToggleBtn, nullptr, kLButtonClick);
    d->OnActionEvent(cPetStateMiniDlg::kIdInvenBtn, nullptr, kLButtonClick);
    d->OnActionEvent(cPetStateMiniDlg::kIdUseRestBtn, nullptr, kLButtonClick);
    SUCCEED();
}
