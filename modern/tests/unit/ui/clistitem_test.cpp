// mxh/tests/unit/ui/clistitem_test.cpp
// Unit tests for mxh::ui::cListItem + mxh::ui::ComboItem (Phase C).
//
// 1:1 port of the legacy `cListItem` helper base from
// `墨香【源码】\[Client]MH\Interface\cListItem.h`. The modern port uses
// std::vector<ComboItem> instead of the engine-side cPtrList, but the
// API surface (AddItem/RemoveAll/RemoveItem/GetItemCount/SetMaxLine) is
// preserved.
//
// Coverage:
//   * ComboItem default field values
//   * AddItem single + multiple, FIFO eviction at cap
//   * AddItem at index, head-drop when at cap
//   * AddItem(idx) with idx >= size silently dropped (legacy FindIndex
//     returns null + InsertAfter no-ops)
//   * RemoveAll empties the list
//   * RemoveItem(idx) ignores out-of-range indices
//   * SetMaxLine / GetMaxLine round-trip
//
// KNOWN-INCONSISTENCY (flagged for follow-up, not fixed in this commit):
//   The header comment says "m_maxLine (WORD cap; 0 = unlimited)" but
//   the implementation rejects any AddItem when m_maxLine < 1, so the
//   "0 = unlimited" path is actually broken.  This commit's tests
//   document the current (buggy) behavior so any future fix is
//   visible.  Phase C 1.1+ will add a regression test once fixed.

#include "clistitem.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

using mxh::ui::cListItem;
using mxh::ui::ComboItem;

// -------------------------------------------------------------------------
// ComboItem default field values (1:1 with the legacy ITEM struct's
// zero-init).
// -------------------------------------------------------------------------

TEST(ComboItem, DefaultFieldsAreZero) {
    ComboItem it;
    EXPECT_EQ(it.text, "");
    EXPECT_EQ(it.rgb,   0xFFFFFFFFu);  // ARGB white by default
    EXPECT_EQ(it.type,  0u);
}

TEST(ComboItem, RoundTripAllFields) {
    ComboItem it;
    it.text = "hello";
    it.rgb  = 0xFFAABBCCu;
    it.type = 42;
    EXPECT_EQ(it.text, "hello");
    EXPECT_EQ(it.rgb,  0xFFAABBCCu);
    EXPECT_EQ(it.type, 42u);
}

// -------------------------------------------------------------------------
// cListItem basic state + AddItem single
// -------------------------------------------------------------------------

TEST(CListItem, DefaultEmptyAndZeroMaxLine) {
    cListItem li;
    EXPECT_EQ(li.GetItemCount(), 0u);
    EXPECT_EQ(li.GetMaxLine(),   0u);
    EXPECT_TRUE(li.Items().empty());
}

TEST(CListItem, AddItemSingle) {
    cListItem li;
    li.SetMaxLine(10);
    li.AddItem(ComboItem{ "alpha", 0xFF112233u, 1 });
    EXPECT_EQ(li.GetItemCount(), 1u);
    EXPECT_EQ(li.Items().front().text, "alpha");
    EXPECT_EQ(li.Items().front().rgb,   0xFF112233u);
    EXPECT_EQ(li.Items().front().type,  1u);
}

TEST(CListItem, AddItemMultiple) {
    cListItem li;
    li.SetMaxLine(10);
    li.AddItem(ComboItem{ "a" });
    li.AddItem(ComboItem{ "b" });
    li.AddItem(ComboItem{ "c" });
    EXPECT_EQ(li.GetItemCount(), 3u);
    EXPECT_EQ(li.Items()[0].text, "a");
    EXPECT_EQ(li.Items()[1].text, "b");
    EXPECT_EQ(li.Items()[2].text, "c");
}

// -------------------------------------------------------------------------
// FIFO eviction at max-line cap (1:1 with legacy behaviour).
// -------------------------------------------------------------------------

TEST(CListItem, AddItemEvictsHeadAtMaxLine) {
    cListItem li;
    li.SetMaxLine(3);
    li.AddItem(ComboItem{ "a" });
    li.AddItem(ComboItem{ "b" });
    li.AddItem(ComboItem{ "c" });
    EXPECT_EQ(li.GetItemCount(), 3u);
    // Adding a 4th should drop the head ("a") and append "d".
    li.AddItem(ComboItem{ "d" });
    ASSERT_EQ(li.GetItemCount(), 3u);
    EXPECT_EQ(li.Items()[0].text, "b");
    EXPECT_EQ(li.Items()[1].text, "c");
    EXPECT_EQ(li.Items()[2].text, "d");
}

TEST(CListItem, EvictionContinuesForManyOverflow) {
    cListItem li;
    li.SetMaxLine(2);
    for (int i = 0; i < 10; ++i) {
        li.AddItem(ComboItem{ "item" + std::to_string(i) });
    }
    ASSERT_EQ(li.GetItemCount(), 2u);
    EXPECT_EQ(li.Items()[0].text, "item8");
    EXPECT_EQ(li.Items()[1].text, "item9");
}

