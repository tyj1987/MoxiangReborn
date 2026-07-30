//
// Unit tests for mxh::ui::cWantedDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//  * Constants: kMaxWantedNum=20, kMaxNameLength=16,
//    kRegistDateSize=11
//  * WantedListEntry field layout (1:1 with legacy WANTEDLIST)
//  * Default construction: list is null until injected
//  * Linking is a no-op (host injects cListDialog first)
//  * SetInfo resets the list then fills rows from the snapshot
//  * SetInfo stops on the first WantedIDX == 0 row
//  * SetInfo respects MAX_WANTED_NUM bound
//  * SetInfo scrolls back to top after ResetGuageBarPos
//  * AddInfo appends a single row (no sentinel check)
//  * AddInfo adds 2 list rows per call (date + name)
//  * InitWanted clears the list
//  * Default chatmsg callback returns "%s" -> name only
//  * Custom chatmsg callback can format (e.g. "[%s]")
//  * Null pInfo is a no-op
//  * NonCopyable
//

#include "mxh/ui/cwanteddialog.hpp"
#include "mxh/ui/clistdialog.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <type_traits>

using mxh::ui::cListDialog;
using mxh::ui::cWantedDialog;
using mxh::ui::WantedListEntry;
using mxh::ui::kMaxNameLength;
using mxh::ui::kMaxWantedNum;
using mxh::ui::kRegistDateSize;

namespace {

struct Harness {
    cWantedDialog dlg;
    cListDialog   list;

    Harness() {
        dlg.SetListDialogForTest(&list);
    }
};

WantedListEntry makeEntry(std::uint32_t idx, const char* name, const char* date) {
    WantedListEntry e{};
    e.WantedIDX = idx;
    std::strncpy(e.WantedName, name, kMaxNameLength);
    std::strncpy(e.RegistDate, date, kRegistDateSize - 1);
    return e;
}

const char* bracketFormat(int /*id*/, void* /*user*/) { return "[%s]"; }

}  // namespace


TEST(CWantedDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(kMaxWantedNum,   20);
    EXPECT_EQ(kMaxNameLength,  16);
    EXPECT_EQ(kRegistDateSize, 11);
}

TEST(CWantedDialog, DefaultConstructionHasNoList) {
    cWantedDialog d;
    EXPECT_EQ(d.GetListDialogForTest(), nullptr);
}

TEST(CWantedDialog, WantedListEntryFieldLayout) {
    // 1:1 with legacy WANTEDLIST in [CC]Header/CommonStruct.h.
    // The struct is part of the wire format; size + offsets
    // must remain stable.
    EXPECT_EQ(sizeof(WantedListEntry), 4u + 4u + (kMaxNameLength + 1) + kRegistDateSize);
    WantedListEntry e{};
    // Fields start zeroed.
    EXPECT_EQ(e.WantedIDX,   0u);
    EXPECT_EQ(e.WantedChrID, 0u);
    EXPECT_EQ(e.WantedName[0], '\0');
    EXPECT_EQ(e.RegistDate[0], '\0');
}

TEST(CWantedDialog, LinkingIsNoOpWithInjectedList) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetListDialogForTest(), &h.list);
}


TEST(CWantedDialog, SetInfoFillsTwoRowsPerWanted) {
    Harness h;
    WantedListEntry arr[kMaxWantedNum] = {};
    arr[0] = makeEntry(1, "Alice",   "2026-07-30");
    arr[1] = makeEntry(2, "Bob",     "2026-07-31");
    arr[2] = makeEntry(3, "Charlie", "2026-08-01");
    h.dlg.SetInfo(arr);
    // 2 rows per wanted: date + name.
    EXPECT_EQ(h.list.RowCount(), 6u);
    EXPECT_EQ(h.list.GetRow(0).first, "2026-07-30");
    EXPECT_EQ(h.list.GetRow(1).first, "Alice");
    EXPECT_EQ(h.list.GetRow(2).first, "2026-07-31");
    EXPECT_EQ(h.list.GetRow(3).first, "Bob");
    EXPECT_EQ(h.list.GetRow(4).first, "2026-08-01");
    EXPECT_EQ(h.list.GetRow(5).first, "Charlie");
}

TEST(CWantedDialog, SetInfoStopsOnZeroSentinel) {
    Harness h;
    WantedListEntry arr[kMaxWantedNum] = {};
    arr[0] = makeEntry(1, "Alice",   "2026-07-30");
    arr[1] = makeEntry(2, "Bob",     "2026-07-31");
    arr[2] = makeEntry(0, "X",       "1970-01-01");  // sentinel
    arr[3] = makeEntry(3, "Charlie", "2026-08-01");
    arr[4] = makeEntry(4, "Dave",    "2026-08-02");
    h.dlg.SetInfo(arr);
    EXPECT_EQ(h.list.RowCount(), 4u);  // 2 wanteds x 2 rows
    EXPECT_EQ(h.list.GetRow(1).first, "Alice");
    EXPECT_EQ(h.list.GetRow(3).first, "Bob");
}

TEST(CWantedDialog, SetInfoRespectsMaxWantedNumBound) {
    Harness h;
    WantedListEntry arr[kMaxWantedNum + 5] = {};
    for (int i = 0; i < kMaxWantedNum + 5; ++i) {
        arr[i] = makeEntry(static_cast<std::uint32_t>(i + 1), "X", "2026-07-31");
    }
    h.dlg.SetInfo(arr);
    EXPECT_EQ(h.list.RowCount(), static_cast<std::size_t>(kMaxWantedNum) * 2u);
}

