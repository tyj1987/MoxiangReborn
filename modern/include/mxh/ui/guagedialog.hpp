// guagedialog.hpp — modern port of 墨香 CGuageDialog (mussang guage).
//
// 1:1 port of legacy `CGuageDialog` from
//   `墨香【源码】\[Client]MH\GuageDialog.{h,cpp}`.
//
// CGuageDialog is the "mussang" (fury/berserk mode) gauge UI. It owns:
//   - 1 cButton (the mussang start button) — gets disabled on Link
//   - 1 cStatic (flicker overlay #1) — toggled on/off by SetFlicker
//     and the per-frame FlickerMussangGuage timer
//   - 1 cObjectGuagen (the mussang gauge bar) — initialized to 0
//
// The legacy class also has a `m_pFlicker02` field which is declared
// in the .h but never wired in the .cpp (the corresponding Linking
// and SetFlicker calls are commented out). The modern port preserves
// the 1:1 quirk: the field exists in the class layout for memory
// compatibility but is not materialised.
//
// 1:1 contract preserved:
//   - Linking() resolves the 3 children by id and initialises the
//     gauge to 0. Calls DisableMussangBtn(TRUE) (1:1 quirk: legacy
//     unconditionally disables the button on Link — the host must
//     re-enable when ready).
//   - OnActionEvent dispatches the mussang start click to the
//     MUSSANGMGR global singleton (stubbed no-op for the modern
//     port; host app wires a real MUSSANGMGR).
//   - DisableMussangBtn toggles SetDisable + an image RGB color
//     hint (legacy `SetImageRGB`). Modern port stores the color in
//     `m_imageRGB` for test inspection; the visual layer (Phase 6.3
//     cImage hook) is not ported so the color is a no-op on screen.
//   - SetFlicker starts the flicker timer; FlickerMussangGuage is
//     the per-frame swap (called from Render). Modern port uses a
//     test-injectable clock (m_nowMillis) so the flicker test is
//     deterministic.
//   - Render is a no-op stub in the modern port (Phase 6.3 GPU
//     path is not active).
//
// 1:1 quirks preserved:
//   - 1:1 quirk: `m_pFlicker02` exists in the header but Linking
//     never materialises it. Modern port omits the member entirely
//     (no 1:1 fidelity is gained by adding a never-used field).
//   - 1:1 quirk: `m_bFlicker` + `m_bFlActive` + `m_dwFlickerSwapTime`
//     state machines are preserved verbatim.
//   - 1:1 quirk: legacy `SetImageRGB` is preserved as a
//     `m_imageRGB` field + Setter; visual layer is no-op (R-10
//     cImage GPU not ported).
//   - 1:1 quirk: legacy `gCurTime` is replaced by a test-injectable
//     `m_nowMillis` clock (0:1 oddity — the legacy code path is
//     "time-since-link" computed by the engine, but modern port
//     uses a deterministic millisecond counter for testability).
//   - 1:1 quirk: legacy `((CObjectGuagen*)...)` cast preserved —
//     modern port uses findWindowById + dynamic_cast, which yields
//     the same cObjectGuagen*.
//   - 1:1 quirk: legacy `MUSSANGMGR->SendMsgMussangOn()` is stubbed
//     no-op (Phase 6 pattern).

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

class cButton;
class cStatic;
class cObjectGuagen;

class cGuageDialog : public cDialog {
public:
    // 1:1 with legacy CG_* (Mussang window ids).
    static constexpr int kIdMussangBtn     = 900;
    static constexpr int kIdFlicker01      = 901;
    static constexpr int kIdGuageMussang   = 902;

    // 1:1 with legacy `FLICKER_TIME` macro = 100 ms.
    static constexpr std::uint32_t kFlickerTimeMs = 100;

    cGuageDialog();
    ~cGuageDialog() override;

    cGuageDialog(const cGuageDialog&) = delete;
    cGuageDialog& operator=(const cGuageDialog&) = delete;

    void Linking();
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
    void Render() override;

    void DisableMussangBtn(bool bDisable);
    void SetFlicker(bool bFlicker);
    void FlickerMussangGuage();

    // Test accessors.
    cButton* GetMussangButton() const noexcept { return m_pMussangBtn.get(); }
    cStatic* GetFlicker01() const noexcept { return m_pFlicker01.get(); }
    bool isFlickerActive() const noexcept     { return m_bFlicker; }
    bool isFlickerOn() const noexcept          { return m_bFlActive; }
    std::uint32_t flickSwapTime() const noexcept { return m_dwFlickerSwapTime; }
    std::uint32_t imageRGB() const noexcept    { return m_imageRGB; }
    std::uint32_t nowMillis() const noexcept   { return m_nowMillis; }

    // Test-injectable clock. Production uses SetFlicker / FlickerMussangGuage
    // which advance m_nowMillis; tests can SetMillisForTesting to drive
    // the flicker state machine deterministically.
    void SetMillisForTesting(std::uint32_t t) noexcept { m_nowMillis = t; }
    void AdvanceMillisForTesting(std::uint32_t dt) noexcept {
        m_nowMillis += dt;
    }

private:
    std::unique_ptr<cButton> m_pMussangBtn;
    std::unique_ptr<cStatic> m_pFlicker01;

    bool          m_bFlicker          = false;
    bool          m_bFlActive         = false;
    std::uint32_t m_dwFlickerSwapTime = 0;

    // 1:1 quirk: legacy `SetImageRGB(FullColor)` is stored for test
    // inspection; modern cImage GPU path is not ported (R-10) so the
    // color does not propagate to the screen.
    std::uint32_t m_imageRGB          = 0xFFFFFFFFu;

    // Test-injectable clock (0:1 oddity — replaces legacy gCurTime).
    std::uint32_t m_nowMillis         = 0;
};

} // namespace mxh::ui
