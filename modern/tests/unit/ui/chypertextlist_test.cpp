// chypertextlist_test.cpp — Phase 6.16 / 0.13.49 coverage for
// cHyperTextList. Tests the data model + AddEntry / GetHyperText
// + RemoveAll. File loading is stubbed (CMHFile is engine-side);
// the modern port's data-side primitive is AddEntry.

#include "chypertextlist.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <type_traits>

TEST(CHyperTextList, DefaultConstructionIsEmpty) {
    mxh::ui::cHyperTextList h;
    EXPECT_EQ(h.GetCount(), 0u);
    EXPECT_EQ(h.GetHyperText(0), nullptr);
    EXPECT_EQ(h.GetHyperText(42), nullptr);
    EXPECT_EQ(h.GetHyperText(UINT32_MAX), nullptr);
}

TEST(CHyperTextList, InheritsBaselineApi) {
    // Smoke check: file-loading is a no-op (no crash).
    mxh::ui::cHyperTextList h;
    h.LoadHyperTextFormFile("test.bin", "rt");
    h.LoadHyperTextFormFile("test.bin");
    SUCCEED();
}

TEST(CHyperTextList, AddEntryAppends) {
    mxh::ui::cHyperTextList h;
    h.AddEntry(1, "first");
    h.AddEntry(2, "second");
    h.AddEntry(3, "third");
    EXPECT_EQ(h.GetCount(), 3u);
}

TEST(CHyperTextList, GetHyperTextReturnsPointer) {
    mxh::ui::cHyperTextList h;
    h.AddEntry(42, "hello world");
    auto* p = h.GetHyperText(42);
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->str, "hello world");
}

TEST(CHyperTextList, GetHyperTextOutOfRangeReturnsNull) {
    mxh::ui::cHyperTextList h;
    h.AddEntry(42, "only");
    EXPECT_EQ(h.GetHyperText(99), nullptr);
    EXPECT_EQ(h.GetHyperText(0), nullptr);
    EXPECT_EQ(h.GetHyperText(UINT32_MAX), nullptr);
}

TEST(CHyperTextList, AddEntryOverwritesAtSameIdx) {
    // 1:1 with legacy m_HyperText.Add(pTemp, idx) — overwrites
    // any existing entry at the same idx.
    mxh::ui::cHyperTextList h;
    h.AddEntry(42, "first");
    h.AddEntry(42, "second");
    EXPECT_EQ(h.GetCount(), 1u);
    auto* p = h.GetHyperText(42);
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->str, "second");
}

TEST(CHyperTextList, AddEntryWithNullStringIsNoOp) {
    // 1:1 with legacy: the legacy uses strlen(buff) to skip
    // empty lines, so a null input is never seen. Modern port
    // makes null a safe no-op (no crash, no state change).
    mxh::ui::cHyperTextList h;
    h.AddEntry(42, nullptr);
    EXPECT_EQ(h.GetCount(), 0u);
    EXPECT_EQ(h.GetHyperText(42), nullptr);
}

TEST(CHyperTextList, AddEntryWithEmptyStringIsValid) {
    // The legacy skips empty lines (strlen(buff) == 0). The
    // modern port stores empty strings so the test layer
    // can verify the boundary explicitly. cHelpDialog never
    // produces empty entries (the file parser filters them).
    mxh::ui::cHyperTextList h;
    h.AddEntry(42, "");
    EXPECT_EQ(h.GetCount(), 1u);
    auto* p = h.GetHyperText(42);
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->str, "");
}

TEST(CHyperTextList, AddEntryStoresDefaultsForNonStringFields) {
    // 1:1 with legacy: the legacy uses pTemp->Init() which
    // zeros all fields. Modern DIALOGUE::Init() sets the
    // same defaults. The legacy file parser only sets str;
    // dwColor, wLine, wType keep their Init() defaults.
    mxh::ui::cHyperTextList h;
    h.AddEntry(42, "msg");
    auto* p = h.GetHyperText(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->dwColor, 0xFFFFFFFFu);
    EXPECT_EQ(p->wLine, 0u);
    EXPECT_EQ(p->wType, 0u);
}

