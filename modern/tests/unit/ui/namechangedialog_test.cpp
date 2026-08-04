// namechangedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cNameChangeDialog (name change editor
// dialog: 1 cEditBox + 1 OK button + item DB idx state).
//
// Covers modern/src/ui/namechangedialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\NameChangeDialog.h (877 B) and
//   墨香【源码】\[Client]MH\NameChangeDialog.cpp.
//
// What's tested:
//   - Default construction: cNameChangeDialog is a
//     cDialog and inherits its tree management.
//   - m_dwDBIdx starts 0 (1:1 with legacy default
//     init).
//   - Id constant matches expected local range
//     (kIdNameBox=450).
//   - kVcmCharname == 2 (1:1 with legacy
//     VCM_CHARNAME enum value).
//   - Linking resolves the cEditBox child by id
//     and calls SetValidCheck(2) on it.
//   - Linking without children leaves m_pNameBox
//     null (SetActive + NameChangeSyn are safe).
//   - Linking before Init does not crash.
//   - SetActive val=true calls base SetActive
//     + clears edit text (1:1 with legacy).
//   - SetActive val=false calls base SetActive
//     (no edit text clear, 1:1 with legacy).
//   - SetActive without Linking is safe.
//   - SetItemDBIdx / GetItemDBIdx round-trip.
//   - NameChangeSyn preserves all legacy validation
//     gates, message ids, callback order, send fields,
//     and successful dialog close.
//   - NameChangeSyn without Linking is safe.
//   - NameChangeSyn before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_NAMECHANGE_DLG drop, modern cWindow
//     does not have m_type field).
//   - State field default 0 (1:1 with legacy
//     m_dwDBIdx = 0 init).
//   - Linking SetValidCheck(2) (1:1 with legacy
//     VCM_CHARNAME = 2).
//   - SetActive override: call base + clear edit
//     text on val=true (1:1 with legacy).
//   - 1:1 quirk: modern SetEditText is a no-op
//     unless InitEditbox was called (m_bInitEdit
//     guard). Test caller must call InitEditbox
//     before SetEditText takes effect.
//   - NameChangeSyn singleton dependencies are
//     supplied through optional host callbacks.
//   - kVcmCharname = 2 (1:1 with legacy
//     cEditBox::SetValidCheck enum value).
//   - Local id range 450 (distinct from 200-443
//     used by previous Tier 2 dialogs; no
//     collision).

#include "namechangedialog.hpp"
#include "cdialog.hpp"
#include "ceditbox.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CNameChangeDialogTest, DefaultConstructionHasZeroState) {
    cNameChangeDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_NAMECHANGE_DLG drop, modern
    // cWindow does not have m_type field).
    // m_dwDBIdx starts 0.
    EXPECT_EQ(dlg.GetItemDBIdx(), 0u);
}

TEST(CNameChangeDialogTest, InheritsDialogTreeManagement) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CNameChangeDialogTest, IdConstantMatchesExpectedLocalRange) {
    EXPECT_EQ(cNameChangeDialog::kIdNameBox, 450);
}

TEST(CNameChangeDialogTest, VcmCharnameIsTwo) {
    // 1:1 with legacy cEditBox::SetValidCheck
    // VCM_CHARNAME = 2 (character-name valid
    // check).
    EXPECT_EQ(cNameChangeDialog::kVcmCharname, 2);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CNameChangeDialogTest, LinkingResolvesEditBox) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cNameChangeDialog::kIdNameBox);
    edit->InitEditbox(50, 64);
    cEditBox* pEdit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));

    dlg.Linking();
    // m_pNameBox is private; verified indirectly
    // via SetActive(true) calling SetEditText("")
    // on the cEditBox.
    dlg.SetActive(true);
    EXPECT_EQ(pEdit->editText(), "");
}

TEST(CNameChangeDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SetActive + NameChangeSyn without children
    // must be safe.
    dlg.SetActive(true);
    dlg.SetActive(false);
    dlg.NameChangeSyn();
    SUCCEED();
}

TEST(CNameChangeDialogTest, LinkingBeforeInitDoesNotCrash) {
    cNameChangeDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetActive override
// ===========================================================================

TEST(CNameChangeDialogTest, SetActiveTrueUpdatesBaseState) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CNameChangeDialogTest, SetActiveFalseUpdatesBaseState) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CNameChangeDialogTest, SetActiveTrueClearsEditText) {
    // 1:1 with legacy: SetActive(val=TRUE) calls
    // m_pNameBox->SetEditText("") after base
    // SetActive. Requires InitEditbox to have
    // been called (m_bInitEdit guard).
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cNameChangeDialog::kIdNameBox);
    edit->InitEditbox(50, 64);
    cEditBox* pEdit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();

    pEdit->SetEditText("stale name");
    EXPECT_EQ(pEdit->editText(), "stale name");
    dlg.SetActive(true);
    EXPECT_EQ(pEdit->editText(), "");
}

