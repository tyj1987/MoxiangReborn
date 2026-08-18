// mxh/ui/cWindowManager.hpp
// Phase 6.6 — modern C++ cWindowManager. Top-level dispatcher that owns
// the active dialog list and routes input to the topmost active dialog.
// Builds on cDialog (6.3) + cWindow (6.0).
//
// Scope (this phase): the dispatcher core.
//   - AddDialog / RemoveDialog / RemoveDialogById
//   - findById / findByXY (top-most active dialog under cursor)
//   - Topmost dialog accessor (the one currently receiving input)
//   - ActionEvent / ActionKeyboardEvent fanout to the topmost dialog
//   - Modal mode: while a modal dialog is open, lower dialogs are blocked
//     from input (the modal is the only one that gets ActionEvent)
//   - Defer-destroy queue: dialogs marked closeRequested() are torn down
//     at the end of the next Process() tick to avoid mid-event deletes
//
// Deferred:
//   - Drag-and-drop icon support (legacy cbDROPPROCESSFUNC)
//   - cScriptManager integration (the .bin UI description loader)
//   - ChatTooltip / SStitletip specialized layers
//   - Window z-order persistence across save/load
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "cWindow.hpp"

namespace mxh::ui {

class cDialog;
class cEditBox;

class cWindowManager {
public:
    cWindowManager() = default;
    ~cWindowManager();

    cWindowManager(const cWindowManager&) = delete;
    cWindowManager& operator=(const cWindowManager&) = delete;

    // -------------------------------------------------------------------------
    // Dialog lifecycle. The manager takes ownership via unique_ptr; the
    // legacy engine's "Add then later delete" pattern maps to
    //   unique_ptr<cDialog> d = make_unique<cDialog>(); wm.AddDialog(move(d));
    // and RemoveDialog returns the unique_ptr so the caller can hold it.
    // -------------------------------------------------------------------------
    void AddDialog(std::unique_ptr<cDialog> dlg);
    std::unique_ptr<cDialog> RemoveDialog(cDialog* dlg);
    bool RemoveDialogById(std::int32_t id);
    void RemoveAll();

    // Process destroy queue (call once per frame). Dialogs marked
    // closeRequested() are removed and destroyed here, not at the
    // close() call site, to avoid mid-event use-after-free.
    void ProcessDestroyQueue();

    // Total dialog count (alive).
    std::size_t dialogCount() const noexcept { return m_dialogs.size(); }

    // Topmost dialog (the one currently receiving input). null if no
    // active dialogs. Determined by the dialogs' Add() order: the last
    // added is on top.
    cDialog* topmost() const noexcept;
    cDialog* topmostActive() const noexcept;   // topmost with SetActive(true)

    // -------------------------------------------------------------------------
    // Lookup. findById walks all dialogs (and their children) for a
    // matching id; returns the first hit. findByXY returns the topmost
    // active dialog whose bounding box contains (x, y).
    // -------------------------------------------------------------------------
    cDialog* findById(std::int32_t id) const;
    cDialog* findByXY(std::int32_t x, std::int32_t y) const;

    // -------------------------------------------------------------------------
    // Input dispatch. ActionEvent routes (mx, my, flags) to the topmost
    // active dialog (or, in modal mode, to the modal dialog). Returns
    // the WE_* code from that dialog, or 0 if no dialog consumed it.
    // -------------------------------------------------------------------------
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags);
    std::uint32_t ActionKeyboardEvent(std::int32_t key, std::int32_t ch);

    // -------------------------------------------------------------------------
    // M-R6.2 Focus chain (1:1 with legacy cWindowManager::SetFocus /
    // TabFocusNext / TabFocusPrev).
    //
    // The legacy engine tracks a single "currently focused" window pointer
    // across the entire dialog tree. Tab cycles to the next focusable
    // child in the topmost dialog's z-order; Shift+Tab reverses. Focus
    // change fires m_bFocus false on the old window and true on the new
    // (the OnKeyEvent handlers check m_bFocus for IME open/close).
    //
    // 1:1 quirks preserved:
    //   - Tab only walks topmost active dialog (not all dialogs).
    //   - If no candidate exists, focus stays put (no wrap).
    //   - SetFocus on the same window is a no-op.
    //   - cEditBox + cPushupButton + cIconDialog are "focusable"; others
    //     (cStatic / cPushupButton passive) are skipped.
    // -------------------------------------------------------------------------
    void SetFocus(cWindow* w);
    cWindow* focusedWindow() const noexcept           { return m_focused; }
    void TabFocusNext();
    void TabFocusPrev();

    // Test-only inspector (not for production use): the focused
    // window's id, or 0 if no focus.
    std::int32_t focusedId() const noexcept;

    // -------------------------------------------------------------------------
    // Modal mode. While a modal dialog is open, all input goes to it
    // regardless of z-order. SetModalDialog(nullptr) clears modal state.
    // -------------------------------------------------------------------------
    void SetModalDialog(cDialog* dlg) noexcept;
    cDialog* modalDialog() const noexcept      { return m_modalDialog; }
    bool isModal() const noexcept              { return m_modalDialog != nullptr; }

    // -------------------------------------------------------------------------
    // Render dispatch (placeholder — real GPU draw in 6.6+ MoxianRenderDemo
    // integration). Iterates dialogs in z-order (back to front) and asks
    // each to Render.
    // -------------------------------------------------------------------------
    void RenderAll();

    // Test accessors.
    bool destroyQueueEmpty() const noexcept     { return m_destroyQueue.empty(); }
    std::size_t destroyQueueSize() const noexcept { return m_destroyQueue.size(); }

private:
    // Focus walk helpers — used by TabFocusNext/Prev. Both return the
    // first focusable window in z-order strictly after (Next) or
    // strictly before (Prev) `start`. `fromIdx` is the index in the
    // current dialog's children where the walk resumes; -1 starts at
    // the top.
    cWindow* findFocusableAfter(cDialog* dlg, std::int32_t fromIdx) const;
    cWindow* findFocusableBefore(cDialog* dlg, std::int32_t fromIdx) const;
    // Is this window a Tab-focus target? 1:1 with legacy
    // cWindow::IsFocusable() (returns true for cEditBox /
    // cPushupButton / cIconDialog etc.; false for cStatic /
    // cPushupButton passive).
    static bool isFocusableCandidate(const cWindow* w) noexcept;

    cWindow* m_focused = nullptr;  // currently focused window (any dialog)

private:
    // Owned dialogs. Order = z-order: back is index 0, front is back().
    std::vector<std::unique_ptr<cDialog>> m_dialogs;
    // Defer-destroy queue. Dialogs are pushed here by RemoveDialog when
    // called mid-frame, and physically destroyed in ProcessDestroyQueue.
    std::vector<std::unique_ptr<cDialog>> m_destroyQueue;
    cDialog* m_modalDialog = nullptr;
};

} // namespace mxh::ui
