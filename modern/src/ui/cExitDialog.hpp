// cExitDialog.hpp — modern port of 墨香 CExitDialog (exit confirmation).
//
// 1:1 port of legacy `CExitDialog` from
//   `墨香【源码】\[Client]MH\ExitDialog.{h,cpp}`.
//
// The legacy CExitDialog only overrides SetActive() to notify the main
// bar dialog to highlight / un-highlight the exit button (OPT_EXITDLGICON).
// The modern port keeps that contract but expresses the notification as
// a callback (`onActiveChanged`) instead of a direct dependency on a
// concrete MainBarDialog instance — the host caller (the application /
// dispatcher that owns the main bar) decides what to do when the exit
// dialog opens / closes. This keeps cExitDialog self-contained and unit
// testable without dragging in the entire main-bar subtree.
//
// Usage pattern (host caller):
//   cExitDialog exitDlg;
//   exitDlg.Init(280, 200, 240, 120, basicImage, 200 /*id*/);
//   exitDlg.SetOnActiveChanged([this](bool active) {
//       m_mainBar.SetExitIconHighlighted(active);  // host-side glue
//   });
//   // dispatcher:
//   exitDlg.SetActive(true);   // → onActiveChanged(true)  → icon highlights
//   exitDlg.SetActive(false);  // → onActiveChanged(false) → icon released

#pragma once

#include <cstdint>
#include <functional>

#include "cDialog.hpp"

namespace mxh::ui {

class cExitDialog : public cDialog {
public:
    // Notification fired whenever the dialog's active state changes.
    // Replaces the legacy CExitDialog::SetActive() side-effect that
    // poked CMainBarDialog::SetPushBarIcon() directly. Argument is the
    // new active state (true = opened, false = closed).
    using ActiveChangedCallback = std::function<void(bool active)>;

    cExitDialog() = default;
    ~cExitDialog() override = default;

    cExitDialog(const cExitDialog&) = delete;
    cExitDialog& operator=(const cExitDialog&) = delete;

    // -------------------------------------------------------------------------
    // Init: position, size, basic image, id. The exit dialog is a small
    // modal — typical legacy size is around 240x120 with a "are you sure
    // you want to exit?" caption and two buttons (OK / Cancel). The
    // modern port owns the size; the host caller decides the layout.
    // -------------------------------------------------------------------------
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid, std::uint16_t hei,
              void* basicImage, std::int32_t id = 0);

    // -------------------------------------------------------------------------
    // SetActive: records the new state and fires the onActiveChanged
    // callback (if set) exactly when the value transitions. Matches the
    // legacy contract: SetActive(TRUE) → main-bar exit icon highlights;
    // SetActive(FALSE) → icon released. The callback is NOT fired when
    // the new value equals the previous value — that prevents
    // double-notifying the main bar if the dispatcher redundantly sets
    // the same state.
    //
    // Overrides cDialog::SetActive (virtual since Phase 12.1 R-12 fix),
    // so cDialog* / cWindow* polymorphic dispatch hits this method and
    // the callback fires correctly. Before the R-12 fix, the base
    // cDialog::SetActive was non-virtual and this was a name-hiding
    // overload — polymorphic calls through cDialog* would skip the
    // callback. See KNOWN_BUGS.md R-12 for the historical bug.
    // -------------------------------------------------------------------------
    void SetActive(bool val) noexcept override;

    // -------------------------------------------------------------------------
    // Callback registration. Caller owns the callback lifetime; the
    // dialog holds a std::function so the caller can rebind at any time
    // (e.g. when the main bar is rebuilt). Clearing the callback
    // (passing {}) silently drops notifications — the active state is
    // still tracked internally so isActive() keeps returning the truth.
    // -------------------------------------------------------------------------
    void SetOnActiveChanged(ActiveChangedCallback cb) noexcept { m_onActiveChanged = std::move(cb); }
    const ActiveChangedCallback& onActiveChanged() const noexcept { return m_onActiveChanged; }

    // -------------------------------------------------------------------------
    // Test accessor — exposes the last active value the dialog was set
    // to, independent of the parent cDialog's m_bActive. Useful for
    // tests that want to verify SetActive() updated the internal state
    // without exercising the full cWindow visibility chain.
    // -------------------------------------------------------------------------
    bool exitActive() const noexcept { return m_bExitActive; }

private:
    bool                  m_bExitActive      = false;
    ActiveChangedCallback m_onActiveChanged;
};

} // namespace mxh::ui
