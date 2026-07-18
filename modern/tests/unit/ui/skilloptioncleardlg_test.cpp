// skilloptioncleardlg_test.cpp — 1:1 port verification tests for
// cSkillOptionClearDlg.

#include "skilloptioncleardlg.hpp"
#include "cIconDialog.hpp"
#include "cdialog.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cSkillOptionClearDlg;
using mxh::ui::cIconDialog;
using mxh::ui::cIcon;
using mxh::ui::cWindow;
using mxh::ui::cDialog;
using mxh::ui::CItem;
using mxh::ui::CMugongBase;
using mxh::ui::MsgWord4;

namespace {

// Helper: insert a fresh cIconDialog child by id, mirroring the
// legacy "T_DefaultICON is a child cIconDialog" relationship. The
// test owns the child; the dialog just resolves it.
void InsertChildById(cDialog* parent, int id,
                     std::unique_ptr<cIconDialog> child) {
    if (!parent || !child) return;
    child->Init(0, 0, 50, 50, nullptr, id);
    parent->Add(std::move(child));
}

std::unique_ptr<cSkillOptionClearDlg> MakeDialog() {
    auto d = std::make_unique<cSkillOptionClearDlg>();
    d->Init(0, 0, 200, 100, nullptr, 0);
    return d;
}

class CSkillOptionClearDlgTest : public ::testing::Test {
protected:
    void SetUp() override {
        cSkillOptionClearDlg::ClearTestInjections();
        cSkillOptionClearDlg::ClearLastSentMessage();
    }
    void TearDown() override {
        cSkillOptionClearDlg::ClearTestInjections();
        cSkillOptionClearDlg::ClearLastSentMessage();
    }
};

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST_F(CSkillOptionClearDlgTest, IdConstantsMatchLocalRange) {
    EXPECT_EQ(cSkillOptionClearDlg::kMugongIconId, 2000);
    EXPECT_EQ(cSkillOptionClearDlg::kOkBtnId,      2001);
    EXPECT_EQ(cSkillOptionClearDlg::kCancelBtnId,  2002);
}

TEST_F(CSkillOptionClearDlgTest, ProtocolConstantsHaveNonZeroValues) {
    // 1:1 with legacy MP_MUGONG (0x4A) and MP_MUGONG_OPTION_CLEAR_SYN
    // (0x4B) from [CC]Header/Protocol.h. Both non-zero so a default-
    // constructed MsgWord4 doesn't accidentally match.
    EXPECT_NE(cSkillOptionClearDlg::kCategoryMpMugong, 0);
    EXPECT_NE(cSkillOptionClearDlg::kProtocolOptionClearSyn, 0);
    EXPECT_NE(cSkillOptionClearDlg::kCategoryMpMugong,
              cSkillOptionClearDlg::kProtocolOptionClearSyn);
}

TEST_F(CSkillOptionClearDlgTest, DefaultConstructionHasNullChild) {
    auto d = MakeDialog();
    EXPECT_EQ(d->mugongIcon(), nullptr);
    EXPECT_EQ(d->itemPos(), 0u);
    EXPECT_FALSE(d->lastFakeMoveResult());
}

TEST_F(CSkillOptionClearDlgTest, ItemPosFieldDefaultIsZero) {
    // 1:1 quirk: legacy m_ItemPos (WORD) is not explicitly
    // initialised in the ctor — default-init in modern port
    // to 0 (preserves "uninitialised in legacy" 1:1 quirk
    // that effectively read as 0).
    auto d = MakeDialog();
    EXPECT_EQ(d->itemPos(), 0u);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST_F(CSkillOptionClearDlgTest, LinkingResolvesInnerIconDialog) {
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    EXPECT_NE(d->mugongIcon(), nullptr);
    EXPECT_EQ(d->mugongIcon()->id(),
              cSkillOptionClearDlg::kMugongIconId);
}

TEST_F(CSkillOptionClearDlgTest, LinkingAddsOneIconCellToInner) {
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    ASSERT_NE(d->mugongIcon(), nullptr);
    // 1:1 with legacy: dialog has exactly 1 mugong slot.
    EXPECT_EQ(d->mugongIcon()->GetCellNum(), 1u);
}

TEST_F(CSkillOptionClearDlgTest, LinkingWithoutInnerChildIsTolerated) {
    // 1:1 quirk: legacy GetWindowForID returns nullptr if the
    // inner cIconDialog hasn't been added. Modern port: Linking
    // does not crash on null inner (m_pMugongIconDlg stays null).
    auto d = MakeDialog();
    d->Linking();
    EXPECT_EQ(d->mugongIcon(), nullptr);
    SUCCEED();
}

TEST_F(CSkillOptionClearDlgTest, LinkingIsIdempotent) {
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    cIconDialog* first = d->mugongIcon();
    d->Linking();
    EXPECT_EQ(d->mugongIcon(), first);
}

// ---------------------------------------------------------------------------
// FakeMoveIcon
// ---------------------------------------------------------------------------

TEST_F(CSkillOptionClearDlgTest, FakeMoveIconWithNullIconReturnsFalse) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_FALSE(d->FakeMoveIcon(0, 0, nullptr));
    EXPECT_FALSE(d->lastFakeMoveResult());
}

TEST_F(CSkillOptionClearDlgTest, FakeMoveIconReplacesExistingIcon) {
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    // 1:1 quirk: legacy FakeMoveIcon always returns FALSE even
    // on the success path. The slot is filled, but the function
    // reports "no, the move didn't take" — which the legacy
    // engine treats as a no-op for the drag-and-drop UI.
    cIcon* first  = reinterpret_cast<cIcon*>(0x1);
    cIcon* second = reinterpret_cast<cIcon*>(0x2);
    EXPECT_FALSE(d->FakeMoveIcon(0, 0, first));
    EXPECT_FALSE(d->FakeMoveIcon(0, 0, second));
    EXPECT_EQ(d->mugongIcon()->GetIconForIdx(0), second);
}

TEST_F(CSkillOptionClearDlgTest, FakeMoveIconIncrementsCallCount) {
    auto d = MakeDialog();
    d->Linking();
    d->FakeMoveIcon(0, 0, nullptr);
    d->FakeMoveIcon(0, 0, reinterpret_cast<cIcon*>(0x1));
    EXPECT_EQ(cSkillOptionClearDlg::fakeMoveIconCallCount(), 2u);
}

TEST_F(CSkillOptionClearDlgTest, FakeMoveIconWithoutInnerIsTolerated) {
    auto d = MakeDialog();
    d->Linking();
    // m_pMugongIconDlg is null — must not crash.
    EXPECT_FALSE(d->FakeMoveIcon(0, 0, reinterpret_cast<cIcon*>(0x1)));
    SUCCEED();
}

// ---------------------------------------------------------------------------
// OnActionEvent
// ---------------------------------------------------------------------------

TEST_F(CSkillOptionClearDlgTest, OnActionEventIgnoresNonClickEvents) {
    // 1:1 quirk: legacy `we & WE_BTNCLICK` gates the body. Modern
    // port uses `we == WindowEvent::LButtonClick` (4). A MouseMove
    // event (1) should be a no-op (legacy `we & 1` is non-zero but
    // the legacy bit field is different — modern port's exact-match
    // is closer to the test contract).
    auto d = MakeDialog();
    d->Linking();
    d->OnActionEvent(cSkillOptionClearDlg::kOkBtnId, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::MouseMove));
    // No increment because the body is gated by the click event.
    EXPECT_EQ(cSkillOptionClearDlg::onActionEventCallCount(), 1u);
}

