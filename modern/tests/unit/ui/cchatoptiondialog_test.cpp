//
// Unit tests for mxh::ui::cChatOptionDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * Constants: kChatOptionCount=12, kIdOptionBase=750,
//                kIdOptionEnd=761, kWeChecked=0x0080,
//                kWeNotChecked=0x0100
//   * Default construction: option array all false,
//                            m_bFirst=true, m_bChanged=false
//   * Init forwards to cDialog::Init and resets
//     m_bFirst / m_bChanged
//   * Linking is a no-op (host injects checkboxes)
//   * SetActive(true): invokes GetOption callback
//     to fill m_options, then SetChecked on each
//     registered checkbox, then clears m_bFirst +
//     m_bChanged
//   * SetActive(true) without callback: zeros the
//     option array (host hasn't wired CHATMGR)
//   * SetActive(true): m_bFirst transitions to
//     false (so the next close fires save if
//     dirty)
//   * SetActive(false) with m_bFirst=false +
//     m_bChanged=true: calls SaveOption callback
//   * SetActive(false) with m_bFirst=false +
//     m_bChanged=false: callback NOT called
//   * SetActive(false) with m_bFirst=true
//     (first close before open): no save
//   * SetActive(false) does NOT reset m_bChanged
//     (1:1 quirk: legacy leaves it latched)
//   * SetActive blocked when dialog is disabled
//   * OnActionEvent(WE_CHECKED): sets options[idx]
//     = true, m_bChanged=true
//   * OnActionEvent(WE_NOTCHECKED): sets options[idx]
//     = false, m_bChanged=true
//   * OnActionEvent without CHECKED/NOTCHECKED flag:
//     no change to m_options, but m_bChanged still
//     set (1:1 fidelity: legacy always sets
//     m_bChanged=TRUE)
//   * OnActionEvent with lId out of range: no-op
//   * SetCheckBoxesForTest stores pointers
//   * NonCopyable
//

#include "mxh/ui/cchatoptiondialog.hpp"
#include "mxh/ui/ccheckbox.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::cCheckBox;
using mxh::ui::cChatOptionDialog;

namespace {

struct Harness {
    cChatOptionDialog dlg;
    cCheckBox*        boxes[cChatOptionDialog::kChatOptionCount] = {};
    cCheckBox         storage[cChatOptionDialog::kChatOptionCount];

    Harness() {
        for (std::size_t i = 0; i < cChatOptionDialog::kChatOptionCount; ++i) {
            boxes[i] = &storage[i];
        }
        dlg.SetCheckBoxesForTest(boxes);
    }
};

bool g_getOptionArr[cChatOptionDialog::kChatOptionCount] = {};
void GetOptionCb(bool out[12], void* /*user*/) {
    for (std::size_t i = 0; i < cChatOptionDialog::kChatOptionCount; ++i) {
        out[i] = g_getOptionArr[i];
    }
}

bool g_saveOptionArr[cChatOptionDialog::kChatOptionCount] = {};
std::uint32_t g_saveCallCount = 0;
void SaveOptionCb(const bool options[12], void* /*user*/) {
    for (std::size_t i = 0; i < cChatOptionDialog::kChatOptionCount; ++i) {
        g_saveOptionArr[i] = options[i];
    }
    ++g_saveCallCount;
}

void ResetCbState() {
    for (std::size_t i = 0; i < cChatOptionDialog::kChatOptionCount; ++i) {
        g_getOptionArr[i] = false;
        g_saveOptionArr[i] = false;
    }
    g_saveCallCount = 0;
}

}  // namespace


TEST(CChatOptionDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(cChatOptionDialog::kChatOptionCount, 12u);
    EXPECT_EQ(cChatOptionDialog::kIdOptionBase, 750);
    EXPECT_EQ(cChatOptionDialog::kIdOptionEnd,  761);
    EXPECT_EQ(cChatOptionDialog::kWeChecked,     0x0080u);
    EXPECT_EQ(cChatOptionDialog::kWeNotChecked,  0x0100u);
}

