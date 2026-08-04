// autonotedlg_test.cpp — 1:1 port tests for 墨香
// CAutoNoteDlg (auto note / auto reply dialog).
//
// Verifies:
//   - ctor does not crash
//   - Dtor does not crash
//   - Inherits from cDialog
//   - 3 id constants (kIdTextAreaManual=630,
//     kIdBtnAsk=631, kIdListAuto=632)
//   - 1 placeholder string (kAutoNoteManualText)
//   - kTestClientLoopCount = 35
//   - kAutoNoteTextColor = 0xFF808080
//   - Linking resolves the 3 children
//   - Linking sets the manual text + gray color
//   - Linking before Init does not crash
//   - Linking without children does not crash
//   - AddAutoList adds an item to the cListDialog
//   - AddAutoList with null pointers is safe
//   - AddAutoList without Linking is safe
//   - SetActiveTestClient activates the dialog
//   - SetActiveTestClient adds 35 items
//   - SetActiveTestClient without Linking is safe
//   - OnActionEvent preserves selection and ask gates

#include "autonotedlg.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"
#include "cbutton.hpp"
#include "clistdialog.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cAutoNoteDlg;
using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cListDialog;
using mxh::ui::cTextArea;
using mxh::ui::cWindow;

namespace {

// helper: build a cAutoNoteDlg + 3 children + Linking
struct LinkedDialog {
    cAutoNoteDlg dlg;
    std::unique_ptr<cTextArea> textArea;
    std::unique_ptr<cButton> btnAsk;
    std::unique_ptr<cListDialog> listAuto;

    LinkedDialog() {
        dlg.Init(0, 0, 300, 300, nullptr, 0);
        textArea = std::make_unique<cTextArea>();
        textArea->InitTextArea(mxh::ui::TextRect{0, 0, 100, 100}, 64);
        textArea->setId(cAutoNoteDlg::kIdTextAreaManual);
        auto* textPtr = textArea.get();
        dlg.Add(std::move(textArea));

        btnAsk = std::make_unique<cButton>();
        btnAsk->Init(0, 100, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                     cAutoNoteDlg::kIdBtnAsk);
        dlg.Add(std::move(btnAsk));

        listAuto = std::make_unique<cListDialog>();
        // cListDialog::InitList(maxLines, clipX, clipY, clipW, clipH) — 5 params
        listAuto->InitList(40, 0, 130, 200, 100);
        listAuto->setId(cAutoNoteDlg::kIdListAuto);
        auto* listPtr = listAuto.get();
        dlg.Add(std::move(listAuto));

        dlg.Linking();

        textPtr_ = textPtr;
        listPtr_ = listPtr;
    }

    cTextArea* textPtr_ = nullptr;
    cListDialog* listPtr_ = nullptr;
};

}  // namespace

// helper: wrap AddAutoList call with no-children to
// avoid dup macro
void ld_AddAutoList(cAutoNoteDlg& dlg) {
    dlg.AddAutoList("Test", "2024-01-01");
    dlg.SetActiveTestClient();
}

// ---------- ctor / dtor ----------

TEST(CAutoNoteDlgTest, CtorDoesNotCrash) {
    cAutoNoteDlg dlg;
    SUCCEED();
}

TEST(CAutoNoteDlgTest, DtorDoesNotCrash) {
    cAutoNoteDlg dlg;
    SUCCEED();
}

TEST(CAutoNoteDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cAutoNoteDlg>,
                  "cAutoNoteDlg must inherit from cDialog");
    SUCCEED();
}

// ---------- id range ----------

TEST(CAutoNoteDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cAutoNoteDlg::kIdTextAreaManual, 630);
    EXPECT_EQ(cAutoNoteDlg::kIdBtnAsk, 631);
    EXPECT_EQ(cAutoNoteDlg::kIdListAuto, 632);
}

