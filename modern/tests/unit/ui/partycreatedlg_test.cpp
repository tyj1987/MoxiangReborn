#include "partycreatedlg.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cCheckBox.hpp"
#include "mxh/ui/cComboBox.hpp"
#include "mxh/ui/cListItem.hpp"
#include "mxh/ui/cEditBox.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cCheckBox;
using mxh::ui::cComboBox;
using mxh::ui::cDialog;
using mxh::ui::cEditBox;
using mxh::ui::cPartyCreateDlg;
using mxh::ui::PartyCreateOptions;
using mxh::ui::PartyDivisionOption;

namespace {

struct Controls {
    cEditBox theme;
    cEditBox minLevel;
    cEditBox maxLevel;
    cCheckBox publicChk;
    cCheckBox privateChk;
    cComboBox distribute;
    cComboBox memberNum;
    cButton ok;
    cButton cancel;

    Controls() {
        theme.InitEditbox(64, 32);
        minLevel.InitEditbox(32, 8);
        maxLevel.InitEditbox(32, 8);
    }

    void Attach(cPartyCreateDlg& d) {
        d.SetControlsForTest(&theme, &minLevel, &maxLevel, &publicChk,
                             &privateChk, &distribute, &memberNum,
                             &ok, &cancel);
    }
};

struct CreateCapture {
    int calls = 0;
    bool returnValue = true;
    PartyCreateOptions lastOpts{};
};

bool CaptureCreateSyn(const PartyCreateOptions& opts, void* userData) {
    auto* cap = static_cast<CreateCapture*>(userData);
    ++cap->calls;
    cap->lastOpts = opts;
    return cap->returnValue;
}

const char* StubChatMsg(std::int32_t messageId, void* /*userData*/) {
    return messageId == cPartyCreateDlg::kChatPartyNameTooLong
        ? "Theme too long" : nullptr;
}

const char* StubResourceMsg(std::int32_t messageId, void* /*userData*/) {
    if (messageId == cPartyCreateDlg::kResourceRandomOption) return "Random";
    if (messageId == cPartyCreateDlg::kResourceDamageOption) return "Damage";
    return nullptr;
}

bool AlwaysNoParty(void* /*userData*/) { return false; }
bool AlwaysHasParty(void* /*userData*/) { return true; }

} // namespace

TEST(PartyCreateDlgTest, InheritsDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<cDialog, cPartyCreateDlg>);
    static_assert(!std::is_copy_constructible_v<cPartyCreateDlg>);
    static_assert(!std::is_copy_assignable_v<cPartyCreateDlg>);
    SUCCEED();
}

TEST(PartyCreateDlgTest, ConstantsMatchLegacyWindowIds) {
    EXPECT_EQ(cPartyCreateDlg::kThemeEditId, 391);
    EXPECT_EQ(cPartyCreateDlg::kMinLevelEditId, 392);
    EXPECT_EQ(cPartyCreateDlg::kMaxLevelEditId, 393);
    EXPECT_EQ(cPartyCreateDlg::kPublicCheckId, 394);
    EXPECT_EQ(cPartyCreateDlg::kPrivateCheckId, 395);
    EXPECT_EQ(cPartyCreateDlg::kDistributeComboId, 396);
    EXPECT_EQ(cPartyCreateDlg::kMemberNumComboId, 397);
    EXPECT_EQ(cPartyCreateDlg::kOkButtonId, 398);
    EXPECT_EQ(cPartyCreateDlg::kCancelButtonId, 399);
    EXPECT_EQ(cPartyCreateDlg::kMaxPartyNameLength, 15);
    EXPECT_EQ(cPartyCreateDlg::kDefaultMinLevel, 1);
    EXPECT_EQ(cPartyCreateDlg::kDefaultMaxLevel, 99);
}

TEST(PartyCreateDlgTest, ConstructorDefaultsAreCorrect) {
    cPartyCreateDlg dialog;
    EXPECT_FALSE(dialog.isProcessing());
    EXPECT_EQ(dialog.themeEdit(), nullptr);
    EXPECT_EQ(dialog.publicCheck(), nullptr);
    EXPECT_EQ(dialog.distributeCombo(), nullptr);
    EXPECT_EQ(dialog.okButton(), nullptr);
    EXPECT_FALSE(dialog.isActive());
}

