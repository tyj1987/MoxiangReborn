// munpamarkdialog.hpp — modern port of 墨香 CMunpaMarkDialog (guild mark UI).
//
// 1:1 port of legacy `CMunpaMarkDialog` from
//   `墨香【源码】\[Client]MH\MunpaMarkDialog.{h,cpp}`.
//
// CMunpaMarkDialog is a small dialog that displays a single guild's
// mark (the heraldic icon of a guild). It owns a single `CMunpaMark*`
// (a 4Dyuchi render object) and renders it in the dialog's bounds
// each frame.
//
// 1:1 contract preserved:
//   - Init(x, y, wid, hei, basicImage, id=0) — modern port does NOT
//     override Init; the base cDialog::Init signature is already
//     1:1 with the legacy override (6 params, default id=0). The
//     legacy Init body was: `cDialog::Init(...) + m_type = WT_*`.
//     Modern port drops the m_type assignment (R-12 fix).
//   - SetMunpaMark(DWORD MunpaID) — looks up the mark via
//     `MUNPAMARKMGR->GetMunpaMark(MunpaID)` (global singleton, stubbed
//     no-op in modern port) and stores the pointer. Returns TRUE on
//     hit, FALSE on miss.
//   - Render() — base cDialog::Render + (if mark exists)
//     m_pMunpaMark->Render(&m_absPos). The munpamark.Render is a
//     no-op stub in modern port (R-10 cImage GPU 1:1 deferred).
//
// 1:1 quirks preserved:
//   - 1:1 quirk: legacy `m_type = WT_MUNPAMARKDLG` is dropped (R-12
//     fix; modern cWindow has no m_type field).
//   - 1:1 quirk: legacy `CMunpaMark` is a forward-declared class;
//     modern port keeps it as forward decl with a 1:1 signature stub
//     `void Render(std::int32_t* pAbsPos)`. The actual munpamark
//     rendering is stubbed in the modern port (4Dyuchi DLL not
//     ported).
//   - 1:1 quirk: legacy `MUNPAMARKMGR` global singleton is stubbed
//     no-op (returns nullptr from GetMunpaMark, mimicking a "no
//     mark found" case).
//   - 1:1 quirk: legacy `Render` does `cDialog::Render() +
//     m_pMunpaMark->Render(&m_absPos)`. Modern port does the same
//     but `m_pMunpaMark` is always nullptr (stub), so the second
//     call is a no-op.
//   - 1:1 quirk: legacy 6-param Init default id=0 is preserved by
//     NOT overriding Init; the base signature is 1:1.
//   - 1:1 quirk: legacy `(CMunpaMark*)MUNPAMARKMGR->GetMunpaMark(...)`
//     — modern port has a similar `MunpaMark* getMunpaMark(DWORD)`
//     stub that returns nullptr.

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

// Stub definition of CMunpaMark (the 4Dyuchi render object). 1:1
// with legacy; modern port provides an inline stub with a virtual
// Render method (the actual 4Dyuchi render DLL is not ported). Host
// apps can replace this stub with a real implementation by linking
// a separate translation unit that overrides the vtable.
class CMunpaMark {
public:
    virtual ~CMunpaMark() = default;
    // 1:1 quirk: legacy `Render(LONG* pAbsPos)` is called with
    // `&m_absPos` (a 2-int struct). Modern port passes a 2-int
    // array (same shape, same memory layout).
    virtual void Render(std::int32_t* pAbsPos) noexcept {
        (void)pAbsPos;  // no-op stub; production wires a 4Dyuchi render
    }
};

class cMunpaMarkDialog : public cDialog {
public:
    cMunpaMarkDialog();
    ~cMunpaMarkDialog() override;

    cMunpaMarkDialog(const cMunpaMarkDialog&) = delete;
    cMunpaMarkDialog& operator=(const cMunpaMarkDialog&) = delete;

    // 1:1 with legacy: Init override is dropped (base cDialog::Init
    // is 1:1 with the legacy body after the m_type=WT_* removal).

    // 1:1 with legacy: returns TRUE on hit (legacy returned non-zero),
    // FALSE on miss (legacy returned 0).
    bool SetMunpaMark(std::uint32_t MunpaID);

    void Render() override;

    // Test accessors.
    bool hasMunpaMark() const noexcept { return m_pMunpaMark != nullptr; }
    CMunpaMark* munpaMark() const noexcept { return m_pMunpaMark; }

    // Test-injectable stub for the MUNPAMARKMGR singleton. Production
    // wires a real GetMunpaMark. Tests can call this to inject a
    // non-null mark and verify Render delegation.
    void SetMunpaMarkForTesting(CMunpaMark* p) noexcept { m_pMunpaMark = p; }

private:
    CMunpaMark* m_pMunpaMark = nullptr;
};

} // namespace mxh::ui