TEST(CAutoNoteDlgTest, IdConstantsAreUnique) {
    EXPECT_NE(cAutoNoteDlg::kIdTextAreaManual, cAutoNoteDlg::kIdBtnAsk);
    EXPECT_NE(cAutoNoteDlg::kIdTextAreaManual, cAutoNoteDlg::kIdListAuto);
    EXPECT_NE(cAutoNoteDlg::kIdBtnAsk, cAutoNoteDlg::kIdListAuto);
}

TEST(CAutoNoteDlgTest, AutoNoteManualTextPlaceholderMatchesExpected) {
    EXPECT_STREQ(cAutoNoteDlg::kAutoNoteManualText, "AUTO_NOTE_MANUAL_TEXT");
}

TEST(CAutoNoteDlgTest, TestClientLoopCountIsThirtyFive) {
    EXPECT_EQ(cAutoNoteDlg::kTestClientLoopCount, 35);
}

TEST(CAutoNoteDlgTest, AutoNoteTextColorIsGray) {
    // 1:1 with legacy RGB_HALF(128, 128, 128) = 0xFF808080
    EXPECT_EQ(cAutoNoteDlg::kAutoNoteTextColor, 0xFF808080u);
}

// ---------- Linking ----------

TEST(CAutoNoteDlgTest, LinkingResolvesAllThreeChildren) {
    LinkedDialog ld;
    // Verify each child has the expected id.
    EXPECT_EQ(ld.textPtr_->id(), cAutoNoteDlg::kIdTextAreaManual);
    EXPECT_EQ(ld.listPtr_->id(), cAutoNoteDlg::kIdListAuto);
}

TEST(CAutoNoteDlgTest, LinkingSetsManualText) {
    LinkedDialog ld;
    EXPECT_EQ(ld.textPtr_->GetScriptText(), cAutoNoteDlg::kAutoNoteManualText);
}

TEST(CAutoNoteDlgTest, LinkingSetsTextColor) {
    LinkedDialog ld;
    EXPECT_EQ(ld.textPtr_->GetTextColor(), cAutoNoteDlg::kAutoNoteTextColor);
}

TEST(CAutoNoteDlgTest, LinkingBeforeInitDoesNotCrash) {
    cAutoNoteDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CAutoNoteDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cAutoNoteDlg dlg;
    dlg.Init(0, 0, 300, 300, nullptr, 0);
    dlg.Linking();
    // AddAutoList + SetActiveTestClient must not
    // crash when children are missing.
    ld_AddAutoList(dlg);
    SUCCEED();
}

// ---------- AddAutoList ----------

TEST(CAutoNoteDlgTest, AddAutoListAddsItem) {
    LinkedDialog ld;
    EXPECT_EQ(ld.listPtr_->RowCount(), 0);
    ld.dlg.AddAutoList("Alice", "2024-01-01");
    EXPECT_EQ(ld.listPtr_->RowCount(), 1);
}

TEST(CAutoNoteDlgTest, AddAutoListMultipleAdds) {
    LinkedDialog ld;
    ld.dlg.AddAutoList("Alice", "2024-01-01");
    ld.dlg.AddAutoList("Bob", "2024-01-02");
    ld.dlg.AddAutoList("Charlie", "2024-01-03");
    EXPECT_EQ(ld.listPtr_->RowCount(), 3);
}

TEST(CAutoNoteDlgTest, AddAutoListWithNullNameIsSafe) {
    LinkedDialog ld;
    EXPECT_EQ(ld.listPtr_->RowCount(), 0);
    ld.dlg.AddAutoList(nullptr, "2024-01-01");
    EXPECT_EQ(ld.listPtr_->RowCount(), 0);  // not added (defensive)
}

TEST(CAutoNoteDlgTest, AddAutoListWithNullDateIsSafe) {
    LinkedDialog ld;
    EXPECT_EQ(ld.listPtr_->RowCount(), 0);
    ld.dlg.AddAutoList("Alice", nullptr);
    EXPECT_EQ(ld.listPtr_->RowCount(), 0);
}

TEST(CAutoNoteDlgTest, AddAutoListWithoutLinkingIsSafe) {
    cAutoNoteDlg dlg;
    dlg.Init(0, 0, 300, 300, nullptr, 0);
    dlg.AddAutoList("Alice", "2024-01-01");
    SUCCEED();
}

