// cListDialogEx.hpp — modern port of 墨香 cListDialogEx
// (text list with WE_ROWCLICK callback on link items + selected-row
// highlight + multi-color link chains).
//
// 1:1 port of legacy `cListDialogEx` from
//   `墨香【源码】\[Client]MH\cListDialogEx.h` (310 B) and
//   `墨香【源码】\[Client]MH\cListDialogEx.cpp` (4,371 B).
//
// Extends the base `cListDialog` (already 1:1 ported) with two
// behavior differences (1:1 from the legacy .cpp):
//
//   1. `ListMouseCheck(x, y, we)` — when WE_LBTNCLICK is received
//      for a row whose item `dwType > emLink_Null`, the dialog
//      fires WE_ROWCLICK via the parent window's callback. The
//      base `cListDialog` does not have this; legacy callers
//      expecting WE_ROWCLICK on a list (HelpDialog, PartyDialog,
//      JournalDialog, etc.) must use cListDialogEx.
//
//   2. `Render()` — draws the selected row with a highlight
//      background image (`m_OverImage` in legacy), and supports
//      multi-color "link chain" items (where a static-text row
//      with `dwType <= emLink_Null` can have a `NextItem`
//      pointer for inline next-color text appended to the
//      right). Modern port stores the next pointer via
//      `std::shared_ptr<LinkItem>` so the dialog can hold
//      chains without manual memory management.
//
// The data model is duplicated (m_linkItems shadows the base
// cListDialog::m_rows) rather than refactored into the base
// because the legacy has the same split: cListDialog stores
// "raw rows", cListDialogEx has the link item variant. Adding
// link items to the base would force every existing
// cListDialog consumer (cGuildDialog, cChatDialog, etc.) to
// pay the LinkItem overhead for no benefit. Keeping the
// cListDialogEx subclass self-contained preserves 1:1 API
// behavior with the legacy engine and the existing tests.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this is
// the second Tier 1.5 subcontrol port (after cGuagen in
// 0.13.12). It unblocks Tier 2 dialogs that need clickable
// link lists: HelpDialog, PartyDialog (member list),
// JournalDialog, etc.

#pragma once

#include "cListDialog.hpp"
#include "legacy_window_event.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mxh::ui {

// Window event flags (subset relevant to cListDialogEx's
// ListMouseCheck). These mirror the legacy [Client]MH/WindowIDEnum.h
// WE_* bit definitions, but we only define the ones we need here
// to avoid pulling in the full WindowIDEnum surface. The full
// enum will land in modern/include/mxh/ui/WindowIDEnum.hpp when
// the first P2-12 dialog port needs it (P2-12 Tier 1.5 work).
constexpr std::uint32_t WE_LBTNCLICK = legacy_window_event::kLeftButtonClick;
constexpr std::uint32_t WE_ROWCLICK = legacy_window_event::kRowClick;

class cListDialogEx : public cListDialog {
public:
    cListDialogEx();
    ~cListDialogEx() override;

    // Link item type (mirrors legacy emLink_Null enum).
    // dwType == 0 = static text (no click, may have NextItem chain).
    // dwType >  0 = link item (clickable, fires WE_ROWCLICK).
    enum LinkType : std::uint8_t {
        emLink_Null     = 0,
        emLink_Default  = 1,
        emLink_Image    = 2,   // legacy distinction reserved for future
        emLink_Hyper    = 3,   // legacy distinction reserved for future
    };

    struct LinkItem {
        std::string              text;
        std::uint32_t            color     = 0xFF000000;  // ARGB
        std::uint32_t            overColor = 0xFF000000;  // ARGB (selected state)
        std::uint8_t             type      = emLink_Null;
        std::shared_ptr<LinkItem> next;                    // multi-color chain
    };

    // ----- Init / data -----

    // Reset and configure the list. Mirrors cListDialog::InitList
    // but with no clip (the legacy cListDialogEx uses parent
    // dialog positioning instead of an internal clip rect).
    void InitLinkList(std::uint16_t maxLines);