TEST(CChatOptionDialog, OptionBaseEndIsContiguous) {
    // 1:1 with legacy COI_CB_OPTION01..12 (12 consecutive ids).
    EXPECT_EQ(cChatOptionDialog::kIdOptionEnd
                  - cChatOptionDialog::kIdOptionBase + 1,
              static_cast<std::int32_t>(
                  cChatOptionDialog::kChatOptionCount));
}

TEST(CChatOptionDialog, DefaultConstructionHasNullCheckboxes) {
    cChatOptionDialog d;
    // SetCheckBoxesForTest with nullptr is a no-op.
    d.SetCheckBoxesForTest(nullptr);
    // No direct getter for the array; assert via
    // Linking + SetActive(false) -- no crash.
    d.Linking();
    d.SetActive(false);
    SUCCEED();
}

TEST(CChatOptionDialog, DefaultOptionsAreAllFalse) {
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetActive(true);
    for (std::size_t i = 0; i < cChatOptionDialog::kChatOptionCount; ++i) {
        EXPECT_FALSE(h.dlg.IsOptionEnabled(i));
    }
}

TEST(CChatOptionDialog, DefaultStateFlags) {
    cChatOptionDialog d;
    EXPECT_TRUE(d.IsFirst());
    EXPECT_FALSE(d.HasChanged());
}


TEST(CChatOptionDialog, InitForwardsToBaseAndResetsFlags) {
    cChatOptionDialog d;
    // Dirty the flags first so we can prove Init
    // resets them.
    d.SetOptionForTest(0, true);
    d.OnActionEvent(cChatOptionDialog::kIdOptionBase, nullptr,
                    cChatOptionDialog::kWeChecked);
    EXPECT_TRUE(d.HasChanged());
    d.Init(10, 20, 200, 300, nullptr, 1234);
    EXPECT_EQ(d.absX(), 10);
    EXPECT_EQ(d.absY(), 20);
    EXPECT_EQ(d.width(),  200);
    EXPECT_EQ(d.height(), 300);
    EXPECT_EQ(d.id(),     1234);
    // m_bFirst and m_bChanged reset on Init.
    EXPECT_TRUE(d.IsFirst());
    EXPECT_FALSE(d.HasChanged());
}


TEST(CChatOptionDialog, LinkingIsNoOpWithInjectedCheckboxes) {
    Harness h;
    h.dlg.Linking();
    // Linking should not crash and should not
    // modify state.  After Linking, SetActive(true)
    // still drives the checkboxes from the array.
    ResetCbState();
    g_getOptionArr[3] = true;
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.storage[3].IsChecked());
    EXPECT_FALSE(h.storage[0].IsChecked());
}


TEST(CChatOptionDialog, SetActiveTrueCallsGetOptionCallback) {
    Harness h;
    ResetCbState();
    g_getOptionArr[1] = true;
    g_getOptionArr[5] = true;
    g_getOptionArr[9] = true;
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.IsOptionEnabled(1));
    EXPECT_TRUE(h.dlg.IsOptionEnabled(5));
    EXPECT_TRUE(h.dlg.IsOptionEnabled(9));
    EXPECT_FALSE(h.dlg.IsOptionEnabled(0));
    // Checkboxes synced.
    EXPECT_TRUE(h.storage[1].IsChecked());
    EXPECT_TRUE(h.storage[5].IsChecked());
    EXPECT_TRUE(h.storage[9].IsChecked());
    EXPECT_FALSE(h.storage[0].IsChecked());
}

TEST(CChatOptionDialog, SetActiveTrueWithoutCallbackZerosOptions) {
    Harness h;
    // No callback: m_options[] should be zeroed.
    // 1:1 fallback: legacy always reads from
    // CHATMGR->GetOption(); modern port defaults
    // to all-false when the host isn't wired.
    h.dlg.SetActive(true);
    for (std::size_t i = 0; i < cChatOptionDialog::kChatOptionCount; ++i) {
        EXPECT_FALSE(h.dlg.IsOptionEnabled(i));
        EXPECT_FALSE(h.storage[i].IsChecked());
    }
}

