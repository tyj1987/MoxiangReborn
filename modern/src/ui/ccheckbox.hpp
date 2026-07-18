// ccheckbox.hpp — modern port of 墨香 cCheckBox (check box widget).
//
// 1:1 port of legacy `cCheckBox` from
//   `墨香【源码】\[Client]MH\interface\cCheckBox.{h,cpp}`.
//
// cCheckBox is a small cWindow subclass that displays a
// check box + label. The user toggles the check state by
// clicking. The dialog fires a callback (cbFUNC) with
// WE_CHECKED / WE_NOTCHECKED when the state changes.
//
// 1:1 contract preserved:
//   - Init(x, y, wid, hei, basicImage, checkBoxImage, checkImage,
//          Func, ID) — initializes the cWindow base with the
//          basicImage, stores the checkBoxImage + checkImage
//          for later Render, and wires the optional cbFUNC
//          callback. Modern port: cWindow::Init has 6 params
//          (no basicImage-style image signature; cImage is
//          forward-declared and stored as `void*`).
//   - ActionEvent(mouseInfo) — checks m_bActive + m_bDisable,
//          runs cWindow::ActionEvent, and on WE_LBTNCLICK
//          (within the window) toggles m_fChecked (XOR) and
//          fires cbWindowFunc with WE_CHECKED / WE_NOTCHECKED.
//          Modern port: CMouse stubbed no-op (Phase 6.x deferred).
//   - Render() — draws the checkBoxImage + checkImage (if
//          checked) + the label text via CFONT_OBJ. Modern
//          port: Render no-op stub (Phase 6.x render deferred).
//   - IsChecked / SetChecked — get/set m_fChecked.
//   - SetCheckBoxMsg(msg, color) — copies the text + stores
//          the color. Modern port: stores the text in a
//          std::string + color as std::uint32_t.
//
// 1:1 quirks preserved:
//   - 1:1 quirk: legacy ctor sets
//     `memset(m_szCheckBoxText, 0, MAX_CHECKBOXTEXT_SIZE);`
//     + `m_dwCheckBoxTextColor=RGB_HALF(255,255,255);`
//     + `m_fChecked = FALSE;` + `m_type = WT_CHECKBOX;`.
//     Modern port: ctor default-inits m_fChecked to false
//     + m_checkBoxTextColor to default. The m_type field
//     is dropped (Phase 6 removed, same as
//     cTipBrowserDlg/cPetStateMiniDlg/cSkillPointNotify).
//   - 1:1 quirk: legacy `m_fChecked ^= TRUE` on click —
//     modern port preserves the XOR semantics.
//   - 1:1 quirk: legacy `WE_LBTNCLICK` is the click event
//     (legacy == 64). Modern cWindow::WindowEvent::LButtonClick
//     is the modern equivalent (modern == 4) per R-12.
//     Modern port: matches against `we == LButtonClick`.
//   - 1:1 quirk: legacy `cbFUNC Func` is a callback typedef
//     (`void(*)(LONG, void*, DWORD)`). Modern port uses
//     `std::function<void(int32_t, void*, uint32_t)>` for
//     test injection.
//   - 1:1 quirk: legacy `cbWindowFunc = Func` is stored
//     in the cWindow base. Modern cWindow has no
//     `cbWindowFunc` field. Modern port: stores the callback
//     in cCheckBox itself + invokes it directly.
//   - 1:1 quirk: legacy `m_pParent` is read to pass as the
//     second arg to cbWindowFunc. Modern cWindow has no
//     `m_pParent` field. Modern port: passes `this` as the
//     second arg (1:1 fidelity for the call signature, modern
//     port uses the dialog itself).
//   - 1:1 quirk: legacy `WE_CHECKED` / `WE_NOTCHECKED`
//     constants are part of the legacy WE_* enum.
//     `cWindow::WindowEvent` doesn't define Checked/NotChecked.
//     Modern port: documents as 1:1 quirk + provides local
//     constexpr kWeChecked=128 / kWeNotChecked=256 (matching
//     the legacy WE_CHECKED=128 / WE_NOTCHECKED=256).
//   - 1:1 quirk: legacy `cImage m_CheckBoxImage` and
//     `cImage m_CheckImage` are value-typed cImage fields.
//     Modern cImage is a GPU-backed resource. Modern port:
//     stores them as `void*` (opaque image handle). The
//     Init's `*checkBoxImage` / `*checkImage` dereferences
//     are not done in modern port (we store the pointer
//     directly; production code wires real cImage).
//   - 1:1 quirk: legacy `m_szCheckBoxText` is a fixed-size
//     char array (`MAX_CHECKBOXTEXT_SIZE`). Modern port:
//     uses `std::string` (dynamic size, same end-state).
//   - 1:1 quirk: legacy `m_dwCheckBoxTextColor` is a DWORD
//     (RGB color). Modern port: `std::uint32_t` ARGB.
//   - 1:1 quirk: legacy `VECTOR2 start_pos` + `RGBA_MERGE`
//     + `cFont` calls in Render. Modern cFont + cImage
//     don't have these APIs. Modern port: Render is a
//     no-op stub (Phase 6.x render wiring deferred).
//   - 1:1 quirk: legacy `m_pParent` is read in
//     ActionEvent but the cCheckBox is a cWindow (not
//     cDialog), so legacy cWindow has no m_pParent. The
//     legacy cCheckBox body actually expects the parent
//     cDialog to be passed externally. Modern port:
//     stores the parent as `cWindow* m_parentDialog` and
//     passes it to the callback (1:1 fidelity for the
//     arg; modern port uses an explicit parent ptr).
//   - 1:1 quirk: legacy ctor sets `m_type = WT_CHECKBOX`
//     (preserved as 1:1 quirk note; modern port drops
//     m_type per Phase 6).
//   - 1:1 quirk: legacy Init's `m_type = WT_CHECKBOX;`
//     (a 2nd assignment in Init body). Modern port: also
//     dropped (same as ctor).