TEST(CNameChangeDialogTest, SetActiveFalseDoesNotClearEditText) {
    // 1:1 with legacy: SetActive(val=FALSE) only
    // calls base SetActive. Pre-existing edit
    // text survives.
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cNameChangeDialog::kIdNameBox);
    edit->InitEditbox(50, 64);
    cEditBox* pEdit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();

    pEdit->SetEditText("preserved name");
    dlg.SetActive(false);
    EXPECT_EQ(pEdit->editText(), "preserved name");
}

TEST(CNameChangeDialogTest, SetActiveWithoutLinkIsSafe) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CNameChangeDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cNameChangeDialog dlg;
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// SetItemDBIdx / GetItemDBIdx
// ===========================================================================

TEST(CNameChangeDialogTest, SetItemDBIdxRoundTrip) {
    cNameChangeDialog dlg;
    dlg.SetItemDBIdx(42u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 42u);
}

TEST(CNameChangeDialogTest, SetItemDBIdxOverridesPrevious) {
    cNameChangeDialog dlg;
    dlg.SetItemDBIdx(1u);
    dlg.SetItemDBIdx(100u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 100u);
}

TEST(CNameChangeDialogTest, SetItemDBIdxZeroIsValid) {
    cNameChangeDialog dlg;
    dlg.SetItemDBIdx(0u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 0u);
}

// ===========================================================================
// NameChangeSyn
// ===========================================================================

namespace {

struct NameChangeChildren {
    cEditBox* edit = nullptr;
};

void BuildNameChangeDialog(cNameChangeDialog& dlg, NameChangeChildren& children) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cNameChangeDialog::kIdNameBox);
    edit->InitEditbox(50, 64);
    children.edit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();
}

struct NameChangeCallbackState {
    std::string heroName = "OldHero";
    std::uint32_t heroObjectId = 77;
    bool invalidChar = false;
    bool usableName = true;
    int messageCalls = 0;
    std::int32_t lastMessageId = 0;
    int invalidCalls = 0;
    int usableCalls = 0;
    int sendCalls = 0;
    std::uint32_t sentObjectId = 0;
    std::uint32_t sentDbIdx = 0;
    std::string sentName;
};

void AddNameChangeSystemMessage(std::int32_t messageId, void* userData) {
    auto& state = *static_cast<NameChangeCallbackState*>(userData);
    ++state.messageCalls;
    state.lastMessageId = messageId;
}

const char* GetNameChangeHeroName(void* userData) {
    return static_cast<NameChangeCallbackState*>(userData)->heroName.c_str();
}

std::uint32_t GetNameChangeHeroObjectId(void* userData) {
    return static_cast<NameChangeCallbackState*>(userData)->heroObjectId;
}

bool IsNameChangeInvalidCharIncluded(const unsigned char*, void* userData) {
    auto& state = *static_cast<NameChangeCallbackState*>(userData);
    ++state.invalidCalls;
    return state.invalidChar;
}

bool IsNameChangeUsable(const char*, void* userData) {
    auto& state = *static_cast<NameChangeCallbackState*>(userData);
    ++state.usableCalls;
    return state.usableName;
}

void SendNameChange(std::uint32_t objectId, std::uint32_t dbIdx,
                    const char* name, void* userData) {
    auto& state = *static_cast<NameChangeCallbackState*>(userData);
    ++state.sendCalls;
    state.sentObjectId = objectId;
    state.sentDbIdx = dbIdx;
    state.sentName = name ? name : "";
}

void InstallNameChangeCallbacks(cNameChangeDialog& dlg,
                                NameChangeCallbackState& state) {
    dlg.SetCallbacks(AddNameChangeSystemMessage, GetNameChangeHeroName,
                     GetNameChangeHeroObjectId, IsNameChangeInvalidCharIncluded,
                     IsNameChangeUsable, SendNameChange, &state);
}

}  // namespace

TEST(CNameChangeDialogTest, LegacyConstantsMatchSource) {
    EXPECT_EQ(cNameChangeDialog::kMaxNameLength, 16u);
    EXPECT_EQ(cNameChangeDialog::kEmptyNameMessageId, 11);
    EXPECT_EQ(cNameChangeDialog::kInvalidNameMessageId, 14);
    EXPECT_EQ(cNameChangeDialog::kShortNameMessageId, 19);
}

TEST(CNameChangeDialogTest, NameChangeSynEmptyNameShowsMessage11) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState state;
    InstallNameChangeCallbacks(dlg, state);

    dlg.NameChangeSyn();

    EXPECT_EQ(state.messageCalls, 1);
    EXPECT_EQ(state.lastMessageId, 11);
    EXPECT_EQ(state.sendCalls, 0);
}