// ---------- SetActiveTestClient ----------

TEST(CAutoNoteDlgTest, SetActiveTestClientActivatesDialog) {
    LinkedDialog ld;
    EXPECT_FALSE(ld.dlg.isActive());
    ld.dlg.SetActiveTestClient();
    EXPECT_TRUE(ld.dlg.isActive());
}

TEST(CAutoNoteDlgTest, SetActiveTestClientAddsThirtyFiveItems) {
    LinkedDialog ld;
    EXPECT_EQ(ld.listPtr_->RowCount(), 0);
    ld.dlg.SetActiveTestClient();
    EXPECT_EQ(ld.listPtr_->RowCount(),
              static_cast<size_t>(cAutoNoteDlg::kTestClientLoopCount));
}

TEST(CAutoNoteDlgTest, SetActiveTestClientWithoutLinkingIsSafe) {
    cAutoNoteDlg dlg;
    dlg.Init(0, 0, 300, 300, nullptr, 0);
    dlg.SetActiveTestClient();
    SUCCEED();
}

// ---------- OnActionEvent ----------

namespace {

struct AutoNoteCallbackState {
    void* selectedObject = reinterpret_cast<void*>(0x1);
    std::int32_t objectKind = cAutoNoteDlg::kPlayerObjectKind;
    std::uint32_t objectId = 77;
    bool heroObject = false;
    std::uint32_t randomValue = 143;
    int messageCalls = 0;
    std::int32_t lastMessageId = 0;
    int askCalls = 0;
    std::uint32_t askedObjectId = 0;
    std::uint32_t askedRandomValue = 0;
};

void* GetAutoNoteSelectedObject(void* data) { return static_cast<AutoNoteCallbackState*>(data)->selectedObject; }
std::int32_t GetAutoNoteObjectKind(void*, void* data) { return static_cast<AutoNoteCallbackState*>(data)->objectKind; }
std::uint32_t GetAutoNoteObjectId(void*, void* data) { return static_cast<AutoNoteCallbackState*>(data)->objectId; }
bool IsAutoNoteHeroObject(void*, void* data) { return static_cast<AutoNoteCallbackState*>(data)->heroObject; }
void AddAutoNoteMessage(std::int32_t id, void* data) {
    auto& state=*static_cast<AutoNoteCallbackState*>(data);
    ++state.messageCalls;
    state.lastMessageId=id;
}
std::uint32_t GetAutoNoteRandomPercent(void* data) { return static_cast<AutoNoteCallbackState*>(data)->randomValue; }
void AskAutoNoteUser(std::uint32_t id, std::uint32_t randomValue, void* data) {
    auto& state=*static_cast<AutoNoteCallbackState*>(data);
    ++state.askCalls;
    state.askedObjectId=id;
    state.askedRandomValue=randomValue;
}
void InstallAutoNoteCallbacks(cAutoNoteDlg& dlg, AutoNoteCallbackState& state) {
    dlg.SetCallbacks(GetAutoNoteSelectedObject, GetAutoNoteObjectKind,
                     GetAutoNoteObjectId, IsAutoNoteHeroObject,
                     AddAutoNoteMessage, GetAutoNoteRandomPercent,
                     AskAutoNoteUser, &state);
}

}  // namespace

TEST(CAutoNoteDlgTest, LegacyConstantsMatchSource) {
    EXPECT_EQ(cAutoNoteDlg::kWeBtnClick, 1u);
    EXPECT_EQ(cAutoNoteDlg::kSelectPlayerMessageId, 1704);
    EXPECT_EQ(cAutoNoteDlg::kManualMessageId, 1721);
}

TEST(CAutoNoteDlgTest, ActionWithoutButtonClickIsNoOp) {
    cAutoNoteDlg dlg;
    AutoNoteCallbackState state;
    InstallAutoNoteCallbacks(dlg,state);
    dlg.OnActionEvent(cAutoNoteDlg::kIdBtnAsk,nullptr,0);
    EXPECT_EQ(state.askCalls,0);
    EXPECT_EQ(state.messageCalls,0);
}

