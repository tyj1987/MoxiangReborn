// mxh/ui/cDialog.hpp
// Phase 6.3 — modern C++ cDialog widget. A container for child widgets
// (cButton / cEditBox / etc.). The most common pattern in the legacy
// engine: a dialog window with a caption (draggable title bar) and a
// set of child controls. Builds on cWindow (6.0) and the Add() tree
// machinery established there.
//
// Scope (this phase):
//   - cWindow tree management (delegates to base Add() / childAt())
//   - findWindowById(id) - O(n) lookup, matches the legacy cDialog contract
//   - active / setActiveRecursive - the legacy engine's "this dialog is the
//     topmost modal right now" flag, used by the dispatcher to know which
//     dialog should receive input
//   - auto-close mode - the legacy engine's escape-hatch for "dialog closes
//     itself when its own event fires"; we record the flag and let the
//     consumer query it
//   - caption rect - the draggable title bar; PtInCaption is a hit-test
//     helper
//   - alpha (placeholder, real GPU in 6.4+)
//
// Deferred (later phases):
//   - dragging the caption (full mouse-drag move with viewport clamping)
//   - close button (m_pCloseBtn in legacy; would need a real cButton child
//     to be useful)
//   - modal-blocking semantics (the cDialog::SetActive dispatcher integration
//     that suspends input to lower dialogs; this lives in cWindowManager
//     in the legacy engine, not cDialog itself)
#pragma once

#include <cstdint>

#include "cWindow.hpp"

namespace mxh::ui {

// Simple caption rect (legacy uses Win32 RECT; we keep a minimal struct so
// the framework doesn't pull in <windows.h>).
struct CaptionRect {
    std::int32_t left   = 0;
    std::int32_t top    = 0;
    std::int32_t right  = 0;
    std::int32_t bottom = 0;
};

class cDialog : public cWindow {
public:
    cDialog() = default;
    ~cDialog() override = default;

    cDialog(const cDialog&) = delete;
    cDialog& operator=(const cDialog&) = delete;

    // -------------------------------------------------------------------------
    // Init: position, size, basic image (the dialog's chrome / background),
    // id. Children are added via cWindow::Add (the base class).
    // -------------------------------------------------------------------------
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid, std::uint16_t hei,
              void* basicImage, std::int32_t id = 0);

    // Render placeholder (real GPU draw in 6.4+).
    void Render() override {}

    // ActionEvent: top-down dispatch already in cWindow recurses into
    // children. We keep the override for type identification; the actual
    // behavior is unchanged.
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // -------------------------------------------------------------------------
    // Auto-close (legacy: SetAutoClose / IsAutoClose). The flag itself
    // doesn't close the dialog; the dispatcher (or a manual call to
    // requestClose) reads it and decides. We also expose requestClose()
    // for the consumer to mark the dialog as "ready to be torn down
    // next frame" — a latched bool that the dispatcher consumes.
    // -------------------------------------------------------------------------
    void SetAutoClose(bool v) noexcept        { m_bAutoClose = v; }
    bool IsAutoClose() const noexcept         { return m_bAutoClose; }
    void requestClose() noexcept              { m_bCloseRequested = true; }
    bool closeRequested() const noexcept      { return m_bCloseRequested; }
    void clearCloseRequest() noexcept         { m_bCloseRequested = false; }

    // -------------------------------------------------------------------------
    // Caption rect (draggable title bar). Legacy: SetCaptionRect /
    // GetCaptionRect. PtInCaption is a hit-test helper for the dispatcher
    // to know whether the user grabbed the title bar (so it can start a
    // drag-move operation).
    // -------------------------------------------------------------------------
    void SetCaptionRect(std::int32_t left, std::int32_t top,
                        std::int32_t right, std::int32_t bottom) noexcept;
    void SetCaptionRect(const struct CaptionRect& r) noexcept;
    bool PtInCaption(std::int32_t x, std::int32_t y) const noexcept;
    std::int32_t captionLeft()   const noexcept { return m_captionLeft; }
    std::int32_t captionTop()    const noexcept { return m_captionTop; }
    std::int32_t captionRight()  const noexcept { return m_captionRight; }
    std::int32_t captionBottom() const noexcept { return m_captionBottom; }
    bool hasCaption() const noexcept           { return m_hasCaption; }

    // -------------------------------------------------------------------------
    // Active state (legacy: SetActive / SetActiveRecursive). In the legacy
    // engine, "active" means "this dialog is the current topmost" — only
    // the active dialog receives input. The recursive variant cascades the
    // flag to all children (used when re-activating a minimized dialog).
    // -------------------------------------------------------------------------
    void SetActive(bool v) noexcept           { m_bActive = v; }
    void SetActiveRecursive(bool v);                              // cascades
    bool isActive() const noexcept           { return m_bActive; }

    // -------------------------------------------------------------------------
    // Alpha (legacy: SetAlpha(BYTE) / SetOptionAlpha(DWORD)). The legacy
    // engine supports a per-dialog alpha for translucent UI (used in
    // tooltips and the option menu). The render layer (6.4+) will read
    // these and apply the per-window alpha multiplier; the framework just
    // stores the values.
    // -------------------------------------------------------------------------
    void SetAlpha(std::uint8_t al) noexcept   { m_alpha = al; }
    std::uint8_t alpha() const noexcept      { return m_alpha; }
    void SetOptionAlpha(std::uint32_t a) noexcept { m_optionAlpha = a; }
    std::uint32_t optionAlpha() const noexcept   { return m_optionAlpha; }

    // -------------------------------------------------------------------------
    // Lookup helpers.
    // -------------------------------------------------------------------------
    cWindow* findWindowById(std::int32_t id) const;
    // componentCount / componentAt mirror cWindow's child machinery but
    // are spelled with the legacy names to keep the engine-facing API close.
    std::size_t componentCount() const noexcept { return childCount(); }
    cWindow*    componentAt(std::size_t i) const noexcept { return childAt(i); }

    // -------------------------------------------------------------------------
    // SetAbsXY override: in addition to moving the dialog, offset every
    // child's absX/absY by the delta so the absolute layout stays correct.
    // Legacy: cDialog::SetAbsXY.
    // -------------------------------------------------------------------------
    void SetAbsXY(std::int32_t x, std::int32_t y) noexcept override;

    // SetDisable cascades to children (legacy contract).
    void SetDisable(bool v) noexcept override;

private:
    bool          m_bAutoClose      = false;
    bool          m_bCloseRequested = false;
    bool          m_bActive         = false;
    bool          m_hasCaption      = false;
    std::int32_t  m_captionLeft     = 0;
    std::int32_t  m_captionTop      = 0;
    std::int32_t  m_captionRight    = 0;
    std::int32_t  m_captionBottom   = 0;
    std::uint8_t  m_alpha           = 255;
    std::uint32_t m_optionAlpha     = 0xFF000000;
};

} // namespace mxh::ui