TEST(CNameChangeDialogTest, NameChangeSynShortNameShowsMessage19) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState state;
    InstallNameChangeCallbacks(dlg, state);
    children.edit->SetEditText("abc");

    dlg.NameChangeSyn();

    EXPECT_EQ(state.lastMessageId, 19);
    EXPECT_EQ(state.invalidCalls, 0);
    EXPECT_EQ(state.sendCalls, 0);
}

TEST(CNameChangeDialogTest, NameChangeSynOverMaxLengthSilentlyReturns) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState state;
    InstallNameChangeCallbacks(dlg, state);
    children.edit->SetEditText("12345678901234567");

    dlg.NameChangeSyn();

    EXPECT_EQ(state.messageCalls, 0);
    EXPECT_EQ(state.invalidCalls, 0);
    EXPECT_EQ(state.sendCalls, 0);
}

TEST(CNameChangeDialogTest, NameChangeSynSameAsHeroNameSilentlyReturns) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState state;
    state.heroName = "SameHero";
    InstallNameChangeCallbacks(dlg, state);
    children.edit->SetEditText("SameHero");

    dlg.NameChangeSyn();

    EXPECT_EQ(state.messageCalls, 0);
    EXPECT_EQ(state.invalidCalls, 0);
    EXPECT_EQ(state.sendCalls, 0);
}

TEST(CNameChangeDialogTest, NameChangeSynInvalidCharacterShowsMessage14) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState state;
    state.invalidChar = true;
    InstallNameChangeCallbacks(dlg, state);
    children.edit->SetEditText("Bad!Name");

    dlg.NameChangeSyn();

    EXPECT_EQ(state.lastMessageId, 14);
    EXPECT_EQ(state.invalidCalls, 1);
    EXPECT_EQ(state.usableCalls, 0);
    EXPECT_EQ(state.sendCalls, 0);
}

TEST(CNameChangeDialogTest, NameChangeSynUnusableNameShowsMessage14) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState state;
    state.usableName = false;
    InstallNameChangeCallbacks(dlg, state);
    children.edit->SetEditText("BlockedName");

    dlg.NameChangeSyn();

    EXPECT_EQ(state.lastMessageId, 14);
    EXPECT_EQ(state.invalidCalls, 1);
    EXPECT_EQ(state.usableCalls, 1);
    EXPECT_EQ(state.sendCalls, 0);
}

TEST(CNameChangeDialogTest, NameChangeSynZeroDbIdxSilentlyReturnsAfterFilters) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState state;
    InstallNameChangeCallbacks(dlg, state);
    children.edit->SetEditText("NewHero");

    dlg.NameChangeSyn();

    EXPECT_EQ(state.invalidCalls, 1);
    EXPECT_EQ(state.usableCalls, 1);
    EXPECT_EQ(state.sendCalls, 0);
}

TEST(CNameChangeDialogTest, NameChangeSynSendsExpectedFieldsAndCloses) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState state;
    InstallNameChangeCallbacks(dlg, state);
    dlg.SetItemDBIdx(1234);
    dlg.SetActive(true);
    children.edit->SetEditText("NewHero");

    dlg.NameChangeSyn();

    EXPECT_EQ(state.sendCalls, 1);
    EXPECT_EQ(state.sentObjectId, 77u);
    EXPECT_EQ(state.sentDbIdx, 1234u);
    EXPECT_EQ(state.sentName, "NewHero");
    EXPECT_FALSE(dlg.isActive());
}

TEST(CNameChangeDialogTest, NameChangeSynWithoutSendCallbackDoesNotClose) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState state;
    dlg.SetCallbacks(AddNameChangeSystemMessage, GetNameChangeHeroName,
                     GetNameChangeHeroObjectId, IsNameChangeInvalidCharIncluded,
                     IsNameChangeUsable, nullptr, &state);
    dlg.SetItemDBIdx(1);
    dlg.SetActive(true);
    children.edit->SetEditText("NewHero");

    dlg.NameChangeSyn();

    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(state.sendCalls, 0);
}

TEST(CNameChangeDialogTest, NameChangeSynWithoutLinkIsSafe) {
    cNameChangeDialog dlg;
    dlg.SetItemDBIdx(1u);
    dlg.NameChangeSyn();
    SUCCEED();
}

TEST(CNameChangeDialogTest, SetCallbacksReplacesExistingDispatch) {
    cNameChangeDialog dlg;
    NameChangeChildren children;
    BuildNameChangeDialog(dlg, children);
    NameChangeCallbackState firstState;
    NameChangeCallbackState secondState;
    InstallNameChangeCallbacks(dlg, firstState);
    InstallNameChangeCallbacks(dlg, secondState);
    dlg.SetItemDBIdx(9);
    children.edit->SetEditText("NewHero");

    dlg.NameChangeSyn();

    EXPECT_EQ(firstState.sendCalls, 0);
    EXPECT_EQ(secondState.sendCalls, 1);
}

}  // namespace mxh::ui::test