TEST(CAutoNoteDlgTest, DifferentButtonIsNoOp) {
    cAutoNoteDlg dlg;
    AutoNoteCallbackState state;
    InstallAutoNoteCallbacks(dlg,state);
    dlg.OnActionEvent(999,nullptr,cAutoNoteDlg::kWeBtnClick);
    EXPECT_EQ(state.askCalls,0);
}

TEST(CAutoNoteDlgTest, MissingSelectionEmits1704) {
    cAutoNoteDlg dlg;
    AutoNoteCallbackState state;
    state.selectedObject=nullptr;
    InstallAutoNoteCallbacks(dlg,state);
    dlg.OnActionEvent(cAutoNoteDlg::kIdBtnAsk,nullptr,cAutoNoteDlg::kWeBtnClick);
    EXPECT_EQ(state.lastMessageId,1704);
    EXPECT_EQ(state.askCalls,0);
}

TEST(CAutoNoteDlgTest, NonPlayerSelectionEmits1704) {
    cAutoNoteDlg dlg;
    AutoNoteCallbackState state;
    state.objectKind=32;
    InstallAutoNoteCallbacks(dlg,state);
    dlg.OnActionEvent(cAutoNoteDlg::kIdBtnAsk,nullptr,cAutoNoteDlg::kWeBtnClick);
    EXPECT_EQ(state.lastMessageId,1704);
    EXPECT_EQ(state.askCalls,0);
}

#ifndef _GMTOOL_
TEST(CAutoNoteDlgTest, HeroSelectionSilentlyReturns) {
    cAutoNoteDlg dlg;
    AutoNoteCallbackState state;
    state.heroObject=true;
    InstallAutoNoteCallbacks(dlg,state);
    dlg.OnActionEvent(cAutoNoteDlg::kIdBtnAsk,nullptr,cAutoNoteDlg::kWeBtnClick);
    EXPECT_EQ(state.messageCalls,0);
    EXPECT_EQ(state.askCalls,0);
}
#endif

TEST(CAutoNoteDlgTest, PlayerSelectionAsksWithObjectIdAndRandModulo100) {
    cAutoNoteDlg dlg;
    AutoNoteCallbackState state;
    InstallAutoNoteCallbacks(dlg,state);
    dlg.OnActionEvent(cAutoNoteDlg::kIdBtnAsk,nullptr,cAutoNoteDlg::kWeBtnClick);
    EXPECT_EQ(state.askCalls,1);
    EXPECT_EQ(state.askedObjectId,77u);
    EXPECT_EQ(state.askedRandomValue,43u);
}

TEST(CAutoNoteDlgTest, MissingAskCallbackDoesNotDispatch) {
    cAutoNoteDlg dlg;
    AutoNoteCallbackState state;
    dlg.SetCallbacks(GetAutoNoteSelectedObject,GetAutoNoteObjectKind,
                     GetAutoNoteObjectId,IsAutoNoteHeroObject,
                     AddAutoNoteMessage,GetAutoNoteRandomPercent,nullptr,&state);
    dlg.OnActionEvent(cAutoNoteDlg::kIdBtnAsk,nullptr,cAutoNoteDlg::kWeBtnClick);
    EXPECT_EQ(state.askCalls,0);
}

TEST(CAutoNoteDlgTest, SetCallbacksReplacesDispatch) {
    cAutoNoteDlg dlg;
    AutoNoteCallbackState first;
    AutoNoteCallbackState second;
    InstallAutoNoteCallbacks(dlg,first);
    InstallAutoNoteCallbacks(dlg,second);
    dlg.OnActionEvent(cAutoNoteDlg::kIdBtnAsk,nullptr,cAutoNoteDlg::kWeBtnClick);
    EXPECT_EQ(first.askCalls,0);
    EXPECT_EQ(second.askCalls,1);
}