TEST(PartyCreateDlgTest, SetControlsForTestAssignsAll) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    EXPECT_EQ(dialog.themeEdit(), &controls.theme);
    EXPECT_EQ(dialog.minLevelEdit(), &controls.minLevel);
    EXPECT_EQ(dialog.maxLevelEdit(), &controls.maxLevel);
    EXPECT_EQ(dialog.publicCheck(), &controls.publicChk);
    EXPECT_EQ(dialog.privateCheck(), &controls.privateChk);
    EXPECT_EQ(dialog.distributeCombo(), &controls.distribute);
    EXPECT_EQ(dialog.memberNumCombo(), &controls.memberNum);
    EXPECT_EQ(dialog.okButton(), &controls.ok);
    EXPECT_EQ(dialog.cancelButton(), &controls.cancel);
}

TEST(PartyCreateDlgTest, InitOptionAppliesDefaults) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.InitOption();
    EXPECT_EQ(controls.theme.editText(), "");
    EXPECT_EQ(controls.minLevel.editText(), "1");
    EXPECT_EQ(controls.maxLevel.editText(), "99");
    EXPECT_TRUE(controls.publicChk.IsChecked());
    EXPECT_FALSE(controls.privateChk.IsChecked());
}

TEST(PartyCreateDlgTest, InitOptionToleratesNullControls) {
    cPartyCreateDlg dialog;
    EXPECT_NO_FATAL_FAILURE(dialog.InitOption());
}

TEST(PartyCreateDlgTest, OnActionEventPublicUnchecksPrivate) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.privateChk.SetChecked(true);
    dialog.OnActionEvent(cPartyCreateDlg::kPublicCheckId, nullptr,
                         cPartyCreateDlg::kActionBtnClick);
    EXPECT_FALSE(controls.privateChk.IsChecked());
    // Public is unchanged by the click event (legacy parity: only the
    // opposing checkbox is cleared).
}

TEST(PartyCreateDlgTest, OnActionEventPrivateUnchecksPublic) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.publicChk.SetChecked(true);
    dialog.OnActionEvent(cPartyCreateDlg::kPrivateCheckId, nullptr,
                         cPartyCreateDlg::kActionBtnClick);
    EXPECT_FALSE(controls.publicChk.IsChecked());
}

TEST(PartyCreateDlgTest, OnActionEventCancelDeactivates) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.SetActive(true);
    EXPECT_TRUE(dialog.isActive());
    dialog.OnActionEvent(cPartyCreateDlg::kCancelButtonId, nullptr,
                         cPartyCreateDlg::kActionBtnClick);
    EXPECT_FALSE(dialog.isActive());
}

TEST(PartyCreateDlgTest, OnActionEventIgnoresNonClickFlags) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.publicChk.SetChecked(true);
    dialog.SetActive(true);
    dialog.OnActionEvent(cPartyCreateDlg::kCancelButtonId, nullptr, 0);
    EXPECT_TRUE(dialog.isActive());
    EXPECT_TRUE(controls.publicChk.IsChecked());
}

TEST(PartyCreateDlgTest, OnActionEventUnknownIdReturnsFalse) {
    cPartyCreateDlg dialog;
    EXPECT_FALSE(dialog.OnActionEvent(99999, nullptr,
                                      cPartyCreateDlg::kActionBtnClick));
}

TEST(PartyCreateDlgTest, CreatePartySynDispatchesOptions) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    CreateCapture cap;
    dialog.SetCallbacks(&CaptureCreateSyn, &StubChatMsg, &StubResourceMsg,
                         &AlwaysNoParty, &cap);

    controls.theme.SetEditText("DragonSlayers");
    controls.minLevel.SetEditText("20");
    controls.maxLevel.SetEditText("60");
    controls.publicChk.SetChecked(true);
    controls.distribute.AddItem({"Random", 0u, 0}); controls.distribute.SelectComboText(0);
    controls.memberNum.AddItem({"8", 0u, 0}); controls.memberNum.SelectComboText(0);

    EXPECT_TRUE(dialog.CreatePartySyn());
    EXPECT_EQ(cap.calls, 1);
    EXPECT_STREQ(cap.lastOpts.theme, "DragonSlayers");
    EXPECT_EQ(cap.lastOpts.minLevel, 20);
    EXPECT_EQ(cap.lastOpts.maxLevel, 60);
    EXPECT_TRUE(cap.lastOpts.isPublic);
    EXPECT_EQ(cap.lastOpts.division, PartyDivisionOption::Random);
    EXPECT_EQ(cap.lastOpts.limitCount, 8);
    EXPECT_TRUE(dialog.isProcessing());
}