TEST(CWantedDialog, SetInfoScrollsBackToTop) {
    Harness h;
    // Pre-populate the list with rows + a fake top index.
    h.list.AddItem("stale", 0xffffffffu);
    h.list.AddItem("stale2", 0xffffffffu);
    // SetTopListItemIdx(1) is a no-op for empty list; with 2
    // rows it sets top to 1.  But SetInfo calls RemoveAll
    // first, so the top index should be reset to 0.
    WantedListEntry arr[kMaxWantedNum] = {};
    arr[0] = makeEntry(1, "Alice", "2026-07-30");
    h.dlg.SetInfo(arr);
    EXPECT_EQ(h.list.GetTopListItemIdx(), 0);
}

TEST(CWantedDialog, SetInfoNullPointerIsNoOp) {
    Harness h;
    // Fill with something first.
    WantedListEntry arr[kMaxWantedNum] = {};
    arr[0] = makeEntry(1, "Alice", "2026-07-30");
    h.dlg.SetInfo(arr);
    EXPECT_EQ(h.list.RowCount(), 2u);
    // Now call with null -- no change.
    h.dlg.SetInfo(nullptr);
    EXPECT_EQ(h.list.RowCount(), 2u);
}


TEST(CWantedDialog, AddInfoAppendsOneEntry) {
    Harness h;
    WantedListEntry a = makeEntry(1, "Alice", "2026-07-30");
    h.dlg.AddInfo(&a);
    EXPECT_EQ(h.list.RowCount(), 2u);
    EXPECT_EQ(h.list.GetRow(0).first, "2026-07-30");
    EXPECT_EQ(h.list.GetRow(1).first, "Alice");
    WantedListEntry b = makeEntry(2, "Bob", "2026-07-31");
    h.dlg.AddInfo(&b);
    EXPECT_EQ(h.list.RowCount(), 4u);
    EXPECT_EQ(h.list.GetRow(2).first, "2026-07-31");
    EXPECT_EQ(h.list.GetRow(3).first, "Bob");
}

TEST(CWantedDialog, AddInfoIgnoresZeroWantedIDX) {
    // 1:1 with legacy AddInfo -- the legacy does NOT check
    // WantedIDX; it just appends.  Zero-id rows are passed
    // through verbatim (a server bug would manifest as a
    // zero-id row, but the dialog doesn't filter it).
    Harness h;
    WantedListEntry a = makeEntry(0, "Alice", "2026-07-30");
    h.dlg.AddInfo(&a);
    EXPECT_EQ(h.list.RowCount(), 2u);
    EXPECT_EQ(h.list.GetRow(1).first, "Alice");
}

TEST(CWantedDialog, AddInfoNullPointerIsNoOp) {
    Harness h;
    h.dlg.AddInfo(nullptr);
    EXPECT_EQ(h.list.RowCount(), 0u);
}


TEST(CWantedDialog, InitWantedClearsList) {
    Harness h;
    WantedListEntry arr[kMaxWantedNum] = {};
    arr[0] = makeEntry(1, "Alice", "2026-07-30");
    arr[1] = makeEntry(2, "Bob",   "2026-07-31");
    h.dlg.SetInfo(arr);
    EXPECT_EQ(h.list.RowCount(), 4u);
    h.dlg.InitWanted();
    EXPECT_EQ(h.list.RowCount(), 0u);
}

TEST(CWantedDialog, InitWantedOnEmptyListIsNoOp) {
    Harness h;
    h.dlg.InitWanted();
    EXPECT_EQ(h.list.RowCount(), 0u);
}


TEST(CWantedDialog, DefaultChatMsgFormatIsPercentS) {
    Harness h;
    WantedListEntry a = makeEntry(1, "Alice", "2026-07-30");
    h.dlg.AddInfo(&a);
    // 1:1 with legacy CHATMGR->GetChatMsg(545) default = "%s".
    EXPECT_EQ(h.list.GetRow(1).first, "Alice");
}

TEST(CWantedDialog, CustomChatMsgFormatAppliesToAllRows) {
    Harness h;
    h.dlg.SetChatMsgCallbackForTest(&bracketFormat, nullptr);
    WantedListEntry arr[kMaxWantedNum] = {};
    arr[0] = makeEntry(1, "Alice", "2026-07-30");
    arr[1] = makeEntry(2, "Bob",   "2026-07-31");
    h.dlg.SetInfo(arr);
    EXPECT_EQ(h.list.GetRow(1).first, "[Alice]");
    EXPECT_EQ(h.list.GetRow(3).first, "[Bob]");
}

TEST(CWantedDialog, CustomChatMsgFormatAppliesToAddInfo) {
    Harness h;
    h.dlg.SetChatMsgCallbackForTest(&bracketFormat, nullptr);
    WantedListEntry a = makeEntry(1, "Alice", "2026-07-30");
    h.dlg.AddInfo(&a);
    EXPECT_EQ(h.list.GetRow(1).first, "[Alice]");
}


TEST(CWantedDialog, NonCopyable) {
    Harness h;
    static_assert(!std::is_copy_constructible<cWantedDialog>::value,
                  "cWantedDialog must not be copyable");
    static_assert(!std::is_copy_assignable<cWantedDialog>::value,
                  "cWantedDialog must not be copy-assignable");
    SUCCEED();
}