TEST(CChatOptionDialog, SetActiveTrueClearsChangedFlag) {
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    // Pre-pollute a checkbox so we can verify SetActive
    // resets via the GetOption round-trip.
    h.storage[2].SetChecked(true);
    h.dlg.SetActive(true);
    EXPECT_FALSE(h.dlg.HasChanged());
    EXPECT_FALSE(h.dlg.IsFirst());
}

TEST(CChatOptionDialog, SetActiveFalseFirstCloseDoesNotSave) {
    // 1:1 quirk: legacy checks m_bFirst == FALSE
    // before saving.  The very first close (before
    // open) doesn't save even if m_bChanged is true.
    cChatOptionDialog d;
    d.SetSaveOptionCallbackForTest(&SaveOptionCb, nullptr);
    ResetCbState();
    d.SetActive(false);
    EXPECT_EQ(g_saveCallCount, 0u);
}

TEST(CChatOptionDialog, SetActiveFalseSavesWhenChanged) {
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetSaveOptionCallbackForTest(&SaveOptionCb, nullptr);
    h.dlg.SetActive(true);  // open
    EXPECT_FALSE(h.dlg.HasChanged());

    // User toggles options 2 and 7.
    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionBase + 2, nullptr,
                        cChatOptionDialog::kWeChecked);
    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionBase + 7, nullptr,
                        cChatOptionDialog::kWeNotChecked);
    EXPECT_TRUE(h.dlg.HasChanged());

    h.dlg.SetActive(false);  // close -- save fires
    EXPECT_EQ(g_saveCallCount, 1u);
    EXPECT_TRUE(g_saveOptionArr[2]);
    EXPECT_FALSE(g_saveOptionArr[7]);
    EXPECT_FALSE(g_saveOptionArr[0]);
}

TEST(CChatOptionDialog, SetActiveFalseSkipsSaveWhenNotChanged) {
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetSaveOptionCallbackForTest(&SaveOptionCb, nullptr);
    h.dlg.SetActive(true);   // open
    // No changes made.
    h.dlg.SetActive(false);  // close -- no save
    EXPECT_EQ(g_saveCallCount, 0u);
}

TEST(CChatOptionDialog, SetActiveFalseWithoutSaveCallbackIsSafe) {
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    // No SaveOptionCallback set.
    h.dlg.SetActive(true);
    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionBase, nullptr,
                        cChatOptionDialog::kWeChecked);
    h.dlg.SetActive(false);  // should not crash
    EXPECT_EQ(g_saveCallCount, 0u);
}

TEST(CChatOptionDialog, SetActiveDoesNotResetChangedOnClose) {
    // 1:1 quirk: legacy leaves m_bChanged set after
    // close; the next open resets it.
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetSaveOptionCallbackForTest(&SaveOptionCb, nullptr);
    h.dlg.SetActive(true);
    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionBase, nullptr,
                        cChatOptionDialog::kWeChecked);
    h.dlg.SetActive(false);
    // Legacy behaviour: m_bChanged is still TRUE.
    // The next open zeros it.
    EXPECT_TRUE(h.dlg.HasChanged());
    h.dlg.SetActive(true);
    EXPECT_FALSE(h.dlg.HasChanged());
}

TEST(CChatOptionDialog, SetActiveBlockedWhenDisabled) {
    // 1:1 with legacy `if (m_bDisable) return;`.
    // Disabled dialogs swallow SetActive.
    Harness h;
    ResetCbState();
    g_getOptionArr[0] = true;
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetDisable(true);
    h.dlg.SetActive(true);
    EXPECT_FALSE(h.dlg.isActive());
    // Checkboxes not updated.
    EXPECT_FALSE(h.storage[0].IsChecked());
    // m_bFirst stayed true (no open happened).
    EXPECT_TRUE(h.dlg.IsFirst());
}


TEST(CChatOptionDialog, OnActionEventCheckedSetsOptionTrue) {
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetActive(true);

    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionBase + 4, nullptr,
                        cChatOptionDialog::kWeChecked);
    EXPECT_TRUE(h.dlg.IsOptionEnabled(4));
    EXPECT_TRUE(h.dlg.HasChanged());
}