TEST(PartyCreateDlgTest, CreatePartySynRejectsLongTheme) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    CreateCapture cap;
    dialog.SetCallbacks(&CaptureCreateSyn, &StubChatMsg, &StubResourceMsg,
                         &AlwaysNoParty, &cap);
    controls.theme.SetEditText("ThisThemeNameIsWayTooLongForLimit");
    EXPECT_FALSE(dialog.CreatePartySyn());
    EXPECT_EQ(cap.calls, 0);
    EXPECT_FALSE(dialog.isProcessing());
}

TEST(PartyCreateDlgTest, CreatePartySynDamageOption) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    CreateCapture cap;
    dialog.SetCallbacks(&CaptureCreateSyn, &StubChatMsg, &StubResourceMsg,
                         &AlwaysNoParty, &cap);

    controls.theme.SetEditText("Raid");
    controls.distribute.AddItem({"Damage", 0u, 0}); controls.distribute.SelectComboText(0);
    controls.memberNum.AddItem({"4", 0u, 0}); controls.memberNum.SelectComboText(0);

    EXPECT_TRUE(dialog.CreatePartySyn());
    EXPECT_EQ(cap.lastOpts.division, PartyDivisionOption::Damage);
    EXPECT_EQ(cap.lastOpts.limitCount, 4);
}

TEST(PartyCreateDlgTest, CreatePartySynUnknownDivisionWhenNoResource) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    CreateCapture cap;
    dialog.SetCallbacks(&CaptureCreateSyn, &StubChatMsg, nullptr,
                         &AlwaysNoParty, &cap);

    controls.theme.SetEditText("Raid");
    controls.distribute.AddItem({"Whatever", 0u, 0}); controls.distribute.SelectComboText(0);
    EXPECT_TRUE(dialog.CreatePartySyn());
    EXPECT_EQ(cap.lastOpts.division, PartyDivisionOption::Unknown);
}

TEST(PartyCreateDlgTest, CreatePartySynRespectsHasParty) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    CreateCapture cap;
    dialog.SetCallbacks(&CaptureCreateSyn, &StubChatMsg, &StubResourceMsg,
                         &AlwaysHasParty, &cap);

    controls.theme.SetEditText("Raid");
    EXPECT_FALSE(dialog.CreatePartySyn());
    EXPECT_EQ(cap.calls, 0);
    EXPECT_FALSE(dialog.isProcessing());
}

TEST(PartyCreateDlgTest, CreatePartySynRejectsWhenSendFails) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    CreateCapture cap;
    cap.returnValue = false;
    dialog.SetCallbacks(&CaptureCreateSyn, &StubChatMsg, &StubResourceMsg,
                         &AlwaysNoParty, &cap);

    controls.theme.SetEditText("Raid");
    EXPECT_FALSE(dialog.CreatePartySyn());
    EXPECT_FALSE(dialog.isProcessing());
}

TEST(PartyCreateDlgTest, CreatePartySynToleratesNullControls) {
    cPartyCreateDlg dialog;
    CreateCapture cap;
    dialog.SetCallbacks(&CaptureCreateSyn, &StubChatMsg, &StubResourceMsg,
                         &AlwaysNoParty, &cap);
    EXPECT_TRUE(dialog.CreatePartySyn());
    EXPECT_EQ(cap.calls, 1);
    EXPECT_STREQ(cap.lastOpts.theme, "");
    EXPECT_FALSE(cap.lastOpts.isPublic);
}

