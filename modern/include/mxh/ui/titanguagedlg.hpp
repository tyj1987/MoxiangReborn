// titanguagedlg.hpp — modern port of 墨香 CTitanGuageDlg (titan HP guage).
//
// 1:1 port of legacy `CTitanGuageDlg` from
//   `墨香【源码】\[Client]MH\TitanGuageDlg.{h,cpp}`.
//
// CTitanGuageDlg displays the titan's HP gauge and HP percentage
// text. It owns a CObjectGuagen (the visual bar) and a cStatic
// (the text label) resolved at Linking() by id.
//
// 1:1 contract preserved:
//   - Linking() resolves 2 children by id: 1 cObjectGuagen (HP bar)
//     and 1 cStatic (HP percent text). The legacy m_TitanGuage
//     array is fully commented out — only m_TitanGuage (the HP
//     bar) is active in the modern port.
//   - SetActive(val) override — calls base cDialog::SetActive, and
//     when deactivated cascades to close the TitanInventoryDlg via
//     the GAMEIN singleton. Modern port: GAMEIN is stubbed, so the
//     cascade is a no-op.
//   - OnActionEvent(lId, p, we) is a static method in legacy.
//     Modern port: keeps the static qualifier for 1:1 fidelity.
//     The TITAN_GUAGE_LOOKBTN branch toggles the TitanInventoryDlg
//     via GAMEIN — stubbed no-op.
//   - SetLife(dwLife) — reads the titan stats via TITANMGR,
//     computes Life = dwLife / MaxFuel, calls SetValue on the
//     CObjectGuagen, and sets the percent text via sprintf.
//     Modern port: TITANMGR is stubbed (returns nullptr from
//     GetTitanStats → MaxFuel=0 → SetValue(0,0) and percent text
//     " : X/0"). SetLife is testable with a test-injectable
//     TITANMGR stub (SetTitanStatsForTesting).
//   - SetNaeRyuk(dwNaeRyuk) — fully commented out in legacy. Modern
//     port preserves this 1:1 quirk (no-op body).
//
// 1:1 quirks preserved:
//   - 1:1 quirk: legacy `enum eTitanGuage` is defined but unused
//     (the array slot is commented out). Modern port omits the
//     enum entirely (no 1:1 fidelity is gained by including a
//     dead enum).
//   - 1:1 quirk: legacy `CObjectGuagen* m_TitanGuage[eTITAN_GuadgeMax]`
//     array is commented out; only the single `m_TitanGuage` slot
//     is active. Modern port: single member (no array).
//   - 1:1 quirk: legacy `cStatic* m_pMpPercent` is declared but
//     never used (no Linking call). Modern port omits this field.
//   - 1:1 quirk: legacy `TITANMGR` global singleton is stubbed
//     no-op per Phase 6 pattern.
//   - 1:1 quirk: legacy `titan_calc_stats*` struct (forward
//     decl, defined in `GameResourceStruct.h`) is replaced by a
//     modern port inline struct with the same field layout
//     (MaxFuel / MaxSpell). Production code would link a real
//     GameResourceStruct.h via the [Client]MH legacy tree.
//   - 1:1 quirk: legacy `static BOOL OnActionEvent` is preserved
//     as a static method.
//   - 1:1 quirk: legacy SetActive override is preserved (R-12 fix:
//     override must be noexcept to match the virtual spec).
//   - 1:1 quirk: legacy `SetNaeRyuk` body is fully commented
//     out. Modern port: empty body (no-op).

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

class cStatic;
class cObjectGuagen;

// 1:1 stub: legacy `titan_calc_stats` struct (defined in
// `GameResourceStruct.h`, part of the 2008-era client). Modern port
// provides a minimal inline definition with the same field
// names (MaxFuel / MaxSpell) so SetLife can be ported 1:1.
// Production code can replace this stub by linking a real
// `GameResourceStruct.h` from the legacy tree.
struct TitanCalcStats {
    std::uint32_t MaxFuel  = 0;
    std::uint32_t MaxSpell = 0;
};

// Forward decl of TITANMGR (the titan manager global). Modern port
// treats the singleton as a no-op stub; the test-injectable
// SetTitanStatsForTesting path lets unit tests inject a real
// stats struct.
class CTitanManager;

class cTitanGuageDlg : public cDialog {
public:
    // 1:1 with legacy TITAN_GUAGE_* ids.
    static constexpr int kIdTitanGuage = 1000;
    static constexpr int kIdHpText     = 1001;
    static constexpr int kIdLookBtn    = 1002;

    // 1:1 with legacy OnActionEvent — static method in legacy too.
    static bool OnActionEventStatic(std::int32_t lId, void* p,
                                   std::uint32_t we);

    cTitanGuageDlg();
    ~cTitanGuageDlg() override;

    cTitanGuageDlg(const cTitanGuageDlg&) = delete;
    cTitanGuageDlg& operator=(const cTitanGuageDlg&) = delete;

    void Linking();
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy SetLife / SetNaeRyuk.
    void SetLife(std::uint32_t dwLife);
    void SetNaeRyuk(std::uint32_t dwNaeRyuk);

    // Test accessors.
    cObjectGuagen* guage() const noexcept { return m_TitanGuage.get(); }
    cStatic* hpPercentText() const noexcept { return m_pHpPercent.get(); }
    const TitanCalcStats& statsForTesting() const noexcept { return m_stats; }

    // Test-injectable TITANMGR stub. Production code would link a
    // real TITANMGR by defining a real GetTitanStats symbol and
    // linking it before the dialog lib (per Phase 6 stub pattern).
    static void SetTitanStatsForTesting(const TitanCalcStats& s) noexcept {
        s_titanStats = s;
    }
    static void ClearTitanStatsForTesting() noexcept {
        s_titanStats = TitanCalcStats{};
    }
    // Internal helper for the file-scope stub (declared public so
    // the inline function below can read s_titanStats).
    static const TitanCalcStats& GetTitanStatsForTesting() noexcept {
        return s_titanStats;
    }

private:
    std::unique_ptr<cObjectGuagen> m_TitanGuage;
    std::unique_ptr<cStatic>       m_pHpPercent;

    // Test-injectable stats (replaces the legacy TITANMGR singleton
    // for unit testing). Default-constructed: MaxFuel=0, MaxSpell=0.
    static inline TitanCalcStats s_titanStats{};
    TitanCalcStats m_stats;  // last-seen stats (for inspection)
};

} // namespace mxh::ui
