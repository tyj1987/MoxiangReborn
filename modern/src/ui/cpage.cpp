// cpage.cpp — modern port implementation.
//
// 1:1 port of legacy `cPage` from
//   `墨香【源码】\[Client]MH\cPage.cpp` (1.5 KB legacy).
//
// Modern-port notes
// =================
//
// 1. **cLinkedList → std::vector.** Legacy uses cLinkedList
//    (engine-side doubly-linked list) for m_pDialogue and
//    m_pHyperLink. Modern uses std::vector; iteration order
//    is preserved (head-to-tail).
//
// 2. **rand() → test-injectable counter.** Legacy uses
//    `rand()%m_nDialogueCount` for the random dialogue pick.
//    Modern port uses a test-injectable counter (g_rand_value)
//    so tests are reproducible. The engine-binder layer
//    (Phase 14+) re-adds a real RNG.
//
// 3. **HYPERLINK is opaque stub.** The legacy HYPERLINK struct
//    is engine-side; modern uses a placeholder struct with
//    the same field names. The engine-binder layer provides
//    the real type when the dialog is wired.

#include "cpage.hpp"

#include <cstring>

namespace mxh::ui {

namespace {
// Test-only random override. nullptr = use default fallback.
std::uint32_t (*g_rand_fn)() = nullptr;
std::uint32_t default_rand() noexcept {
    // Deterministic fallback: returns 0. The legacy uses
    // rand() which is non-deterministic; modern is
    // deterministic for tests.
    return 0;
}
}  // namespace

void SetPageRandomForTesting(std::uint32_t (*fn)()) {
    g_rand_fn = fn;
}

cPageBase::cPageBase() {
    m_nDialogueCount = 0;
    m_nNextPageId    = -1;
    m_nPrevPageId    = -1;
}

cPageBase::~cPageBase() {
    RemoveAll();
}

void cPageBase::Init(std::uint32_t dwId) noexcept {
    m_dwPageId = dwId;
}

void cPageBase::AddDialogue(std::uint32_t dwId) {
    m_dialogueIds.push_back(dwId);
    m_nDialogueCount = static_cast<int>(m_dialogueIds.size());
}

void cPageBase::RemoveAll() noexcept {
    m_dialogueIds.clear();
    m_nDialogueCount = 0;
    m_nNextPageId = -1;
    m_nPrevPageId = -1;
    m_dwPageId = 0;
}

std::uint32_t cPageBase::GetRandomDialogue() noexcept {
    if (m_nDialogueCount == 0) return 0;
    if (m_nDialogueCount == 1) {
        return m_dialogueIds[0];
    }
    // 1:1 with legacy: rand()%count. Modern uses the
    // test-injectable counter.
    std::uint32_t r = g_rand_fn ? g_rand_fn() : default_rand();
    std::size_t idx = static_cast<std::size_t>(r) % static_cast<std::size_t>(m_nDialogueCount);
    return m_dialogueIds[idx];
}

// ---- cPage ----

cPage::cPage() {
    m_nHyperLinkCount = 0;
}

cPage::~cPage() {
    RemoveAll();
}

void cPage::AddHyperLink(const HYPERLINK* pLink) {
    if (!pLink) return;
    HYPERLINK copy = *pLink;
    m_hyperLinks.push_back(copy);
    m_nHyperLinkCount = static_cast<int>(m_hyperLinks.size());
}

void cPage::RemoveAll() noexcept {
    cPageBase::RemoveAll();
    m_hyperLinks.clear();
    m_nHyperLinkCount = 0;
}

const HYPERLINK* cPage::GetHyperText(std::uint32_t dwIdx) const noexcept {
    if (dwIdx >= m_hyperLinks.size()) return nullptr;
    return &m_hyperLinks[dwIdx];
}

} // namespace mxh::ui