    // Add a simple link item at position `line` (default = append).
    // `type` defaults to emLink_Default so WE_ROWCLICK is fired
    // on click.
    void AddLinkItem(const std::string& text,
                     std::uint8_t type = emLink_Default,
                     std::uint32_t color = 0xFF000000,
                     std::uint32_t overColor = 0xFFFF0000,
                     int line = -1);

    // Add a static text row with a multi-color chain: the head
    // item shows first, and each `next` item is rendered
    // immediately to its right in its own color (used by
    // HelpDialog's rich-text entries: "Prefix <link>suffix</link>").
    void AddLinkItemChain(const LinkItem& head);

    // Remove all items. Clears m_linkItems AND the base
    // cListDialog::m_rows (so the dialog looks empty even if
    // the host also added base rows separately).
    //
    // Note: cListDialog::RemoveAll is non-virtual, so this
    // is **name-hide** rather than virtual override (the
    // same pattern flagged in KNOWN_BUGS.md R-12 for
    // cExitDialog). If a host ever holds a cDialog* /
    // cListDialog* base pointer and calls RemoveAll() on
    // it, the base implementation will run instead of this
    // one. The fix is to make cListDialog::RemoveAll
    // virtual; tracked as R-12 follow-up. For the
    // cListDialogEx port the host always uses the
    // cListDialogEx* type, so the hide works in practice.
    void RemoveAll() noexcept;

    // ----- Behavior -----

    // Mouse check (1:1 with legacy cListDialogEx::ListMouseCheck).
    // Returns the row index under (x, y) (delegates to
    // cListDialog::PtIdxInRow) and updates the selected row.
    // On WE_LBTNCLICK over a link item (type > emLink_Null),
    // sets `m_rowClicked = true` so the host can call
    // ConsumeRowClicked() to check whether WE_ROWCLICK should
    // be dispatched via the parent window's callback.
    //
    // The modern port does NOT auto-dispatch WE_ROWCLICK
    // through the window-tree callback (the legacy does this
    // via `cbWindowFunc` which is a static function pointer
    // with no easy modern equivalent). The host dialog is
    // expected to query ConsumeRowClicked() inside its own
    // event handler and dispatch the WE_ROWCLICK semantically
    // (e.g. by calling a std::function callback registered
    // via SetOnRowClicked).
    int ListMouseCheck(std::int32_t x, std::int32_t y, std::uint32_t we) noexcept;

    // Render placeholder (1:1 with legacy — base does no-op,
    // cListDialogEx draws selected highlight + link chains).
    // The actual sprite draws are deferred to the 6.4+ cImage
    // / Phase 13 host integration (same as cGuagen's RenderIsNoop).
    void Render() override {}

    // ----- Callback hook -----

    using RowClickedCallback = void(*)(std::int32_t rowIdx, void* userData);

    // Register a callback fired when a link row is clicked.
    // The callback receives the row index (0-based into
    // m_linkItems) and the userData pointer. Modern
    // replacement for the legacy `cbWindowFunc` static
    // dispatch.
    void SetOnRowClicked(RowClickedCallback cb, void* userData) noexcept {
        m_onRowClicked = cb;
        m_rowClickedUserData = userData;
    }

    // Called by the host after ListMouseCheck. Returns true
    // exactly once per click on a link item, then resets the
    // internal flag. Returns false if no click happened or
    // the click was on a non-link row.
    bool ConsumeRowClicked() noexcept {
        bool r = m_rowClicked;
        m_rowClicked = false;
        return r;
    }

    // ----- Accessors -----

    std::size_t LinkItemCount() const noexcept { return m_linkItems.size(); }
    const LinkItem& GetLinkItemAt(std::size_t i) const noexcept { return m_linkItems.at(i); }

private:
    std::vector<LinkItem> m_linkItems;
    RowClickedCallback    m_onRowClicked       = nullptr;
    void*                 m_rowClickedUserData = nullptr;
    bool                  m_rowClicked         = false;
};

}  // namespace mxh::ui
