// filtering_table_test.cpp - Phase 6.3 FilteringTable 1:1 port tests.

#include "mxh/server/filtering_table.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::FilterKind;
using mxh::server::FilterMethod;
using mxh::server::FilterNode;
using mxh::server::FilterTrie;
using mxh::server::add_word;
using mxh::server::filter_trie_clear;
using mxh::server::filter_trie_init;
using mxh::server::filter_word_in_string;
using mxh::server::fm_include;
using mxh::server::fm_whole_match;
using mxh::server::is_dbcs_lead_byte;
using mxh::server::make_filter_trie;

} // namespace

// ---- Enums 1:1 ----

TEST(FilteringTableEnum, FilterKindValuesMatchLegacy) {
    EXPECT_EQ(static_cast<int>(FilterKind::eFilter_GM),     0);
    EXPECT_EQ(static_cast<int>(FilterKind::eFilter_System), 1);
    EXPECT_EQ(static_cast<int>(FilterKind::eFilter_Slang),  2);
    EXPECT_EQ(static_cast<int>(FilterKind::eFilter_Byte),   3);
    EXPECT_EQ(static_cast<int>(FilterKind::eFilter_Count),  4);
}

TEST(FilteringTableEnum, FilterMethodValuesMatchLegacy) {
    EXPECT_EQ(static_cast<int>(FilterMethod::eFM_WHOLE_MATCH), 0);
    EXPECT_EQ(static_cast<int>(FilterMethod::eFM_INCLUDE),     1);
    EXPECT_EQ(static_cast<int>(FilterMethod::eFM_ALLOWSPACE),  2);
}

// ---- DBCS helper ----

TEST(FilteringTableDBCS, IsDBCSLeadByteRange) {
    EXPECT_FALSE(is_dbcs_lead_byte(0x00u));
    EXPECT_FALSE(is_dbcs_lead_byte(0x7Fu));
    EXPECT_FALSE(is_dbcs_lead_byte(0x80u));
    EXPECT_TRUE (is_dbcs_lead_byte(0x81u));
    EXPECT_TRUE (is_dbcs_lead_byte(0xB0u));
    EXPECT_TRUE (is_dbcs_lead_byte(0xFCu));
    EXPECT_FALSE(is_dbcs_lead_byte(0xFDu));
    EXPECT_FALSE(is_dbcs_lead_byte(0xFFu));
}

// ---- POD 1:1 ----

TEST(FilteringTablePOD, FilterNodeDefaultsAreZero) {
    FilterNode n;
    EXPECT_EQ(n.cChar, 0);
    EXPECT_EQ(n.cExChar, 0);
    EXPECT_EQ(n.bEndFlag, 0);
    EXPECT_EQ(n.child, nullptr);
    EXPECT_EQ(n.sibling, nullptr);
}

// ---- Lifecycle ----

TEST(FilteringTableInit, MakeFilterTrieHasFourEmptyRoots) {
    auto t = make_filter_trie();
    EXPECT_EQ(t.m_Root.size(), 4u);
    for (const auto& r : t.m_Root) {
        EXPECT_EQ(r.child, nullptr);
        EXPECT_EQ(r.cChar, 0);
    }
    EXPECT_TRUE(t.m_Owned.empty());
}

TEST(FilteringTableInit, FilterTrieInitResetsAfterUse) {
    auto t = make_filter_trie();
    add_word(t, "BADWORD", FilterKind::eFilter_Slang);
    EXPECT_FALSE(t.m_Owned.empty());

    filter_trie_init(t);

    for (const auto& r : t.m_Root) {
        EXPECT_EQ(r.child, nullptr);
    }
    EXPECT_TRUE(t.m_Owned.empty());
}

// ---- Add: insert + idempotent ----

TEST(FilteringTableAdd, AddSingleWordMakesMatch) {
    auto t = make_filter_trie();
    add_word(t, "BAD", FilterKind::eFilter_Slang);
    EXPECT_TRUE(fm_include(t, "BAD", FilterKind::eFilter_Slang));
}

TEST(FilteringTableAdd, AddEmptyWordIsNoEffect) {
    auto t = make_filter_trie();
    add_word(t, "", FilterKind::eFilter_GM);
    EXPECT_FALSE(fm_whole_match(t, "", FilterKind::eFilter_GM));
    EXPECT_FALSE(fm_include(t, "", FilterKind::eFilter_GM));
    EXPECT_EQ(t.m_Root[0].child, nullptr);
}