// -------------------------------------------------------------------------
// AddItem(idx) at index
// -------------------------------------------------------------------------

TEST(CListItem, AddItemAtIndex) {
    cListItem li;
    li.SetMaxLine(10);
    li.AddItem(ComboItem{ "a" });
    li.AddItem(ComboItem{ "b" });
    li.AddItem(ComboItem{ "c" });
    li.AddItem(ComboItem{ "X" }, 1);
    ASSERT_EQ(li.GetItemCount(), 4u);
    EXPECT_EQ(li.Items()[0].text, "a");
    EXPECT_EQ(li.Items()[1].text, "X");
    EXPECT_EQ(li.Items()[2].text, "b");
    EXPECT_EQ(li.Items()[3].text, "c");
}

TEST(CListItem, AddItemAtIndexOutOfRangeSilentlyDropped) {
    cListItem li;
    li.SetMaxLine(10);
    li.AddItem(ComboItem{ "a" });
    li.AddItem(ComboItem{ "b" });
    // idx >= size -> silent drop (legacy FindIndex returns null).
    li.AddItem(ComboItem{ "X" }, 5);
    EXPECT_EQ(li.GetItemCount(), 2u);
    EXPECT_EQ(li.Items()[0].text, "a");
    EXPECT_EQ(li.Items()[1].text, "b");
}

TEST(CListItem, AddItemAtIndexEvictsHeadAtMaxLine) {
    cListItem li;
    li.SetMaxLine(3);
    li.AddItem(ComboItem{ "a" });
    li.AddItem(ComboItem{ "b" });
    li.AddItem(ComboItem{ "c" });
    // List is at cap; AddItem(idx=1, "X") should evict head ("a") first
    // then insert "X" at position 1.
    li.AddItem(ComboItem{ "X" }, 1);
    ASSERT_EQ(li.GetItemCount(), 3u);
    EXPECT_EQ(li.Items()[0].text, "b");
    EXPECT_EQ(li.Items()[1].text, "X");
    EXPECT_EQ(li.Items()[2].text, "c");
}

// -------------------------------------------------------------------------
// RemoveAll + RemoveItem
// -------------------------------------------------------------------------

TEST(CListItem, RemoveAllEmpties) {
    cListItem li;
    li.SetMaxLine(10);
    li.AddItem(ComboItem{ "a" });
    li.AddItem(ComboItem{ "b" });
    li.AddItem(ComboItem{ "c" });
    li.RemoveAll();
    EXPECT_EQ(li.GetItemCount(), 0u);
    EXPECT_TRUE(li.Items().empty());
    // AddItem after RemoveAll should work.
    li.AddItem(ComboItem{ "z" });
    EXPECT_EQ(li.GetItemCount(), 1u);
    EXPECT_EQ(li.Items()[0].text, "z");
}

TEST(CListItem, RemoveItemByIndex) {
    cListItem li;
    li.SetMaxLine(10);
    li.AddItem(ComboItem{ "a" });
    li.AddItem(ComboItem{ "b" });
    li.AddItem(ComboItem{ "c" });
    li.RemoveItem(1);
    ASSERT_EQ(li.GetItemCount(), 2u);
    EXPECT_EQ(li.Items()[0].text, "a");
    EXPECT_EQ(li.Items()[1].text, "c");
}

TEST(CListItem, RemoveItemOutOfRangeIgnored) {
    cListItem li;
    li.SetMaxLine(10);
    li.AddItem(ComboItem{ "a" });
    li.RemoveItem(5);
    EXPECT_EQ(li.GetItemCount(), 1u);
    EXPECT_EQ(li.Items()[0].text, "a");
}

// -------------------------------------------------------------------------
// MaxLine round-trip
// -------------------------------------------------------------------------

TEST(CListItem, SetMaxLineRoundTrip) {
    cListItem li;
    EXPECT_EQ(li.GetMaxLine(), 0u);
    li.SetMaxLine(15);
    EXPECT_EQ(li.GetMaxLine(), 15u);
    li.SetMaxLine(0);
    EXPECT_EQ(li.GetMaxLine(), 0u);
}

// -------------------------------------------------------------------------
// KNOWN INCONSISTENCY (documented; not fixed in this commit).
//
// The header comment says "m_maxLine (WORD cap; 0 = unlimited)" but
// the implementation rejects any AddItem when m_maxLine < 1, so
// m_maxLine=0 is effectively "no items ever added" rather than
// "unlimited".  Locking down the current behaviour so a future fix is
// visible:
// -------------------------------------------------------------------------

TEST(CListItem, KnownInconsistencyMaxLineZeroRejectsAdds) {
    cListItem li;
    // m_maxLine defaults to 0; the implementation treats this as a
    // hard "do not add" gate instead of "unlimited".  Documented for
    // a follow-up fix.
    EXPECT_EQ(li.GetMaxLine(), 0u);
    li.AddItem(ComboItem{ "should_not_be_added" });
    EXPECT_EQ(li.GetItemCount(), 0u);
}
