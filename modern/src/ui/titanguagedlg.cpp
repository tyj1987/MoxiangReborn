// titanguagedlg.cpp — modern port of 墨香 CTitanGuageDlg (titan HP guage).
//
// 1:1 port body. See legacy `TitanGuageDlg.cpp` for the original.

#include "titanguagedlg.hpp"

#include "cObjectGuagen.hpp"
#include "cStatic.hpp"

#include <cstdint>
#include <cstdio>
#include <memory>

namespace mxh::ui {

// 1:1 stub: legacy TITANMGR->GetTitanStats() returns a
// titan_calc_stats*. Modern port has no TITANMGR singleton; the
// stub returns the test-injectable s_titanStats (default-constructed
// → MaxFuel=0 → SetValue(0,0) and " : X/0" text). Host app can
// wire a real TITANMGR by linking a real GetTitanStats symbol.
inline TitanCalcStats StubGetTitanStats() {
    return cTitanGuageDlg::GetTitanStatsForTesting();
}

cTitanGuageDlg::cTitanGuageDlg() = default;
cTitanGuageDlg::~cTitanGuageDlg() = default;

void cTitanGuageDlg::Linking() {
    // 1:1 with legacy Linking() body. The HP guage + HP percent
    // text are resolved by id. (The MP guage + MP percent are
    // commented out in legacy cpp; modern port omits them.)
    if (!m_TitanGuage) {
        auto p = std::make_unique<cObjectGuagen>();
        p->Init(0, 0, 100, 8, nullptr, kIdTitanGuage);
        m_TitanGuage = std::move(p);
    }
    if (!m_pHpPercent) {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 60, 16, nullptr, kIdHpText);
        m_pHpPercent = std::move(p);
    }
}

void cTitanGuageDlg::SetActive(bool val) noexcept {
    // 1:1 with legacy:
    //   cDialog::SetActive(val);
    //   if(!val) {
    //     if(GAMEIN->GetTitanInventoryDlg()->IsActive())
    //       GAMEIN->GetTitanInventoryDlg()->SetActive(FALSE);
    //   }
    //
    // Modern port: GAMEIN is stubbed, so the cascade is a no-op.
    cDialog::SetActive(val);
    if (!val) {
        // 1:1 quirk: legacy cascade to TitanInventoryDlg via GAMEIN.
        // Stubbed: no-op (GAMEIN not ported). The conditional guard
        // is preserved for 1:1 fidelity.
        // (Stubbed: GAMEIN->GetTitanInventoryDlg()->SetActive(FALSE);)
    }
}

bool cTitanGuageDlg::OnActionEventStatic(std::int32_t lId, void* /*p*/,
                                        std::uint32_t /*we*/) {
    // 1:1 with legacy `static BOOL OnActionEvent`:
    //   case TITAN_GUAGE_LOOKBTN:
    //     if(GAMEIN->GetTitanInventoryDlg()->IsActive())
    //       GAMEIN->GetTitanInventoryDlg()->SetActive(FALSE);
    //     else
    //       GAMEIN->GetTitanInventoryDlg()->SetActive(TRUE);
    //   return TRUE;
    //
    // Modern port: GAMEIN is stubbed, so the toggle is a no-op.
    if (lId == kIdLookBtn) {
        // (Stubbed: toggle TitanInventoryDlg via GAMEIN.)
    }
    return true;
}

void cTitanGuageDlg::SetLife(std::uint32_t dwLife) {
    // 1:1 with legacy:
    //   titan_calc_stats* pTitanStatsInfo = TITANMGR->GetTitanStats();
    //   GUAGEVAL Life = (GUAGEVAL)dwLife / (GUAGEVAL)(pTitanStatsInfo->MaxFuel);
    //   m_TitanGuage->SetValue(Life, 0);
    //   char buf[64] = {0,};
    //   wsprintf(buf, " : %u/%u", dwLife, pTitanStatsInfo->MaxFuel);
    //   m_pHpPercent->SetStaticText(buf);
    const TitanCalcStats stats = StubGetTitanStats();
    m_stats = stats;
    // 1:1 quirk: legacy divides by MaxFuel without a zero guard.
    // If MaxFuel == 0, modern port treats Life as 0 (no division by
    // zero). Legacy would also produce 0/0 = NaN at runtime — the
    // modern port's defensive behaviour is closer to the test
    // contract.
    const float maxFuel = static_cast<float>(stats.MaxFuel);
    const float life = (maxFuel > 0.0f)
        ? (static_cast<float>(dwLife) / maxFuel)
        : 0.0f;
    if (m_TitanGuage) {
        m_TitanGuage->SetValue(life, 0u);
    }
    if (m_pHpPercent) {
        char buf[64] = { 0 };
        std::snprintf(buf, sizeof(buf), " : %u/%u", dwLife, stats.MaxFuel);
        m_pHpPercent->SetStaticText(buf);
    }
}

void cTitanGuageDlg::SetNaeRyuk(std::uint32_t /*dwNaeRyuk*/) {
    // 1:1 quirk: legacy SetNaeRyuk is fully commented out:
    //   // titan_calc_stats* pTitanStatsInfo = TITANMGR->GetTitanStats();
    //   // GUAGEVAL NaeRyuk = (GUAGEVAL)dwNaeRyuk / ...;
    //   // m_TitanGuage[eTITAN_GuageMp]->SetValue(NaeRyuk, 0);
    //   // ... m_pMpPercent->SetStaticText(buf);
    //
    // Modern port preserves the empty body verbatim.
}

} // namespace mxh::ui