#pragma once

#include "cWindow.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace mxh::ui {

class cImage;
class CMouse;

// 1:1 with legacy `WE_CHECKED` / `WE_NOTCHECKED` (per legacy
// cWindowDef.h enum WINDOW_EVENT). Modern cWindow::WindowEvent
// doesn't define these values; modern port uses local
// constants matching the legacy bit-field values.
constexpr std::uint32_t kWeChecked    = 128u;
constexpr std::uint32_t kWeNotChecked = 256u;

// 1:1 with legacy `cbFUNC` typedef:
//   `typedef void (*cbFUNC)(LONG, void*, DWORD);`
// Modern port uses std::function for test-injectability.
using CheckboxCallback = std::function<void(std::int32_t, void*, std::uint32_t)>;

class cCheckBox : public cWindow {
public:
    cCheckBox();
    ~cCheckBox() override;

    cCheckBox(const cCheckBox&) = delete;
    cCheckBox& operator=(const cCheckBox&) = delete;

    // 1:1 with legacy surface.
    void Init(std::int32_t x, std::int32_t y, std::int16_t wid,
              std::int32_t hei, void* basicImage, void* checkBoxImage,
              void* checkImage, CheckboxCallback Func,
              std::int32_t ID = 0);
    std::uint32_t ActionEvent(CMouse* mouseInfo);
    void Render();

    bool IsChecked() const noexcept { return m_fChecked; }
    void SetChecked(bool val) noexcept { m_fChecked = val; }

    // 1:1 with legacy SetCheckBoxMsg(msg, color).
    void SetCheckBoxMsg(const char* msg, std::uint32_t color);

    // Test accessors.
    const std::string& checkBoxText() const noexcept { return m_szCheckBoxText; }
    std::uint32_t checkBoxTextColor() const noexcept { return m_dwCheckBoxTextColor; }
    std::uint32_t callbackFiredCount() const noexcept { return s_callbackFiredCount; }
    std::uint32_t lastCallbackWe()    const noexcept { return s_lastCallbackWe; }
    std::int32_t  lastCallbackId()    const noexcept { return s_lastCallbackId; }
    void*         lastCallbackParent() const noexcept { return s_lastCallbackParent; }

    // Test-injectable: replace the click-handler callback.
    // (Production code wires real callbacks via Init.)
    static void ClearTestInjections() noexcept;

    // Test-injectable: simulate a click on the check box.
    // Modern port: CMouse is a stub (Phase 6.x deferred),
    // so ActionEvent can't detect clicks. The test
    // pattern uses `ToggleForTesting()` to verify the
    // 1:1 click → toggle + callback dispatch.
    // 1:1 with legacy:
    //   m_fChecked ^= TRUE;
    //   (*cbWindowFunc)(m_ID, m_pParent, ...);
    void ToggleForTesting();

    // Test-injectable: legacy `m_pParent` is a cWindow*
    // passed to the callback. Modern cWindow has no
    // `m_pParent`; the test pattern wires a parent
    // explicitly via this setter.
    void SetParentDialogForTesting(cWindow* parent) noexcept {
        m_parentDialog = parent;
    }

private:
    // 1:1 quirk: legacy stores cImage by value
    // (`cImage m_CheckBoxImage; cImage m_CheckImage;`).
    // Modern cImage is a GPU-backed resource; we store
    // opaque handles (void*) instead.
    void* m_CheckBoxImageHandle = nullptr;
    void* m_CheckImageHandle     = nullptr;
    cWindow* m_parentDialog      = nullptr;  // 1:1 quirk: legacy
                                              // reads m_pParent; modern
                                              // cWindow has no such field.

    // 1:1 with legacy m_szCheckBoxText (fixed-size char array).
    // Modern port uses std::string for dynamic sizing.
    std::string m_szCheckBoxText;
    std::uint32_t m_dwCheckBoxTextColor = 0xFFFFFFFFu;  // 1:1 with legacy
                                                        // RGB_HALF(255,255,255) = white.
    bool m_fChecked = false;  // 1:1 with legacy m_fChecked.

    // 1:1 with legacy cbWindowFunc. Modern cWindow doesn't
    // have cbWindowFunc; we store it locally.
    CheckboxCallback m_func;

    // Test-injectable state.
    static inline std::uint32_t s_callbackFiredCount = 0;
    static inline std::uint32_t s_lastCallbackWe     = 0;
    static inline std::int32_t  s_lastCallbackId     = 0;
    static inline void*         s_lastCallbackParent = nullptr;
};

} // namespace mxh::ui
