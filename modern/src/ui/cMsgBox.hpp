// mxh/ui/cMsgBox.hpp
// Phase 6.9 — modern C++ cMsgBox. Modal message box (the most common
// pop-up in the legacy engine: yes/no prompts, OK-only info, cancel
// confirmations). Builds on cDialog (6.3) + cButton (6.1) +
// cEditBox-style text area (we use std::string + a render hook for now;
// full multi-line text widget lands in 6.13).
//
// Scope (this phase): the dispatch + callback contract.
//   - MBT_NOBTN / MBT_OK / MBT_YESNO / MBT_CANCEL button configurations
//   - Modern callback signature: void(cMsgBox&, MBResult, void*)
//   - Default button (Enter / Esc keys map to it)
//   - Auto-close after button click
//   - Force-press a button (programmatic, e.g. timer-triggered close)
//   - ForceClose (clean teardown path)
//
// Deferred:
//   - Full multi-line text rendering (depends on cMultiLineText 6.13)
//   - Custom button per-row icons (the legacy engine supports per-row
//     button images; we use the dialog's basic button style for now)
#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "cDialog.hpp"

namespace mxh::ui {

class cMsgBox : public cDialog {
public:
    // Message-box type. Determines which buttons appear.
    enum class MBType : std::int32_t {
        NoBtn   = 0,
        Ok      = 1,
        YesNo   = 2,
        Cancel  = 3,
    };

    // Result code returned to the callback. Mirrors the legacy
    // eMB_BTN_ID ordering.
    enum class MBResult : std::int32_t {
        Ok     = 0,
        Yes    = 1,
        No     = 2,
        Cancel = 3,
        Count  = 4,
    };

    // Modern callback signature: receives the box, the result, and the
    // userdata pointer (set via SetUserdata).
    using MsgBoxCallback = std::function<void(cMsgBox& self, MBResult result,
                                              void* userdata)>;

    cMsgBox() = default;
    ~cMsgBox() override = default;

    cMsgBox(const cMsgBox&) = delete;
    cMsgBox& operator=(const cMsgBox&) = delete;

    // -------------------------------------------------------------------------
    // Static one-time init (loads shared button images + label strings).
    // The legacy engine uses static state; we mirror that to keep the
    // call sites compatible. Modern code can call this once at app
    // startup; it's a no-op if already initialized.
    // -------------------------------------------------------------------------
    static void InitMsgBox();
    static bool IsInitialized() noexcept;

    // -------------------------------------------------------------------------
    // Configure the message box. After this call the box is ready to be
    // added to a cWindowManager and dispatched. The (id, nMBType, strMsg,
    // cb) signature mirrors the legacy MsgBox(...) helper; we keep the
    // id as a void* so callers can stash a pointer (e.g. a window) for
    // callback dispatch.
    // -------------------------------------------------------------------------
    void MsgBox(std::int32_t lId, MBType nMBType, const std::string& strMsg,
                MsgBoxCallback cb = nullptr);

    // Action overrides — modal behaviour: any click on a button closes
    // the box and fires the callback.
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;
    std::uint32_t ActionKeyboardEvent(std::int32_t key, std::int32_t ch) override;

    // Default button. The Enter key activates it; the Esc key activates
    // the Cancel button (if any) or the Ok button (YesNo).
    void SetDefaultBtn(MBResult r) noexcept;
    MBResult defaultBtn() const noexcept { return m_defaultBtn; }

    // Force-press a specific button (e.g. from a timer that auto-closes
    // the box). Returns true if the button existed and was pressed.
    bool ForcePressButton(MBResult r);

    // Force-close without firing the callback. Used by the dispatcher
    // when the box is being torn down as part of a parent dialog
    // destruction.
    void ForceClose() noexcept;

    // Test accessors.
    MBType type() const noexcept              { return m_type; }
    const std::string& message() const noexcept { return m_message; }
    std::int32_t id() const noexcept           { return cWindow::id(); }
    bool isClosed() const noexcept            { return m_closed; }
    void SetCallback(MsgBoxCallback cb)       { m_callback = std::move(cb); }
    void SetUserdata(void* u)                 { m_userdata = u; }

private:
    void layoutButtons();
    void fireCallback(MBResult r);

    MBType           m_type      = MBType::NoBtn;
    MBResult         m_defaultBtn = MBResult::Ok;
    std::string      m_message;
    MsgBoxCallback   m_callback;
    void*            m_userdata  = nullptr;
    bool             m_closed    = false;

    // Cached button child IDs (1-based for SetAdd via cDialog::Add).
    static constexpr std::int32_t kBtnIdOk     = 1001;
    static constexpr std::int32_t kBtnIdYes    = 1002;
    static constexpr std::int32_t kBtnIdNo     = 1003;
    static constexpr std::int32_t kBtnIdCancel = 1004;
};

} // namespace mxh::ui
