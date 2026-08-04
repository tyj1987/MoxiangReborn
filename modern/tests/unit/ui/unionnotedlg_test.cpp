// unionnotedlg_test.cpp — 1:1 port tests for
// 墨香 CUnionNoteDialog (guild union note sender
// dialog).
//
// Verifies:
//   - ctor does not crash
//   - Dtor does not crash
//   - Inherits from cDialog
//   - 4 id constants (kIdNoteText=620, kIdTitleEdit=621,
//     kIdSendOkBtn=622, kIdCancelBtn=623)
//   - Linking resolves the cTextArea
//   - Linking sets SetEnterAllow(false)
//   - Linking clears the script text
//   - Linking before Init does not crash
//   - Show stores the pItem + activates dialog
//   - Use clears m_pNoteText + m_bUse + m_pItem
//   - OnActionEvent sends union notes and closes
//   - IsUse returns m_bUse

#include "unionnotedlg.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"
#include "ceditbox.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cEditBox;
using mxh::ui::cTextArea;
using mxh::ui::cUnionNoteDlg;

namespace {

// helper: build a cUnionNoteDlg + 1 cTextArea + Linking
struct LinkedDialog {
    cUnionNoteDlg dlg;
    std::unique_ptr<cTextArea> noteText;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        noteText = std::make_unique<cTextArea>();
        noteText->InitTextArea(mxh::ui::TextRect{0, 0, 100, 100}, 64);
        noteText->setId(cUnionNoteDlg::kIdNoteText);
        auto* notePtr = noteText.get();
        dlg.Add(std::move(noteText));

        dlg.Linking();

        notePtr_ = notePtr;
    }

    cTextArea* notePtr_ = nullptr;
};

}  // namespace

// ---------- ctor / dtor ----------

TEST(CUnionNoteDlgTest, CtorDoesNotCrash) {
    cUnionNoteDlg dlg;
    SUCCEED();
}

TEST(CUnionNoteDlgTest, DtorDoesNotCrash) {
    cUnionNoteDlg dlg;
    SUCCEED();
}

TEST(CUnionNoteDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cUnionNoteDlg>,
                  "cUnionNoteDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CUnionNoteDlgTest, DefaultBUseIsFalse) {
    cUnionNoteDlg dlg;
    EXPECT_FALSE(dlg.IsUse());
}

// ---------- id range ----------

TEST(CUnionNoteDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cUnionNoteDlg::kIdNoteText, 620);
    EXPECT_EQ(cUnionNoteDlg::kIdTitleEdit, 621);
    EXPECT_EQ(cUnionNoteDlg::kIdSendOkBtn, 622);
    EXPECT_EQ(cUnionNoteDlg::kIdCancelBtn, 623);
}

TEST(CUnionNoteDlgTest, IdConstantsAreUnique) {
    EXPECT_NE(cUnionNoteDlg::kIdNoteText, cUnionNoteDlg::kIdTitleEdit);
    EXPECT_NE(cUnionNoteDlg::kIdNoteText, cUnionNoteDlg::kIdSendOkBtn);
    EXPECT_NE(cUnionNoteDlg::kIdNoteText, cUnionNoteDlg::kIdCancelBtn);
    EXPECT_NE(cUnionNoteDlg::kIdTitleEdit, cUnionNoteDlg::kIdSendOkBtn);
    EXPECT_NE(cUnionNoteDlg::kIdTitleEdit, cUnionNoteDlg::kIdCancelBtn);
    EXPECT_NE(cUnionNoteDlg::kIdSendOkBtn, cUnionNoteDlg::kIdCancelBtn);
}

// ---------- Linking ----------

TEST(CUnionNoteDlgTest, LinkingResolvesNoteText) {
    LinkedDialog ld;
    // m_pNoteText is private; verify by setting
    // script text via the dialog's child.
    EXPECT_NE(ld.notePtr_, nullptr);
    EXPECT_EQ(ld.notePtr_->GetScriptText(), "");
}

TEST(CUnionNoteDlgTest, LinkingDisablesEnterAllow) {
    LinkedDialog ld;
    // 1:1 with legacy SetEnterAllow(FALSE).
    EXPECT_FALSE(ld.notePtr_->IsEnterAllow());
}

TEST(CUnionNoteDlgTest, LinkingClearsScriptText) {
    LinkedDialog ld;
    // 1:1 with legacy SetScriptText("").
    EXPECT_EQ(ld.notePtr_->GetScriptText(), "");
}