TEST(CChatOptionDialog, OnActionEventNotCheckedSetsOptionFalse) {
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetActive(true);

    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionBase + 4, nullptr,
                        cChatOptionDialog::kWeChecked);
    EXPECT_TRUE(h.dlg.IsOptionEnabled(4));

    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionBase + 4, nullptr,
                        cChatOptionDialog::kWeNotChecked);
    EXPECT_FALSE(h.dlg.IsOptionEnabled(4));
    EXPECT_TRUE(h.dlg.HasChanged());
}

TEST(CChatOptionDialog, OnActionEventUnknownFlagStillFlagsChanged) {
    // 1:1 quirk: legacy always sets m_bChanged = TRUE
    // regardless of the WE_* bits.  Use a flag that
    // overlaps neither WE_CHECKED nor WE_NOTCHECKED
    // (e.g. WE_LBTNCLICK = 0x0040).  The options
    // array is unchanged but m_bChanged latches.
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetActive(true);
    EXPECT_FALSE(h.dlg.HasChanged());

    constexpr std::uint32_t kWeOther = 0x0040u;  // WE_LBTNCLICK
    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionBase + 0, nullptr,
                        kWeOther);
    EXPECT_TRUE(h.dlg.HasChanged());
    // Options array unchanged -- still false from open.
    for (std::size_t i = 0; i < cChatOptionDialog::kChatOptionCount; ++i) {
        EXPECT_FALSE(h.dlg.IsOptionEnabled(i));
    }
}

TEST(CChatOptionDialog, OnActionEventBelowBaseIsNoOp) {
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetActive(true);

    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionBase - 1, nullptr,
                        cChatOptionDialog::kWeChecked);
    EXPECT_FALSE(h.dlg.HasChanged());
}

TEST(CChatOptionDialog, OnActionEventAboveEndIsNoOp) {
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetActive(true);

    h.dlg.OnActionEvent(cChatOptionDialog::kIdOptionEnd + 1, nullptr,
                        cChatOptionDialog::kWeChecked);
    EXPECT_FALSE(h.dlg.HasChanged());
}

TEST(CChatOptionDialog, OnActionEventAllTwelveChannels) {
    // 1:1 with legacy: every channel id
    // (kIdOptionBase..kIdOptionBase+11) toggles the
    // corresponding m_options[] slot.
    Harness h;
    ResetCbState();
    h.dlg.SetGetOptionCallbackForTest(&GetOptionCb, nullptr);
    h.dlg.SetActive(true);

    for (std::size_t i = 0; i < cChatOptionDialog::kChatOptionCount; ++i) {
        h.dlg.OnActionEvent(
            static_cast<std::int32_t>(
                cChatOptionDialog::kIdOptionBase + i),
            nullptr,
            cChatOptionDialog::kWeChecked);
        EXPECT_TRUE(h.dlg.IsOptionEnabled(i));
    }
    for (std::size_t i = 0; i < cChatOptionDialog::kChatOptionCount; ++i) {
        h.dlg.OnActionEvent(
            static_cast<std::int32_t>(
                cChatOptionDialog::kIdOptionBase + i),
            nullptr,
            cChatOptionDialog::kWeNotChecked);
        EXPECT_FALSE(h.dlg.IsOptionEnabled(i));
    }
}


TEST(CChatOptionDialog, SetCheckBoxesNullIsSafe) {
    cChatOptionDialog d;
    d.SetCheckBoxesForTest(nullptr);
    // SetCheckBoxesForTest with nullptr is a no-op
    // (does not crash on a null pointer).
    d.Linking();
    SUCCEED();
}


TEST(CChatOptionDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible<cChatOptionDialog>::value,
                  "cChatOptionDialog must not be copyable");
    static_assert(!std::is_copy_assignable<cChatOptionDialog>::value,
                  "cChatOptionDialog must not be copy-assignable");
    SUCCEED();
}

TEST(CChatOptionDialog, IscDialog) {
    // 1:1 with legacy class hierarchy: cChatOptionDialog
    // inherits from cDialog.
    static_assert(std::is_base_of<mxh::ui::cDialog,
                                  cChatOptionDialog>::value,
                  "cChatOptionDialog must inherit from cDialog");
    SUCCEED();
}