TEST(PartyCreateDlgTest, OnActionEventOkTriggersCreateAndDeactivates) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    CreateCapture cap;
    dialog.SetCallbacks(&CaptureCreateSyn, &StubChatMsg, &StubResourceMsg,
                         &AlwaysNoParty, &cap);
    controls.theme.SetEditText("Hunt");
    dialog.SetActive(true);

    dialog.OnActionEvent(cPartyCreateDlg::kOkButtonId, nullptr,
                         cPartyCreateDlg::kActionBtnClick);
    EXPECT_EQ(cap.calls, 1);
    EXPECT_FALSE(dialog.isActive());
}

TEST(PartyCreateDlgTest, OkDoesNotDeactivateWhenCreateFails) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    CreateCapture cap;
    cap.returnValue = false;
    dialog.SetCallbacks(&CaptureCreateSyn, &StubChatMsg, &StubResourceMsg,
                         &AlwaysNoParty, &cap);
    controls.theme.SetEditText("Hunt");
    dialog.SetActive(true);

    dialog.OnActionEvent(cPartyCreateDlg::kOkButtonId, nullptr,
                         cPartyCreateDlg::kActionBtnClick);
    EXPECT_TRUE(dialog.isActive());
}

TEST(PartyCreateDlgTest, SetActiveFalseReappliesDefaults) {
    cPartyCreateDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.theme.SetEditText("Custom");
    controls.publicChk.SetChecked(false);
    dialog.SetActive(false);
    EXPECT_EQ(controls.theme.editText(), "");
    EXPECT_TRUE(controls.publicChk.IsChecked());
}

TEST(PartyCreateDlgTest, LinkingResolvesAllControlsAndResetsProcessing) {
    cPartyCreateDlg dialog;
    auto addEdit = [&](std::int32_t id) {
        auto child = std::make_unique<cEditBox>();
        child->Init(0, 0, 50, 20, nullptr, nullptr, id);
        cEditBox* raw = child.get();
        dialog.Add(std::move(child));
        return raw;
    };
    auto addCheck = [&](std::int32_t id) {
        auto child = std::make_unique<cCheckBox>();
        child->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, {}, id);
        cCheckBox* raw = child.get();
        dialog.Add(std::move(child));
        return raw;
    };
    auto addCombo = [&](std::int32_t id) {
        auto child = std::make_unique<cComboBox>();
        child->Init(0, 0, 100, 20, nullptr, id);
        cComboBox* raw = child.get();
        dialog.Add(std::move(child));
        return raw;
    };
    auto addButton = [&](std::int32_t id) {
        auto child = std::make_unique<cButton>();
        child->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, {}, nullptr, id);
        cButton* raw = child.get();
        dialog.Add(std::move(child));
        return raw;
    };

    cEditBox* theme = addEdit(cPartyCreateDlg::kThemeEditId);
    cEditBox* minLevel = addEdit(cPartyCreateDlg::kMinLevelEditId);
    cEditBox* maxLevel = addEdit(cPartyCreateDlg::kMaxLevelEditId);
    cCheckBox* pub = addCheck(cPartyCreateDlg::kPublicCheckId);
    cCheckBox* priv = addCheck(cPartyCreateDlg::kPrivateCheckId);
    cComboBox* dist = addCombo(cPartyCreateDlg::kDistributeComboId);
    cComboBox* mem = addCombo(cPartyCreateDlg::kMemberNumComboId);
    cButton* ok = addButton(cPartyCreateDlg::kOkButtonId);
    cButton* cancel = addButton(cPartyCreateDlg::kCancelButtonId);

    dialog.Linking();
    EXPECT_EQ(dialog.themeEdit(), theme);
    EXPECT_EQ(dialog.minLevelEdit(), minLevel);
    EXPECT_EQ(dialog.maxLevelEdit(), maxLevel);
    EXPECT_EQ(dialog.publicCheck(), pub);
    EXPECT_EQ(dialog.privateCheck(), priv);
    EXPECT_EQ(dialog.distributeCombo(), dist);
    EXPECT_EQ(dialog.memberNumCombo(), mem);
    EXPECT_EQ(dialog.okButton(), ok);
    EXPECT_EQ(dialog.cancelButton(), cancel);
    EXPECT_FALSE(dialog.isProcessing());
    EXPECT_TRUE(pub->IsChecked());
    EXPECT_FALSE(priv->IsChecked());
}