TEST(CUnionNoteDlgTest, LinkingBeforeInitDoesNotCrash) {
    cUnionNoteDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CUnionNoteDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cUnionNoteDlg dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

// ---------- Show / Use / OnActionEvent ----------

namespace {

struct UnionNoteState {
    std::uint32_t guildIdx = 1;
    std::int32_t rank = cUnionNoteDlg::kGuildMaster;
    std::uint32_t unionIdx = 9;
    std::uint32_t heroObjectId = 77;
    std::string heroName = "Hero";
    std::uint16_t itemIdx = 321;
    std::uint32_t itemPosition = 1234;
    int messageCalls = 0;
    std::int32_t lastMessageId = 0;
    int itemUseCalls = 0;
    std::uint32_t usedObjectId = 0;
    std::uint16_t usedItemIdx = 0;
    std::uint32_t usedPosition = 0;
    int itemUseCount = 0;
    int noteCalls = 0;
    std::uint32_t noteObjectId = 0;
    std::uint32_t noteUnionId = 0;
    std::string noteFromName;
    std::string noteText;
};

void AddUnionNoteMessage(std::int32_t id, void* data) {
    auto& state = *static_cast<UnionNoteState*>(data);
    ++state.messageCalls;
    state.lastMessageId = id;
}
std::uint32_t GetUnionNoteGuildIdx(void* data) { return static_cast<UnionNoteState*>(data)->guildIdx; }
std::int32_t GetUnionNoteRank(void* data) { return static_cast<UnionNoteState*>(data)->rank; }
std::uint32_t GetUnionNoteUnionIdx(void* data) { return static_cast<UnionNoteState*>(data)->unionIdx; }
std::uint32_t GetUnionNoteHeroId(void* data) { return static_cast<UnionNoteState*>(data)->heroObjectId; }
const char* GetUnionNoteHeroName(void* data) { return static_cast<UnionNoteState*>(data)->heroName.c_str(); }
std::uint16_t GetUnionNoteItemIdx(void*, void* data) { return static_cast<UnionNoteState*>(data)->itemIdx; }
std::uint32_t GetUnionNoteItemPosition(void*, void* data) { return static_cast<UnionNoteState*>(data)->itemPosition; }
void SendUnionNoteItemUse(std::uint32_t objectId, std::uint16_t itemIdx,
                          std::uint32_t position, void* data) {
    auto& state = *static_cast<UnionNoteState*>(data);
    ++state.itemUseCalls;
    state.usedObjectId = objectId;
    state.usedItemIdx = itemIdx;
    state.usedPosition = position;
}
void SendUnionNoteMessage(std::uint32_t objectId, std::uint32_t unionId,
                          const char* fromName, const char* note, void* data) {
    auto& state = *static_cast<UnionNoteState*>(data);
    ++state.noteCalls;
    state.noteObjectId = objectId;
    state.noteUnionId = unionId;
    state.noteFromName = fromName ? fromName : "";
    state.noteText = note ? note : "";
}
void IncrementUnionNoteItemUseCount(void* data) {
    ++static_cast<UnionNoteState*>(data)->itemUseCount;
}
void InstallUnionNoteCallbacks(cUnionNoteDlg& dlg, UnionNoteState& state) {
    dlg.SetCallbacks(AddUnionNoteMessage, GetUnionNoteGuildIdx,
                     GetUnionNoteRank, GetUnionNoteUnionIdx,
                     GetUnionNoteHeroId, GetUnionNoteHeroName,
                     GetUnionNoteItemIdx, GetUnionNoteItemPosition,
                     SendUnionNoteItemUse, SendUnionNoteMessage,
                     IncrementUnionNoteItemUseCount, &state);
}

}  // namespace

TEST(CUnionNoteDlgTest, LegacyConstantsMatchSource) {
    EXPECT_EQ(cUnionNoteDlg::kGuildMaster, 50);
    EXPECT_EQ(cUnionNoteDlg::kGuildViceMaster, 40);
    EXPECT_EQ(cUnionNoteDlg::kNoGuildMessageId, 35);
    EXPECT_EQ(cUnionNoteDlg::kInvalidRankMessageId, 1100);
    EXPECT_EQ(cUnionNoteDlg::kNoUnionMessageId, 1103);
    EXPECT_EQ(cUnionNoteDlg::kInvalidItemMessageId, 786);
    EXPECT_EQ(cUnionNoteDlg::kAlreadyUsingMessageId, 752);
    EXPECT_EQ(cUnionNoteDlg::kWeBtnClick, 1u);
}

TEST(CUnionNoteDlgTest, ShowWithoutGuildEmits35) {
    cUnionNoteDlg dlg;
    UnionNoteState state;
    state.guildIdx = 0;
    InstallUnionNoteCallbacks(dlg, state);
    int item = 1;
    dlg.Show(&item);
    EXPECT_EQ(state.lastMessageId, 35);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CUnionNoteDlgTest, ShowInvalidRankEmits1100) {
    cUnionNoteDlg dlg;
    UnionNoteState state;
    state.rank = 30;
    InstallUnionNoteCallbacks(dlg, state);
    int item = 1;
    dlg.Show(&item);
    EXPECT_EQ(state.lastMessageId, 1100);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CUnionNoteDlgTest, ShowViceMasterIsAllowed) {
    cUnionNoteDlg dlg;
    UnionNoteState state;
    state.rank = cUnionNoteDlg::kGuildViceMaster;
    InstallUnionNoteCallbacks(dlg, state);
    int item = 1;
    dlg.Show(&item);
    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(state.messageCalls, 0);
}

TEST(CUnionNoteDlgTest, ShowWithoutUnionEmits1103) {
    cUnionNoteDlg dlg;
    UnionNoteState state;
    state.unionIdx = 0;
    InstallUnionNoteCallbacks(dlg, state);
    int item = 1;
    dlg.Show(&item);
    EXPECT_EQ(state.lastMessageId, 1103);
}

TEST(CUnionNoteDlgTest, ShowWithNullItemEmits786) {
    cUnionNoteDlg dlg;
    UnionNoteState state;
    InstallUnionNoteCallbacks(dlg, state);
    dlg.Show(nullptr);
    EXPECT_EQ(state.lastMessageId, 786);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CUnionNoteDlgTest, ShowWithValidContextActivates) {
    cUnionNoteDlg dlg;
    UnionNoteState state;
    InstallUnionNoteCallbacks(dlg, state);
    int item = 1;
    dlg.Show(&item);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CUnionNoteDlgTest, UseClearsTextAndSendsItemFields) {
    LinkedDialog linked;
    UnionNoteState state;
    InstallUnionNoteCallbacks(linked.dlg, state);
    int item = 1;
    linked.dlg.Show(&item);
    linked.notePtr_->SetScriptText("note");

    linked.dlg.Use();

    EXPECT_FALSE(linked.dlg.IsUse());
    EXPECT_TRUE(linked.notePtr_->GetScriptText().empty());
    EXPECT_EQ(state.itemUseCalls, 1);
    EXPECT_EQ(state.usedObjectId, 77u);
    EXPECT_EQ(state.usedItemIdx, 321u);
    EXPECT_EQ(state.usedPosition, 1234u);
    EXPECT_EQ(state.itemUseCount, 1);
}

TEST(CUnionNoteDlgTest, UseWithoutItemDoesNotSendOrIncrement) {
    LinkedDialog linked;
    UnionNoteState state;
    InstallUnionNoteCallbacks(linked.dlg, state);
    linked.dlg.Use();
    EXPECT_EQ(state.itemUseCalls, 0);
    EXPECT_EQ(state.itemUseCount, 0);
}

TEST(CUnionNoteDlgTest, SendButtonSendsNoteFieldsAndCloses) {
    LinkedDialog linked;
    UnionNoteState state;
    InstallUnionNoteCallbacks(linked.dlg, state);
    linked.notePtr_->SetScriptText("union announcement");
    linked.dlg.SetActive(true);

    linked.dlg.OnActionEvent(cUnionNoteDlg::kIdSendOkBtn, nullptr,
                             cUnionNoteDlg::kWeBtnClick);

    EXPECT_EQ(state.noteCalls, 1);
    EXPECT_EQ(state.noteObjectId, 77u);
    EXPECT_EQ(state.noteUnionId, 9u);
    EXPECT_EQ(state.noteFromName, "Hero");
    EXPECT_EQ(state.noteText, "union announcement");
    EXPECT_FALSE(linked.dlg.isActive());
}

TEST(CUnionNoteDlgTest, SendButtonWithoutHostStillCloses) {
    LinkedDialog linked;
    linked.dlg.SetActive(true);
    linked.dlg.OnActionEvent(cUnionNoteDlg::kIdSendOkBtn, nullptr,
                             cUnionNoteDlg::kWeBtnClick);
    EXPECT_FALSE(linked.dlg.isActive());
}

TEST(CUnionNoteDlgTest, CancelButtonClosesWithoutSend) {
    LinkedDialog linked;
    UnionNoteState state;
    InstallUnionNoteCallbacks(linked.dlg, state);
    linked.dlg.SetActive(true);
    linked.dlg.OnActionEvent(cUnionNoteDlg::kIdCancelBtn, nullptr,
                             cUnionNoteDlg::kWeBtnClick);
    EXPECT_FALSE(linked.dlg.isActive());
    EXPECT_EQ(state.noteCalls, 0);
}

TEST(CUnionNoteDlgTest, ActionWithoutButtonClickIsNoOp) {
    LinkedDialog linked;
    UnionNoteState state;
    InstallUnionNoteCallbacks(linked.dlg, state);
    linked.dlg.SetActive(true);
    linked.dlg.OnActionEvent(cUnionNoteDlg::kIdSendOkBtn, nullptr, 0);
    EXPECT_TRUE(linked.dlg.isActive());
    EXPECT_EQ(state.noteCalls, 0);
}

TEST(CUnionNoteDlgTest, SetCallbacksReplacesDispatch) {
    LinkedDialog linked;
    UnionNoteState first;
    UnionNoteState second;
    InstallUnionNoteCallbacks(linked.dlg, first);
    InstallUnionNoteCallbacks(linked.dlg, second);
    linked.notePtr_->SetScriptText("note");
    linked.dlg.OnActionEvent(cUnionNoteDlg::kIdSendOkBtn, nullptr,
                             cUnionNoteDlg::kWeBtnClick);
    EXPECT_EQ(first.noteCalls, 0);
    EXPECT_EQ(second.noteCalls, 1);
}
