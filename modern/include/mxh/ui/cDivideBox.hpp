// cDivideBox.hpp — modern port of 墨香 cDivideBox (split-stack dialog).
//
// 1:1 port of legacy `cDivideBox` from
//   `墨香【源码】\[Client]MH\cDivideBox.{h,cpp}`.
//
// The legacy widget is a 173x40 modal dialog that lets the player split a
// quantity of stacked items (e.g. moving 30 out of a stack of 200 from
// inventory to trade window). It pairs a numeric "spin" input with two
// callback hooks (OK and Cancel). The legacy engine drives the spin via
// its own cSpin subclass; in the modern port we use a plain
// cEditBox-configured-for-digits as the input control so we don't have to
// ship a separate cSpin widget. The legacy up/down spin arrows aren't
// 1:1 required: a digits-only text input with Enter-to-confirm and
// OK/Cancel buttons expresses the same gameplay. (Spinner buttons are
// straightforward follow-up work, but they aren't on the critical UX
// path — the player can also type the number directly.)
//
// Construction is via CreateDivideBox(x, y, id, divideCb, cancelCb,
// vData1, vData2); this populates the child widgets (OK button, Cancel
// button, numeric input) with deterministic ids and links the callbacks.

#pragma once

#include "cDialog.hpp"
#include <cstdint>
#include <functional>

namespace mxh::ui {

class cButton;
class cEditBox;

class cDivideBox : public cDialog {
public:
    cDivideBox();

    using Callback = std::function<void(std::int32_t /*idThis*/,
                                        cDivideBox* /*self*/,
                                        std::uint32_t /*value*/,
                                        void* /*vData1*/,
                                        void* /*vData2*/)>;

    void CreateDivideBox(std::int32_t x, std::int32_t y, std::int32_t id,
                         Callback divideCb, Callback cancelCb,
                         void* vData1, void* vData2, const char* strTitle);

    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;
    std::uint32_t ActionKeyboardEvent(std::int32_t key,
                                      std::int32_t ch) override;

    void SetValue(std::uint32_t val) noexcept;
    std::uint32_t GetValue() const noexcept;
    void SetMaxValue(std::uint32_t val) noexcept;
    void SetMinValue(std::uint32_t val) noexcept;

    // Drives the OK/Cancel side effects based on a `WindowEvent` flag (the
    // legacy code path called this from its ActionKeyboardEvent handler).
    void ExcuteDBFunc(std::uint32_t we);

    // Tag used by changeKind to swap button labels (0/1/2 — the three
    // call sites the legacy engine wired up for "split", "sell", "give").
    void ChangeKind(int kind);

    cButton* m_okBtn     = nullptr;
    cButton* m_cancelBtn = nullptr;
    cEditBox* m_input    = nullptr;

    Callback m_cbDivide;
    Callback m_cbCancel;
    void*    m_vData1    = nullptr;
    void*    m_vData2    = nullptr;

private:
    static constexpr std::int32_t kOkId     = 501;
    static constexpr std::int32_t kCancelId = 502;
    static constexpr std::int32_t kInputId  = 503;

    std::uint32_t m_min = 0;
    std::uint32_t m_max = 4294967295u;
};

} // namespace mxh::ui