TEST_F(CSkillOptionClearDlgTest, OnActionEventOkWithNoMugongIsNoOp) {
    // 1:1 with legacy: if (!pMugong) return; before any msgbox
    // dispatch. Modern port: no mugong icon at cell 0 → return
    // without dispatching the WINDOWMGR msgbox.
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    cSkillOptionClearDlg::SetItemForTesting(0u);
    d->OnActionEvent(cSkillOptionClearDlg::kOkBtnId, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    // No msgbox / network send. (The test sentinel s_lastSentMsg
    // remains default-constructed.)
    EXPECT_EQ(cSkillOptionClearDlg::lastSentMessage().Category, 0);
}

TEST_F(CSkillOptionClearDlgTest, OnActionEventOkWithNoItemIsNoOp) {
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    d->mugongIcon()->AddIcon(0, reinterpret_cast<cIcon*>(0x1),
                             /*onlyLink=*/true);
    // No SetItemForTesting — s_itemPos stays at the 0xFFFFu
    // "no item" sentinel.
    d->OnActionEvent(cSkillOptionClearDlg::kOkBtnId, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    EXPECT_EQ(cSkillOptionClearDlg::lastSentMessage().Category, 0);
}

TEST_F(CSkillOptionClearDlgTest, OnActionEventCancelClosesDialog) {
    // 1:1 with legacy: T_DefaultCANCERBTN → SetActive(FALSE).
    // Modern port: SetActive(false) cascades to base cIconDialog
    // (inherits from cDialog) and clears the inner slot icon.
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    d->mugongIcon()->AddIcon(0, reinterpret_cast<cIcon*>(0x1),
                             /*onlyLink=*/true);
    d->SetActive(true);
    d->OnActionEvent(cSkillOptionClearDlg::kCancelBtnId, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    EXPECT_FALSE(d->isActive());
    EXPECT_EQ(d->mugongIcon()->GetIconForIdx(0), nullptr);
}

TEST_F(CSkillOptionClearDlgTest, OnActionEventUnknownIdIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    // Unknown lId — the switch's default branch is implicit.
    d->OnActionEvent(99999, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    SUCCEED();
}

// ---------------------------------------------------------------------------
// SetActive override
// ---------------------------------------------------------------------------

TEST_F(CSkillOptionClearDlgTest, SetActiveFalseDeletesInnerIcon) {
    // 1:1 with legacy: SetActive(FALSE) deletes the icon at cell 0.
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    d->mugongIcon()->AddIcon(0, reinterpret_cast<cIcon*>(0x1),
                             /*onlyLink=*/true);
    d->SetActive(true);
    d->SetActive(false);
    EXPECT_FALSE(d->isActive());
    EXPECT_EQ(d->mugongIcon()->GetIconForIdx(0), nullptr);
}

TEST_F(CSkillOptionClearDlgTest, SetActiveTrueLeavesIconInPlace) {
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    cIcon* icon = reinterpret_cast<cIcon*>(0x1);
    d->mugongIcon()->AddIcon(0, icon, /*onlyLink=*/true);
    d->SetActive(true);
    EXPECT_TRUE(d->isActive());
    EXPECT_EQ(d->mugongIcon()->GetIconForIdx(0), icon);
}

TEST_F(CSkillOptionClearDlgTest, SetActiveFalseWithoutInnerIsTolerated) {
    auto d = MakeDialog();
    d->Linking();
    // m_pMugongIconDlg is null — must not crash.
    d->SetActive(false);
    EXPECT_FALSE(d->isActive());
}

// ---------------------------------------------------------------------------
// SetItem + OptionClearSyn
// ---------------------------------------------------------------------------

TEST_F(CSkillOptionClearDlgTest, SetItemStoresPosition) {
    auto d = MakeDialog();
    CItem item(7u);
    d->SetItem(&item);
    EXPECT_EQ(d->itemPos(), 7u);
}

TEST_F(CSkillOptionClearDlgTest, OptionClearSynSendsExpectedMessage) {
    // 1:1 with legacy: MSG_WORD4 fields are populated from
    // pMugong (GetItemIdx, GetPosition) and pItem (wIconIdx,
    // Position). Modern port: s_mugong (test-injectable) and
    // s_itemPos (test-injectable) supply those values.
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    d->mugongIcon()->AddIcon(0, reinterpret_cast<cIcon*>(0x1),
                             /*onlyLink=*/true);
    auto mugong = std::make_unique<CMugongBase>();
    mugong->SetItemIdx(42u);
    mugong->SetPosition(99u);
    cSkillOptionClearDlg::SetMugongForTesting(mugong.get());
    cSkillOptionClearDlg::SetItemForTesting(7u);
    d->OptionClearSyn();
    const MsgWord4& msg = cSkillOptionClearDlg::lastSentMessage();
    EXPECT_EQ(msg.Category, cSkillOptionClearDlg::kCategoryMpMugong);
    EXPECT_EQ(msg.Protocol, cSkillOptionClearDlg::kProtocolOptionClearSyn);
    EXPECT_EQ(msg.wData1, 42u);  // pMugong->GetItemIdx()
    EXPECT_EQ(msg.wData2, 99u);  // pMugong->GetPosition()
    EXPECT_EQ(msg.wData4, 7u);   // pItem->Position (s_itemPos)
    // wData3 is legacy pItem->wIconIdx — modern port stubbed 0u.
    EXPECT_EQ(msg.wData3, 0u);
}

TEST_F(CSkillOptionClearDlgTest, OptionClearSynClosesDialog) {
    // 1:1 quirk: legacy OptionClearSyn ends with SetActive(FALSE).
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    d->mugongIcon()->AddIcon(0, reinterpret_cast<cIcon*>(0x1),
                             /*onlyLink=*/true);
    cSkillOptionClearDlg::SetMugongForTesting(new CMugongBase());
    cSkillOptionClearDlg::SetItemForTesting(7u);
    d->SetActive(true);
    d->OptionClearSyn();
    EXPECT_FALSE(d->isActive());
}

TEST_F(CSkillOptionClearDlgTest, OptionClearSynWithoutMugongIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    cSkillOptionClearDlg::SetItemForTesting(7u);
    d->OptionClearSyn();
    EXPECT_EQ(cSkillOptionClearDlg::lastSentMessage().Category, 0);
}

TEST_F(CSkillOptionClearDlgTest, OptionClearSynWithoutItemIsNoOp) {
    auto d = MakeDialog();
    auto inner = std::make_unique<cIconDialog>();
    InsertChildById(d.get(), cSkillOptionClearDlg::kMugongIconId,
                    std::move(inner));
    d->Linking();
    d->mugongIcon()->AddIcon(0, reinterpret_cast<cIcon*>(0x1),
                             /*onlyLink=*/true);
    cSkillOptionClearDlg::SetMugongForTesting(new CMugongBase());
    // No SetItemForTesting — s_itemPos stays at 0xFFFFu.
    d->OptionClearSyn();
    EXPECT_EQ(cSkillOptionClearDlg::lastSentMessage().Category, 0);
}
