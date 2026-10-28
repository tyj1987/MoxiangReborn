// cpage.hpp — modern port of 墨香 cPage (help-dialog page data
// model: a page holds a list of dialogue ids + a list of
// hyperlinks; the cHelpDialog iterates a page to render the help
// text + clickable links).
//
// 1:1 port of legacy `cPage` from
//   `墨香【源码】\[Client]MH\cPage.{h,cpp}` (1.5 KB legacy
//   code; modern port preserves the data model + state machine).
//
// Modern port scope (this commit):
//   - cPageBase: m_dwPageId, m_dialogueIds (std::vector<DWORD>),
//     m_nDialogueCount, m_nNextPageId, m_nPrevPageId.
//   - cPage extends cPageBase: m_hyperLinks (std::vector<HYPERLINK>),
//     m_nHyperLinkCount.
//   - Init / AddDialogue / RemoveAll / GetPageId /
//     GetRandomDialogue / Get/SetNextPageId / Get/SetPrevPageId.
//   - AddHyperLink / GetHyperLinkCount / GetHyperText.
//   - HYPERLINK struct stub (1:1 with legacy fields).
//
// Modern-port simplifications (all documented in the .cpp file
// header):
// 1. **cLinkedList → std::vector.** The legacy uses cLinkedList
//    (engine-side linked list) for m_pDialogue and m_pHyperLink.
//    Modern uses std::vector. Iteration semantics preserved 1:1.
// 2. **rand() → modern <random>.** The legacy uses rand()%count
//    for GetRandomDialogue. Modern port uses a deterministic
//    test-injectable counter (g_rand_value) so tests are
//    reproducible; the engine-binder layer (Phase 14+) will
//    re-add the random source.
// 3. **HYPERLINK is opaque stub.** The legacy HYPERLINK is
//    engine-side; modern port declares a placeholder struct
//    with the same field names (the engine-binder layer
//    provides the real type when the dialog is wired).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::ui {

// HYPERLINK is the engine-side link payload (1:1 with legacy
// `HYPERLINK` from `[CC]Header/CommonGameStruct.h`). The
// cHelpDialog / cNpcScriptDialog chain reads wLinkId +
// wLinkType + dwData; cPage just stores + retrieves the struct
// (the consumer interprets the fields). Modern port uses
// std::uint16_t / std::uint32_t to match the legacy WORD /
// DWORD types; the engine-binder layer (Phase 14+) provides
// the real type when the help-dialog content is loaded.
struct HYPERLINK {
    std::uint16_t wLinkId   = 0;  // index into cHyperTextList
    std::uint16_t wLinkType = 0;  // emLink_Page / emLink_Open / emLink_End / etc.
    std::uint32_t dwData    = 0;  // target page id (for emLink_Page) / etc.
};

class cPageBase {
public:
    cPageBase();
    virtual ~cPageBase();

    cPageBase(const cPageBase&) = delete;
    cPageBase& operator=(const cPageBase&) = delete;

    // -------------------------------------------------------------------------
    // Init: set the page id. Other state (dialogue list, next/prev
    // ids) is initialized in the ctor.
    // -------------------------------------------------------------------------
    virtual void Init(std::uint32_t dwId) noexcept;

    // AddDialogue: append a dialogue id. The legacy uses
    // cLinkedList::AddTail with a heap-allocated DWORD. Modern
    // uses std::vector::push_back (no manual heap alloc).
    void AddDialogue(std::uint32_t dwId);

    // RemoveAll: clear all dialogues + hyperlinks.
    virtual void RemoveAll() noexcept;

    std::uint32_t GetPageId() const noexcept          { return m_dwPageId; }
    int           GetDialogueCount() const noexcept  { return m_nDialogueCount; }

    // GetRandomDialogue: 1:1 with legacy. Returns a random
    // dialogue id from the page. The legacy uses rand()%count
    // for m_nDialogueCount > 1, else returns the only entry.
    // Modern port uses a test-injectable counter; the
    // engine-binder layer (Phase 14+) provides a real RNG.
    std::uint32_t GetRandomDialogue() noexcept;

    void SetNextPageId(int id) noexcept { m_nNextPageId = id; }
    void SetPrevPageId(int id) noexcept { m_nPrevPageId = id; }
    int  GetNextPageId() const noexcept { return m_nNextPageId; }
    int  GetPrevPageId() const noexcept { return m_nPrevPageId; }

protected:
    std::uint32_t          m_dwPageId = 0;
    std::vector<std::uint32_t> m_dialogueIds;
    int                    m_nDialogueCount = 0;
    int                    m_nNextPageId = -1;
    int                    m_nPrevPageId = -1;
};

class cPage : public cPageBase {
public:
    cPage();
    ~cPage() override;

    cPage(const cPage&) = delete;
    cPage& operator=(const cPage&) = delete;

    // -------------------------------------------------------------------------
    // AddHyperLink: append a hyperlink. The legacy deep-copies
    // the HYPERLINK struct (memcpy). Modern port uses
    // std::vector::push_back on a HYPERLINK copy.
    // -------------------------------------------------------------------------
    void AddHyperLink(const HYPERLINK* pLink);

    // RemoveAll: clear all dialogues + hyperlinks.
    void RemoveAll() noexcept override;

    int                GetHyperLinkCount() const noexcept { return m_nHyperLinkCount; }
    const HYPERLINK*   GetHyperText(std::uint32_t dwIdx) const noexcept;

private:
    std::vector<HYPERLINK> m_hyperLinks;
    int                    m_nHyperLinkCount = 0;
};

// Test-only: inject a deterministic "random" value for
// GetRandomDialogue. Pass nullptr to restore the default
// (returns 0 when count is 1, or the first dialogue id when
// count > 1 — deterministic fallback).
void SetPageRandomForTesting(std::uint32_t (*fn)());

} // namespace mxh::ui