TEST(FilteringTableAdd, AddMultipleWordsBuildsSiblings) {
    auto t = make_filter_trie();
    add_word(t, "BAD",   FilterKind::eFilter_Slang);
    add_word(t, "BASTARD", FilterKind::eFilter_Slang);
    add_word(t, "BOO",   FilterKind::eFilter_Slang);
    EXPECT_TRUE(fm_include(t, "BAD",    FilterKind::eFilter_Slang));
    EXPECT_TRUE(fm_include(t, "BASTARD",FilterKind::eFilter_Slang));
    EXPECT_TRUE(fm_include(t, "BOO",    FilterKind::eFilter_Slang));
}

TEST(FilteringTableAdd, AddWordIsIdempotent) {
    auto t = make_filter_trie();
    add_word(t, "DUPE", FilterKind::eFilter_Slang);
    std::size_t n_after_first = t.m_Owned.size();
    add_word(t, "DUPE", FilterKind::eFilter_Slang);
    EXPECT_EQ(t.m_Owned.size(), n_after_first);
    EXPECT_TRUE(fm_include(t, "DUPE", FilterKind::eFilter_Slang));
}

TEST(FilteringTableAdd, AddToDifferentKindsIsolates) {
    auto t = make_filter_trie();
    add_word(t, "FOO", FilterKind::eFilter_Slang);
    EXPECT_FALSE(fm_include(t, "FOO", FilterKind::eFilter_GM));
    EXPECT_TRUE (fm_include(t, "FOO", FilterKind::eFilter_Slang));
}

// ---- FM_WholeMatch ----

TEST(FilteringTableWhole, ExactMatchOnly) {
    auto t = make_filter_trie();
    add_word(t, "FOO", FilterKind::eFilter_Slang);
    EXPECT_TRUE (fm_whole_match(t, "FOO",  FilterKind::eFilter_Slang));
    EXPECT_FALSE(fm_whole_match(t, "FOOBAR", FilterKind::eFilter_Slang));
    EXPECT_FALSE(fm_whole_match(t, "XFOO",  FilterKind::eFilter_Slang));
    EXPECT_FALSE(fm_whole_match(t, "BAR",  FilterKind::eFilter_Slang));
}

TEST(FilteringTableWhole, EmptyTrieReturnsFalse) {
    auto t = make_filter_trie();
    EXPECT_FALSE(fm_whole_match(t, "FOO", FilterKind::eFilter_Slang));
}

TEST(FilteringTableWhole, NullStrReturnsFalse) {
    auto t = make_filter_trie();
    add_word(t, "FOO", FilterKind::eFilter_Slang);
    EXPECT_FALSE(fm_whole_match(t, static_cast<const char*>(nullptr), FilterKind::eFilter_Slang));
}

// ---- FM_Include ----

TEST(FilteringTableInclude, SubstringMatch) {
    auto t = make_filter_trie();
    add_word(t, "FOO", FilterKind::eFilter_Slang);
    EXPECT_TRUE(fm_include(t, "XFOOX", FilterKind::eFilter_Slang));
    EXPECT_TRUE(fm_include(t, "FOO",   FilterKind::eFilter_Slang));
    EXPECT_TRUE(fm_include(t, "FOOBAR",FilterKind::eFilter_Slang));
    EXPECT_FALSE(fm_include(t, "BAR", FilterKind::eFilter_Slang));
}

TEST(FilteringTableInclude, EmptyTrieReturnsFalse) {
    auto t = make_filter_trie();
    EXPECT_FALSE(fm_include(t, "FOO", FilterKind::eFilter_Slang));
}

TEST(FilteringTableInclude, LongWordIncludesShortPrefix) {
    auto t = make_filter_trie();
    add_word(t, "BASTARD", FilterKind::eFilter_Slang);
    EXPECT_FALSE(fm_include(t, "B", FilterKind::eFilter_Slang));
    EXPECT_TRUE (fm_include(t, "BASTARD", FilterKind::eFilter_Slang));
    EXPECT_TRUE (fm_include(t, "XBASTARDX", FilterKind::eFilter_Slang));
}

// ---- FilterWordInString dispatch ----

