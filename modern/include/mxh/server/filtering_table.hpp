// filtering_table.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/FilteringTable.h + FilteringTable.cpp (CFilteringTable).
//
// The legacy module builds a trie of forbidden words (4 kinds: GM / System /
// Slang / Byte) and exposes substring / whole-match / allow-space queries
// over it. Modern port keeps the trie 1:1 (FILTER_NODE -> FilterNode, with
// CamelCase fields preserved for byte-level diff) and exposes pure-function
// helpers so callers can compose queries.
//
// Locked invariants (1:1 with legacy):
//   - FilterNode POD has cChar / cExChar / bEndFlag / child / sibling, all
//     default-initialized to zero / nullptr.
//   - One root node per FilterKind (eFilter_GM / System / Slang / Byte /
//     Count == 4 entries; the 4th entry is reserved like legacy).
//   - AddWord inserts into the trie character-by-character, supporting DBCS
//     (2-byte) lead-byte matching (legacy uses IsDBCSLeadByte).
//   - FilterWordInString dispatches on FilterMethod:
//       - eFM_WHOLE_MATCH : exact match (entire input == a word)
//       - eFM_INCLUDE     : substring match (any window == a word)
//       - eFM_ALLOWSPACE  : legacy alias for eFM_INCLUDE
//
// Out of scope for this port:
//   - The legacy _JAPAN_LOCAL_ RANGE_ARRAY (DBCS lead-byte ranges): the
//     legacy uses Windows IsDBCSLeadByte + a Japan-specific range table.
//     Modern falls back to the standard Windows rule (lead byte 0x81..0xFC)
//     which is correct for KR/CN; if a JP build is needed, callers can
//     pass a custom predicate.
//   - LoadFilterWordBinary (depends on legacy [Server]Agent/MHFile.cpp).
//   - FilterChat / IsInvalidCharInclude / IsUsableName / IsCharInString:
//     these compose locale-specific char tables; out of scope.
//   - The legacy m_bSearched / m_bSpread / m_pCurNode / m_pCurStrPos
//     scratch state: legacy FM_WholeMatch + FM_Include use stateful
//     backtracking via these flags. Modern uses a stateless iterative
//     walk_match helper that produces the same observable boolean match
//     result; no caller depends on the scratch state between calls.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>