TEST(CHyperTextList, AddEntryWithLongStringTruncates) {
    mxh::ui::cHyperTextList h;
    std::string longStr(2000, 'x');
    h.AddEntry(42, longStr.c_str());
    auto* p = h.GetHyperText(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(std::strlen(p->str), 1023u);  // truncated to DIALOGUE::str size - 1
}

TEST(CHyperTextList, AddEntryWithExactSizeStringFits) {
    mxh::ui::cHyperTextList h;
    std::string s(1023, 'a');  // 1023 chars + NUL = 1024 bytes
    h.AddEntry(42, s.c_str());
    auto* p = h.GetHyperText(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(std::strlen(p->str), 1023u);
}

TEST(CHyperTextList, RemoveAllClearsEntries) {
    mxh::ui::cHyperTextList h;
    h.AddEntry(1, "a");
    h.AddEntry(2, "b");
    h.AddEntry(3, "c");
    EXPECT_EQ(h.GetCount(), 3u);
    h.RemoveAll();
    EXPECT_EQ(h.GetCount(), 0u);
    EXPECT_EQ(h.GetHyperText(1), nullptr);
    EXPECT_EQ(h.GetHyperText(2), nullptr);
    EXPECT_EQ(h.GetHyperText(3), nullptr);
}

TEST(CHyperTextList, AddEntryAfterRemoveAllReusesIds) {
    // 1:1 with legacy: RemoveAll clears the table; new
    // AddEntry calls populate fresh entries.
    mxh::ui::cHyperTextList h;
    h.AddEntry(1, "first");
    h.AddEntry(2, "second");
    h.RemoveAll();
    h.AddEntry(1, "reused");
    EXPECT_EQ(h.GetCount(), 1u);
    auto* p = h.GetHyperText(1);
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->str, "reused");
    EXPECT_EQ(h.GetHyperText(2), nullptr);
}

TEST(CHyperTextList, ManyEntriesPreserveLookup) {
    mxh::ui::cHyperTextList h;
    for (std::uint32_t i = 0; i < 100; ++i) {
        std::string s = "msg_" + std::to_string(i);
        h.AddEntry(i, s.c_str());
    }
    EXPECT_EQ(h.GetCount(), 100u);
    for (std::uint32_t i = 0; i < 100; ++i) {
        auto* p = h.GetHyperText(i);
        ASSERT_NE(p, nullptr);
        std::string expected = "msg_" + std::to_string(i);
        EXPECT_STREQ(p->str, expected.c_str());
    }
}

TEST(CHyperTextList, NonContiguousKeysSupported) {
    // 1:1 with legacy: CYHHashTable accepts any DWORD key;
    // modern std::unordered_map does the same.
    mxh::ui::cHyperTextList h;
    h.AddEntry(0, "zero");
    h.AddEntry(100, "hundred");
    h.AddEntry(1000, "thousand");
    h.AddEntry(UINT32_MAX, "max");
    EXPECT_EQ(h.GetCount(), 4u);
    EXPECT_STREQ(h.GetHyperText(0)->str, "zero");
    EXPECT_STREQ(h.GetHyperText(100)->str, "hundred");
    EXPECT_STREQ(h.GetHyperText(1000)->str, "thousand");
    EXPECT_STREQ(h.GetHyperText(UINT32_MAX)->str, "max");
}

TEST(CHyperTextList, NotCopyable) {
    // 1:1 with the legacy not-copyable-by-default behavior.
    // Modern port deletes copy ctor + copy assign to enforce
    // ownership uniqueness of the unique_ptr-backed map.
    static_assert(!std::is_copy_constructible_v<mxh::ui::cHyperTextList>);
    static_assert(!std::is_copy_assignable_v<mxh::ui::cHyperTextList>);
}
