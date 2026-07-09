// mxh/ui/cMsgBox.cpp
// Phase 6.9 — implementation of the modern cMsgBox.
#include "cMsgBox.hpp"

#include "cButton.hpp"

namespace mxh::ui {

namespace {

// Static state: shared button label strings. The legacy engine uses
// 3 button labels (Yes / No / Cancel / Ok). We keep the same 4 strings
// here for forward-compat with custom-button-label API in 6.13.
struct MsgBoxStatic {
    bool initialized = false;
    // Per-button labels (legacy: MB_BTN_OK, MB_BTN_YES, MB_BTN_NO,
    // MB_BTN_CANCEL — 4 entries).
    const char* labels[4] = {"OK", "Yes", "No", "Cancel"};
    std::uint32_t colors[3] = {0xFF000000, 0xFF000000, 0xFF000000};
};
MsgBoxStatic& staticState() {
    static MsgBoxStatic s;
    return s;
}

} // namespace

void cMsgBox::InitMsgBox() {
    staticState().initialized = true;
}

bool cMsgBox::IsInitialized() noexcept {
    return staticState().initialized;
}

void cMsgBox::MsgBox(std::int32_t lId, MBType nMBType,
                     const std::string& strMsg, MsgBoxCallback cb) {
    // 1. Store the dialog-level identity + the per-call message.
    cWindow::setId(lId);
    m_type    = nMBType;
    m_message = strMsg;
    m_callback = std::move(cb);
    m_closed  = false;
    // 2. Build the buttons according to the type.
    layoutButtons();
    // 3. Default button:
    //      NoBtn   -> none
    //      Ok      -> Ok
    //      YesNo   -> Yes
    //      Cancel  -> Cancel
    switch (nMBType) {
        case MBType::NoBtn:  /* no default */ break;
        case MBType::Ok:     m_defaultBtn = MBResult::Ok;     break;
        case MBType::YesNo:  m_defaultBtn = MBResult::Yes;    break;
        case MBType::Cancel: m_defaultBtn = MBResult::Cancel; break;
    }
}

void cMsgBox::layoutButtons() {
    // Each call clears any previous buttons (the legacy engine resets
    // the box on every MsgBox() call). We rebuild from scratch.
    while (childCount() > 0) {
        removeChildAt(0);
    }
    // Button geometry: row of N buttons along the bottom, evenly spaced.
    // The box itself is sized to fit by the caller (cDialog::Init was
    // already called before MsgBox).
    const std::int32_t w = static_cast<std::int32_t>(width());
    const std::int32_t h = static_cast<std::int32_t>(height());
    const std::int32_t btnW = 70;
    const std::int32_t btnH = 24;
    const std::int32_t gap  = 8;
    auto makeBtn = [&](std::int32_t btnId, const char* text) {
        auto b = std::make_unique<cButton>();
        // No per-button callback: cMsgBox::ActionEvent routes the click
        // to fireCallback() based on the button's id, so the button's
        // own onClick is a no-op (we still pass an empty std::function
        // to satisfy the cButton Init signature).
        b->Init(0, h - btnH - 8, btnW, btnH, nullptr, nullptr, nullptr,
                cButton::ClickCallback{}, nullptr, btnId);
        b->SetText(text, 0xFF000000u);
        this->Add(std::move(b));
    };
    switch (m_type) {
        case MBType::Ok: {
            makeBtn(kBtnIdOk, staticState().labels[0]);
            break;
        }
        case MBType::YesNo: {
            makeBtn(kBtnIdYes, staticState().labels[1]);
            makeBtn(kBtnIdNo,  staticState().labels[2]);
            break;
        }
        case MBType::Cancel: {
            makeBtn(kBtnIdCancel, staticState().labels[3]);
            break;
        }
        case MBType::NoBtn:
        default:
            break;
    }
    // Re-layout: center the button row along the bottom.
    std::size_t btnCount = 0;
    std::int32_t totalW = 0;
    for (std::size_t i = 0; i < childCount(); ++i) {
        if (childAt(i)) { ++btnCount; totalW += btnW; }
    }
    if (btnCount == 0) return;
    totalW += static_cast<std::int32_t>(btnCount - 1) * gap;
    std::int32_t startX = (w - totalW) / 2;
    std::size_t placed = 0;
    for (std::size_t i = 0; i < childCount(); ++i) {
        cWindow* c = childAt(i);
        if (!c) continue;
        c->SetAbsXY(absX() + startX + static_cast<std::int32_t>(placed) * (btnW + gap),
                    absY() + h - btnH - 8);
        ++placed;
    }
}

void cMsgBox::SetDefaultBtn(MBResult r) noexcept { m_defaultBtn = r; }

bool cMsgBox::ForcePressButton(MBResult r) {
    std::int32_t btnId = 0;
    switch (r) {
        case MBResult::Ok:     btnId = kBtnIdOk;     break;
        case MBResult::Yes:    btnId = kBtnIdYes;    break;
        case MBResult::No:     btnId = kBtnIdNo;     break;
        case MBResult::Cancel: btnId = kBtnIdCancel; break;
        default: return false;
    }
    cButton* b = nullptr;
    for (std::size_t i = 0; i < childCount(); ++i) {
        cWindow* c = childAt(i);
        if (c && c->id() == btnId) { b = static_cast<cButton*>(c); break; }
    }
    if (!b) return false;
    // Synthesize a click on the cMsgBox itself at the button's center.
    // Going through cMsgBox::ActionEvent ensures the dispatch path
    // (which fires the callback + auto-closes) runs uniformly with the
    // mouse-click flow. We must be active for the dispatch to fire.
    if (!isActive()) SetActive(true);
    const std::int32_t cx = b->absX() + b->width()  / 2;
    const std::int32_t cy = b->absY() + b->height() / 2;
    ActionEvent(cx, cy, cWindow::MouseFlagLButton);
    ActionEvent(cx, cy, 0);
    return true;
}

void cMsgBox::ForceClose() noexcept {
    m_closed = true;
    requestClose();
}

void cMsgBox::fireCallback(MBResult r) {
    if (m_closed) return;
    m_closed = true;
    if (m_callback) m_callback(*this, r, m_userdata);
    // Auto-close: the dispatcher will pick up closeRequested() and
    // remove us on the next ProcessDestroyQueue().
    requestClose();
}

std::uint32_t cMsgBox::ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                    std::uint32_t mouseFlags) {
    // Modal by default — the dispatcher marks us modal when added.
    if (!isEnabled() || !isActive()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    // Top-down dispatch. If a child button consumed the event, fire the
    // callback + close.
    if (mouseFlags & cWindow::MouseFlagLButton) {
        // Probe children back-to-front: the click might land on a button.
        for (std::size_t i = childCount(); i > 0; --i) {
            cWindow* c = childAt(i - 1);
            if (!c) continue;
            if (!c->PtInWindow(mouseX, mouseY)) continue;
            cButton* b = dynamic_cast<cButton*>(c);
            if (!b) break;          // hit a non-button child, don't proceed
            // Determine the result from the button's id.
            MBResult r = MBResult::Count;
            switch (c->id()) {
                case kBtnIdOk:     r = MBResult::Ok;     break;
                case kBtnIdYes:    r = MBResult::Yes;    break;
                case kBtnIdNo:     r = MBResult::No;     break;
                case kBtnIdCancel: r = MBResult::Cancel; break;
                default: break;
            }
            if (r != MBResult::Count) {
                // Drive the button's state machine so its click is
                // dispatched + the callback fires consistently.
                b->ActionEvent(mouseX, mouseY, mouseFlags);
                b->ActionEvent(mouseX, mouseY, 0);
                fireCallback(r);
                return static_cast<std::uint32_t>(WindowEvent::LButtonClick);
            }
            break;
        }
    }
    // No button hit (or no LButton): defer to cDialog for the rest.
    return cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
}

std::uint32_t cMsgBox::ActionKeyboardEvent(std::int32_t key, std::int32_t ch) {
    if (!isEnabled() || !isActive()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    // Enter / Esc -> default button (legacy cMsgBox contract).
    constexpr std::int32_t kEnter = 13;
    constexpr std::int32_t kEscape = 27;
    if (key == kEnter) {
        if (m_type != MBType::NoBtn) {
            ForcePressButton(m_defaultBtn);
            fireCallback(m_defaultBtn);
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        }
    } else if (key == kEscape) {
        if (m_type == MBType::YesNo) {
            ForcePressButton(MBResult::No);
            fireCallback(MBResult::No);
        } else if (m_type == MBType::Ok) {
            ForcePressButton(MBResult::Ok);
            fireCallback(MBResult::Ok);
        } else if (m_type == MBType::Cancel) {
            ForcePressButton(MBResult::Cancel);
            fireCallback(MBResult::Cancel);
        }
        return static_cast<std::uint32_t>(WindowEvent::KeyDown);
    }
    return cDialog::ActionKeyboardEvent(key, ch);
}

} // namespace mxh::ui