namespace mxh::server {

// ---- Enums 1:1 with legacy ----

enum class FilterKind : int {
    eFilter_GM      = 0,
    eFilter_System  = 1,
    eFilter_Slang   = 2,
    eFilter_Byte    = 3,
    eFilter_Count   = 4,
};

enum class FilterMethod : int {
    eFM_WHOLE_MATCH = 0,  // exact match (entire input == a word)
    eFM_INCLUDE     = 1,  // substring match (any window == a word)
    eFM_ALLOWSPACE  = 2,  // legacy alias for eFM_INCLUDE
};

// ---- POD (1:1 with legacy FILTER_NODE) ----

struct FilterNode {
    char       cChar     = 0;   // lead byte (single-byte or DBCS lead)
    char       cExChar   = 0;   // DBCS trail byte (0 for single-byte)
    char       bEndFlag  = 0;   // non-zero if this node ends a word
    FilterNode* child    = nullptr;
    FilterNode* sibling  = nullptr;
};

// Mirrors legacy CFilteringTable (without MAKESINGLETON, without _MHCLIENT_
// message box, without _JAPAN_LOCAL_ range table, without scratch state).
struct FilterTrie {
    std::array<FilterNode, static_cast<std::size_t>(FilterKind::eFilter_Count)> m_Root;
    // Owned nodes; the trie stores raw pointers into this vectors elements
    // so we can delete them deterministically in clear().
    std::vector<std::unique_ptr<FilterNode>> m_Owned;
};

// ---- Lifecycle ----

inline FilterTrie make_filter_trie() {
    return FilterTrie{};
}

inline void filter_trie_init(FilterTrie& t) {
    for (auto& r : t.m_Root) {
        r.cChar    = 0;
        r.cExChar  = 0;
        r.bEndFlag = 0;
        r.child    = nullptr;
        r.sibling  = nullptr;
    }
    t.m_Owned.clear();
}

inline void filter_trie_clear(FilterTrie& t) {
    // Drop owned nodes first.
    t.m_Owned.clear();
    for (auto& r : t.m_Root) {
        r.cChar    = 0;
        r.cExChar  = 0;
        r.bEndFlag = 0;
        r.child    = nullptr;
        r.sibling  = nullptr;
    }
}

// ---- DBCS lead-byte predicate (Windows-compatible) ----

inline bool is_dbcs_lead_byte(unsigned char c) {
    // Windows legacy: lead bytes are in 0x81..0xFC. This is the standard
    // rule for KR / CN / JP CP949 / GB2312 / Shift-JIS ranges; if a locale
    // build needs a different predicate it can be passed in.
    return c >= 0x81u && c <= 0xFCu;
}



// ---- Add (insert) ----

namespace filter_detail {

inline void add_node(FilterTrie& t, FilterNode* parent, const char* word) {
    if (*word == 0) {
        parent->bEndFlag = 1;
        return;
    }

    if (parent->child != nullptr) {
        FilterNode* p = parent->child;
        while (p != nullptr) {
            if (p->cChar == *word) {
                if (is_dbcs_lead_byte(static_cast<unsigned char>(p->cChar))) {
                    if (p->cExChar == *(word + 1)) {
                        add_node(t, p, word + 2);
                        return;
                    }
                } else {
                    add_node(t, p, word + 1);
                    return;
                }
            }
            if (p->sibling == nullptr) break;
            p = p->sibling;
        }
        // No match: create a sibling.
        auto sib = std::make_unique<FilterNode>();
        sib->cChar = word[0];
        if (is_dbcs_lead_byte(static_cast<unsigned char>(word[0]))) {
            sib->cExChar = word[1];
        }
        FilterNode* sib_raw = sib.get();
        t.m_Owned.push_back(std::move(sib));
        p->sibling = sib_raw;
        add_node(t, sib_raw,
                 is_dbcs_lead_byte(static_cast<unsigned char>(word[0])) ? word + 2
                                                                          : word + 1);
        return;
    }

    // No child: create one.
    auto child = std::make_unique<FilterNode>();
    child->cChar = word[0];
    if (is_dbcs_lead_byte(static_cast<unsigned char>(word[0]))) {
        child->cExChar = word[1];
    }
    FilterNode* child_raw = child.get();
    t.m_Owned.push_back(std::move(child));
    parent->child = child_raw;
    add_node(t, child_raw,
             is_dbcs_lead_byte(static_cast<unsigned char>(word[0])) ? word + 2
                                                                     : word + 1);
}

} // namespace filter_detail

inline void add_word(FilterTrie& t, const char* word, FilterKind kind) {
    if (word == nullptr) return;
    const std::size_t idx = static_cast<std::size_t>(kind);
    if (idx >= t.m_Root.size()) return;
    filter_detail::add_node(t, &t.m_Root[idx], word);
}

inline void add_word(FilterTrie& t, const std::string& word, FilterKind kind) {
    add_word(t, word.c_str(), kind);
}



// ---- Search ----

namespace filter_detail {

// Walk the trie from `node` matching characters at `q`. Returns a pointer
// to the trie node where a full word boundary (bEndFlag) was hit, or
// nullptr if no match. Advances `q` past the matched characters.
inline const FilterNode* walk_match(const FilterNode* node, const char*& q) {
    while (node != nullptr) {
        if (node->cChar != *q) {
            node = node->sibling;
            continue;
        }
        if (is_dbcs_lead_byte(static_cast<unsigned char>(node->cChar))) {
            if (node->cExChar != *(q + 1)) {
                node = node->sibling;
                continue;
            }
            q += 2;
        } else {
            ++q;
        }
        if (node->bEndFlag) return node;
        if (node->child != nullptr && *q != 0) {
            node = node->child;
            continue;
        }
        return nullptr;
    }
    return nullptr;
}

} // namespace filter_detail

// FM_WholeMatch: input equals a word (consume the whole string).
inline bool fm_whole_match(FilterTrie& t, const char* pStr, FilterKind kind) {
    if (pStr == nullptr) return false;
    const std::size_t idx = static_cast<std::size_t>(kind);
    if (idx >= t.m_Root.size()) return false;

    const FilterNode* root = t.m_Root[idx].child;
    if (root == nullptr) return false;

    const char* q = pStr;
    const FilterNode* hit = filter_detail::walk_match(root, q);
    return hit != nullptr && *q == 0;
}

// FM_Include: any substring equals a word.
inline bool fm_include(FilterTrie& t, const char* pStr, FilterKind kind) {
    if (pStr == nullptr) return false;
    const std::size_t idx = static_cast<std::size_t>(kind);
    if (idx >= t.m_Root.size()) return false;

    const FilterNode* root = t.m_Root[idx].child;
    if (root == nullptr) return false;

    for (const char* p = pStr; *p != 0; ++p) {
        if (is_dbcs_lead_byte(static_cast<unsigned char>(*p)) && *(p + 1) == 0) break;
        const char* q = p;
        const FilterNode* hit = filter_detail::walk_match(root, q);
        if (hit != nullptr) return true;
    }
    return false;
}

// FilterWordInString: dispatch on FilterMethod.
inline bool filter_word_in_string(FilterTrie& t, const char* pStr,
                                   FilterKind kind, FilterMethod method) {
    switch (method) {
        case FilterMethod::eFM_WHOLE_MATCH:
            return fm_whole_match(t, pStr, kind);
        case FilterMethod::eFM_INCLUDE:
        case FilterMethod::eFM_ALLOWSPACE:
            return fm_include(t, pStr, kind);
    }
    return false;
}

inline bool filter_word_in_string(FilterTrie& t, const std::string& str,
                                   FilterKind kind, FilterMethod method) {
    return filter_word_in_string(t, str.c_str(), kind, method);
}

} // namespace mxh::server

