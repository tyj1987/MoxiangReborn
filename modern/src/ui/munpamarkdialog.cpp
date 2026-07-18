// munpamarkdialog.cpp — modern port of 墨香 CMunpaMarkDialog (guild mark UI).
//
// 1:1 port body. See legacy `MunpaMarkDialog.cpp` for the original.

#include "munpamarkdialog.hpp"

#include "cWindow.hpp"  // for absX() / absY() accessors

#include <cstdint>

// CMunpaMark is defined inline in the header (so tests can extend
// it for verification). This translation unit just uses the
// definition.

namespace mxh::ui {

// 1:1 stub: legacy MUNPAMARKMGR->GetMunpaMark(MunpaID) returns a
// CMunpaMark* (or NULL). Modern port has no MUNPAMARKMGR singleton;
// the stub returns nullptr (mimicking "no mark found" — the legacy
// code would set m_pMunpaMark = NULL and return FALSE). Host app can
// wire a real MUNPAMARKMGR by linking before the dialog lib (per
// Phase 6 stub pattern).
inline CMunpaMark* StubGetMunpaMark(std::uint32_t /*MunpaID*/) {
    return nullptr;
}

cMunpaMarkDialog::cMunpaMarkDialog() = default;
cMunpaMarkDialog::~cMunpaMarkDialog() = default;

bool cMunpaMarkDialog::SetMunpaMark(std::uint32_t MunpaID) {
    // 1:1 with legacy:
    //   m_pMunpaMark = MUNPAMARKMGR->GetMunpaMark(MunpaID);
    //   if(m_pMunpaMark) return TRUE;
    //   else return FALSE;
    m_pMunpaMark = StubGetMunpaMark(MunpaID);
    return m_pMunpaMark != nullptr;
}

void cMunpaMarkDialog::Render() {
    // 1:1 with legacy:
    //   cDialog::Render();
    //   if(m_pMunpaMark) m_pMunpaMark->Render(&m_absPos);
    //
    // Modern port: cDialog::Render is a no-op stub (Phase 6.3 GPU path
    // not ported). The conditional guard is preserved verbatim. When
    // a test injects a mark via SetMunpaMarkForTesting, the call
    // delegates to the injected mark's Render (which is a no-op stub
    // in production, but the test's TestMunpaMark::Render records
    // the call).
    //
    // 1:1 quirk: legacy m_absPos is a cPOINT (2-int struct). Modern
    // cWindow stores m_absX / m_absY as separate int fields, so the
    // modern port synthesises a 2-int array on the fly and passes a
    // pointer to it. The 1:1 contract preserved is "the mark is
    // handed a pointer to the dialog's (x, y) origin".
    cDialog::Render();
    if (m_pMunpaMark) {
        // 1:1 quirk: legacy passes &m_absPos (a cPOINT 2-int struct).
        // Modern cWindow exposes absX()/absY() accessors; m_absX and
        // m_absY are private. We synthesise the 2-int array on the fly
        // to preserve the legacy call shape.
        std::int32_t absPos[2] = { absX(), absY() };
        m_pMunpaMark->Render(absPos);
    }
}

} // namespace mxh::ui