TEST(FilteringTableDispatch, WholeMatchDispatch) {
    auto t = make_filter_trie();
    add_word(t, "FOO", FilterKind::eFilter_Slang);
    EXPECT_TRUE (filter_word_in_string(t, "FOO",  FilterKind::eFilter_Slang, FilterMethod::eFM_WHOLE_MATCH));
    EXPECT_FALSE(filter_word_in_string(t, "XFOO", FilterKind::eFilter_Slang, FilterMethod::eFM_WHOLE_MATCH));
}

TEST(FilteringTableDispatch, IncludeDispatch) {
    auto t = make_filter_trie();
    add_word(t, "FOO", FilterKind::eFilter_Slang);
    EXPECT_TRUE(filter_word_in_string(t, "XFOO", FilterKind::eFilter_Slang, FilterMethod::eFM_INCLUDE));
}

TEST(FilteringTableDispatch, AllowSpaceIsAliasForInclude) {
    auto t = make_filter_trie();
    add_word(t, "FOO", FilterKind::eFilter_Slang);
    EXPECT_TRUE(filter_word_in_string(t, "XFOO", FilterKind::eFilter_Slang, FilterMethod::eFM_ALLOWSPACE));
}

// ---- Clear ----

TEST(FilteringTableClear, ClearDropsAllWords) {
    auto t = make_filter_trie();
    add_word(t, "BAD",  FilterKind::eFilter_Slang);
    add_word(t, "EVIL", FilterKind::eFilter_Slang);
    EXPECT_TRUE(fm_include(t, "BAD", FilterKind::eFilter_Slang));

    filter_trie_clear(t);

    EXPECT_FALSE(fm_include(t, "BAD", FilterKind::eFilter_Slang));
    EXPECT_FALSE(fm_include(t, "EVIL",FilterKind::eFilter_Slang));
    for (const auto& r : t.m_Root) {
        EXPECT_EQ(r.child, nullptr);
    }
    EXPECT_TRUE(t.m_Owned.empty());
}

TEST(FilteringTableClear, ClearThenAddReusesTrie) {
    auto t = make_filter_trie();
    add_word(t, "OLD", FilterKind::eFilter_Slang);
    filter_trie_clear(t);
    add_word(t, "NEW", FilterKind::eFilter_Slang);
    EXPECT_FALSE(fm_include(t, "OLD", FilterKind::eFilter_Slang));
    EXPECT_TRUE (fm_include(t, "NEW", FilterKind::eFilter_Slang));
}

// ---- DBCS / 2-byte chars ----

TEST(FilteringTableDBCS, AddDBCSWordAndMatch) {
    auto t = make_filter_trie();
    const char word[] = { static_cast<char>(0xB0), static_cast<char>(0xA1), 0 };
    add_word(t, word, FilterKind::eFilter_Slang);
    EXPECT_TRUE(fm_whole_match(t, word, FilterKind::eFilter_Slang));
    EXPECT_TRUE(fm_include(t, word, FilterKind::eFilter_Slang));
}

TEST(FilteringTableDBCS, DBCSWordDoesNotMatchDifferentTrailByte) {
    auto t = make_filter_trie();
    const char word[]       = { static_cast<char>(0xB0), static_cast<char>(0xA1), 0 };
    const char different[]  = { static_cast<char>(0xB0), static_cast<char>(0xA2), 0 };
    add_word(t, word, FilterKind::eFilter_Slang);
    EXPECT_FALSE(fm_include(t, different, FilterKind::eFilter_Slang));
}

TEST(FilteringTableDBCS, DBCSWordMatchesInsideLongerString) {
    auto t = make_filter_trie();
    const char word[]    = { static_cast<char>(0xB0), static_cast<char>(0xA1), 0 };
    const char longer[]  = { 0x58, 0x59, static_cast<char>(0xB0), static_cast<char>(0xA1), 0x5A, 0x57, 0 };
    add_word(t, word, FilterKind::eFilter_Slang);
    EXPECT_TRUE(fm_include(t, longer, FilterKind::eFilter_Slang));
}

// ---- Add null ----

TEST(FilteringTableAdd, AddNullWordIsNoOp) {
    auto t = make_filter_trie();
    add_word(t, static_cast<const char*>(nullptr), FilterKind::eFilter_Slang);
    EXPECT_TRUE(t.m_Owned.empty());
}

