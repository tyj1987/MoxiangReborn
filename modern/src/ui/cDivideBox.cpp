// cDivideBox.cpp — modern implementation of 墨香 cDivideBox.

#include "cDivideBox.hpp"

#include "cButton.hpp"
#include "cEditBox.hpp"

namespace mxh::ui {

cDivideBox::cDivideBox()
    : cDialog() {}

void cDivideBox::CreateDivideBox(std::int32_t x, std::int32_t y,
                                  std::int32_t id,
                                  Callback divideCb, Callback cancelCb,
                                  void* vData1, void* vData2,
                                  const char* /*strTitle*/) {
    m_cbDivide = std::move(divideCb);
    m_cbCancel = std::move(cancelCb);
    m_vData1   = vData1;
    m_vData2   = vData2;

    // Layout: 173x40 mirror of the legacy sized dialog.
    // Children:
    //   - OK button:    x=128, y=15, w=40, h=20
    //   - Cancel button: x=85 (≈170-40-5), y=15, w=40, h=20
    //   - Numeric input: x=30, y=15, w=93, h=20  (digits-only)
    //
    // We Rebuild from scratch every call (legacy pattern).
    Init(x, y, 173, 40, nullptr, id);

    // OK button.
    {
        auto b = std::make_unique<cButton>();
        b->Init(128, 15, 40, 20, nullptr, nullptr, nullptr,
                cButton::ClickCallback{}, nullptr, kOkId);
        b->SetText("OK", 0xFFE1E1E1u);
        m_okBtn = b.get();
        Add(std::move(b));
    }
    // Cancel button.
    {
        auto b = std::make_unique<cButton>();
        b->Init(85, 15, 40, 20, nullptr, nullptr, nullptr,
                cButton::ClickCallback{}, nullptr, kCancelId);
        b->SetText("Cancel", 0xFFE1E1E1u);
        m_cancelBtn = b.get();
        Add(std::move(b));
    }
    // Numeric input.
    {
        auto e = std::make_unique<cEditBox>();
        e->Init(30, 15, 93, 20, nullptr, nullptr, kInputId);
        e->InitEditbox(93, 16);  // up to 15 ASCII digits + NUL
        m_input = e.get();
        Add(std::move(e));
    }
}

std::uint32_t cDivideBox::ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                      std::uint32_t mouseFlags) {
    if (!isEnabled() || !isActive()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    // Walk children top-down via base class dispatch (cDialog's path).
    // OK / Cancel detection happens after the child button processes the
    // event: cDialog's child walk will call cButton::ActionEvent which
    // emits LButtonClick(4). We probe our two known ids.
    const std::uint32_t we = cDialog::ActionEvent(mouseX, mouseY, mouseFlags);

    // The cDialog dispatch already routed LButtonClick to cButton — but
    // cButton fires its own callback on click. Since we passed empty
    // callbacks for the buttons, we identify the click by id here.
    // (For simplicity we re-check: if the click is inside either button's
    // rect and LButton was set, fire the corresponding side effect.)
    bool clicked = false;
    if (mouseFlags & cWindow::MouseFlagLButton) {
        if (m_okBtn && m_okBtn->PtInWindow(mouseX, mouseY)) {
            if (m_cbDivide) {
                m_cbDivide(id(), this, GetValue(), m_vData1, m_vData2);
            }
            clicked = true;
        } else if (m_cancelBtn && m_cancelBtn->PtInWindow(mouseX, mouseY)) {
            if (m_cbCancel) {
                m_cbCancel(id(), this, GetValue(), m_vData1, m_vData2);
            }
            clicked = true;
        }
    }
    if (clicked) {
        requestClose();
        SetDisable(true);
    }
    return we;
}

std::uint32_t cDivideBox::ActionKeyboardEvent(std::int32_t key,
                                              std::int32_t /*ch*/) {
    if (!isEnabled() || !isActive()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    // Enter commits (matches the legacy keyboard handler).
    if (key == 13 /*VK_RETURN*/) {
        ExcuteDBFunc(static_cast<std::uint32_t>(WindowEvent::KeyDown));
        return static_cast<std::uint32_t>(WindowEvent::KeyDown);
    }
    return static_cast<std::uint32_t>(WindowEvent::Null);
}

void cDivideBox::SetValue(std::uint32_t val) noexcept {
    if (!m_input) return;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u", val);
    m_input->SetEditText(buf);
}

std::uint32_t cDivideBox::GetValue() const noexcept {
    if (!m_input) return 0;
    // cEditBox exposes content as std::string via editText().
    const std::string& s = m_input->editText();
    if (s.empty()) return 0;
    // Best-effort unsigned parse; clamp to m_min/m_max on output.
    std::uint32_t v = 0;
    for (char c : s) {
        if (c < '0' || c > '9') continue;
        const std::uint32_t d = static_cast<std::uint32_t>(c - '0');
        v = v * 10u + d;
    }
    if (v < m_min) v = m_min;
    if (v > m_max) v = m_max;
    return v;
}

void cDivideBox::SetMaxValue(std::uint32_t val) noexcept { m_max = val; }
void cDivideBox::SetMinValue(std::uint32_t val) noexcept { m_min = val; }

void cDivideBox::ExcuteDBFunc(std::uint32_t we) {
    // Legacy: we==WE_RETURN runs divide; we==0 runs cancel.
    if (we == static_cast<std::uint32_t>(WindowEvent::KeyDown)) {
        if (m_cbDivide) {
            m_cbDivide(id(), this, GetValue(), m_vData1, m_vData2);
        }
        requestClose();
        SetDisable(true);
        return;
    }
    if (we == 0) {
        if (m_cbCancel) {
            m_cbCancel(id(), this, GetValue(), m_vData1, m_vData2);
        }
        requestClose();
        SetDisable(true);
    }
}

void cDivideBox::ChangeKind(int /*kind*/) {
    // Legacy: rewrites button labels based on chat-manager strings.
    // In the modern port labels are kept simple ("OK" / "Cancel").
    // Test sites pass 0/1/2 — keeping the signature avoids breaking
    // caller code while deferring string-table wiring to a follow-up
    // (which needs the chat-text subsystem to be ported).
}

} // namespace mxh::ui
