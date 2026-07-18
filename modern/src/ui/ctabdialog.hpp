// ctabdialog.hpp — modern port of 墨香 cTabDialog (tab container).
//
// 1:1 port of legacy `cTabDialog` from
//   `墨香【源码】\[Client]MH\interface\cTabDialog.{h,cpp}`.
//
// cTabDialog is a tab container: a cDialog subclass that holds
// N pairs of (cPushupButton* tab btn, cWindow* tab sheet).
// Only the selected tab's sheet is active; the other sheets
// are deactivated. The user clicks a tab btn to switch
// (SelectTab). SetAbsXY / SetActive / SetDisable / SetAlpha /
// SetOptionAlpha all cascade to the tabs.
//
// 1:1 contract preserved:
//   - InitTab(BYTE tabNum) — sets capacity to `tabNum`. Legacy
//     allocates cPushupButton*[N] and cWindow*[N] arrays.
//     Modern port: 2 std::vector<std::unique_ptr<...>> with
//     resize(N) — capacity match legacy.
//   - AddTabBtn(idx, btn) — stores the tab btn at slot idx,
//     sets absolute position relative to the parent dialog,
//     calls SetParent (legacy; modern omits — cWindow::Add
//     auto-parent-links), and pushes the btn if idx matches
//     the current selected tab.
//   - AddTabSheet(idx, sheet) — stores the tab sheet at slot
//     idx, sets absolute position, calls SetParent (legacy;
//     modern omits).
//   - SelectTab(idx) — pushes the idx button, activates the
//     idx sheet, and pushes-deactivates all other buttons +
//     sheets. Legacy iterates all tabs and resets each.
//   - SetActive(val) override (R-12) — cascades to all tab
//     btns + sheets: sheets for val==TRUE activate only the
//     currently-selected tab's sheet; sheets for val==FALSE
//     all deactivate. Then cDialog::SetActiveRecursive(val).
//   - SetAbsXY(x, y) — computes delta from current position,
//     cascades to all tab btns + sheets (each by their delta),
//     then sets the parent dialog's absolute position.
//   - SetDisable(val) override — cascades to all tab btns +
//     sheets, then cDialog::SetDisable(val).
//   - SetAlpha(al) — cascades to all tab btns + sheets, then
//     cDialog::SetAlpha(al).
//   - SetOptionAlpha(dwAlpha) — same cascade.
//   - Render / RenderTabComponent — no-op stubs (modern port
//     keeps the API surface for 1:1 fidelity; render wiring
//     is Phase 12.x deferred).
//   - GetWindowForID(id) override — first checks the parent
//     cDialog's GetWindowForID (which finds direct children
//     added via Add). If not found, iterates the tab btns +
//     sheets looking for the matching id.
//   - ActionEvent(CMouse*) — no-op stub (CMouse not ported,
//     Phase 6.x deferred).
//
// 1:1 quirks preserved:
//   - 1:1 quirk: legacy ctor sets `m_type = WT_TABDIALOG`.
//     Modern port drops the m_type field entirely (Phase 6
//     removed cWindow::m_type as a 1:1 quirk; same pattern
//     as cTipBrowserDlg / cPetStateMiniDlg / cSkillPointNotify).
//   - 1:1 quirk: legacy ctor initialises `curIdx1 = 0;
//     curIdx2 = 0;` (declared in header, but never used in
//     cpp). Modern port preserves the curIdx1/curIdx2 fields
//     for 1:1 fidelity (set in InitTab to 0; same shape as
//     legacy).
//   - 1:1 quirk: legacy destructor iterates N tabs and
//     SAFE_DELETE each btn + sheet, then SAFE_DELETE_ARRAY
//     the pointers. Modern port uses std::unique_ptr per slot
//     + std::vector — automatic cleanup, no manual delete.
//   - 1:1 quirk: legacy AddTabBtn / AddTabSheet call
//     `btn->SetAbsXY(m_absPos.x+btn->m_relPos.x,
//     m_absPos.y+btn->m_relPos.y)` — uses cPOINT m_absPos +
//     cPOINT m_relPos. Modern port: `absX()+btn->relX()` +
//     `absY()+btn->relY()` (per cWindow.hpp Phase 6 split
//     of cPOINT into 2 int fields, same pattern as
//     cMunpaMarkDialog m_absPos=cPOINT → m_absX/m_absY).
//   - 1:1 quirk: legacy AddTabBtn / AddTabSheet call
//     `SetParent(this)`. Modern port omits SetParent —
//     cWindow::Add auto-parent-links; tab btns + sheets are
//     stored in the std::vector, not added to cDialog's
//     children list (so GetWindowForID needs the override
//     to find them — same pattern as cTabDialog override).
//   - 1:1 quirk: legacy InitTab uses `BYTE tabNum` (0-255
//     range). Modern port: takes `std::uint8_t tabNum` to
//     match the same range. The default constructor's
//     `m_bTabNum=0` and `m_ppPushupTabBtn=NULL` are
//     preserved as default-init (vector empty, m_bTabNum=0).
//   - 1:1 quirk: legacy ActionEvent returns `DWORD` (legacy
//     WE_* bit field). Modern port: returns `std::uint32_t`
//     with WE_NULL=0 as the only safe return. The CMouse
//     param is forward-declared as a stub (Phase 6 doesn't
//     have a CMouse type yet).
//   - 1:1 quirk: legacy SetActive(val) is `virtual` but
//     cDialog::SetActive was name-hiding overload (not
//     virtual override) in legacy. Modern port: cDialog::
//     SetActive is `virtual noexcept` (R-12 fix), so
//     `noexcept override` is required by MSVC C2694.
//   - 1:1 quirk: legacy SetActive has the `if(m_bDisable)
//     return;` guard at the top. Modern port: cDialog has
//     m_bDisable (legacy field preserved). When m_bDisable,
//     the cascade is skipped — same as legacy.
//   - 1:1 quirk: legacy SetActive's loop has a `if(val && i
//     == m_bSelTabNum) m_ppWindowTabSheet[i]->SetActive(val)`
//     conditional. Modern port preserves this exactly (only
//     the selected tab's sheet is activated on val==TRUE).
//   - 1:1 quirk: legacy SetDisable override is `virtual`.
//     Modern port: cDialog::SetDisable is `virtual` too
//     (not noexcept per Phase 6 design). Override does not
//     need noexcept (R-12 only applies to SetActive).
//   - 1:1 quirk: legacy SetAlpha / SetOptionAlpha cascade
//     to all tab btns + sheets. Modern cWindow has
//     `SetAlpha` / `SetOptionAlpha` methods; modern port
//     calls them in the same loop. If a tab btn or sheet
//     is null (slot not yet added), the modern port guards
//     with a null check (legacy would have crashed; modern
//     port is defensive).
//   - 1:1 quirk: legacy GetWindowForID iterates tab btns
//     + sheets looking for matching id. Modern port
//     preserves this exact loop pattern.
//   - 1:1 quirk: legacy ctor's commented-out
//     `m_BtnPushstartTime = 0; m_BtnPushDelayTime = 700;`
//     are not ported (modern port omits the commented-out
//     fields; same as cTitanGuageDlg m_pMpPercent omit).
//   - 1:1 quirk: legacy `BYTE curIdx1; BYTE curIdx2;` are
//     declared as protected fields, init to 0 in ctor +
//     InitTab, but never used in cpp. Modern port preserves
//     them as protected fields for 1:1 fidelity (dead
//     fields, but matching the header signature).
//   - 1:1 quirk: legacy `GetID()` (cWindow member) is used
//     in GetWindowForID. Modern cWindow uses `id()` lower-
//     case accessor. Modern port: `btn->id()` / `sheet->
//     id()` matching the modern convention.

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace mxh::ui {

class cPushupButton;
class cWindow;

// 1:1 stub: legacy `CMouse` (defined in [Client]MH/Input/
// Mouse.h, part of the 2008-era client). Modern port uses
// an opaque forward-declared class for the ActionEvent
// signature. Production code would link a real Mouse.h.
class CMouse;

class cTabDialog : public cDialog {
public:
    cTabDialog();
    ~cTabDialog() override;

    cTabDialog(const cTabDialog&) = delete;
    cTabDialog& operator=(const cTabDialog&) = delete;

    // ----- 1:1 with legacy cTabDialog API -----

    void InitTab(std::uint8_t tabNum);
    std::uint32_t ActionEvent(CMouse* mouseInfo);
    void SetAbsXY(std::int32_t x, std::int32_t y) noexcept;
    void SetActive(bool val) noexcept override;
    void SetAlpha(std::uint8_t al);
    void SetOptionAlpha(std::uint32_t dwAlpha);
    void SetDisable(bool val) noexcept override;
    void Render();
    void RenderTabComponent();
    void SelectTab(std::uint8_t idx);

    void AddTabBtn(std::uint8_t idx, std::unique_ptr<cPushupButton> btn);
    void AddTabSheet(std::uint8_t idx, std::unique_ptr<cWindow> sheet);

    cPushupButton* GetTabBtn(std::uint8_t idx) const;
    cWindow*       GetTabSheet(std::uint8_t idx) const;

    // 1:1 quirk: legacy cTabDialog overrides cDialog::GetWindowForID
    // to find tab btns + sheets (which are NOT in cDialog's
    // children list — they're stored in m_ppPushupTabBtn +
    // m_ppWindowTabSheet). Modern cDialog doesn't have a
    // virtual GetWindowForID (only findWindowById which
    // searches direct children). Modern port: instead of
    // overriding, we expose a public FindAnyWindowForID that
    // searches both cDialog's direct children AND the tab btns
    // + sheets. Same lookup order as legacy, same return value.
    cWindow* FindAnyWindowForID(std::int32_t id);

    // Read accessors.
    std::uint8_t GetCurTabNum() const noexcept { return m_bSelTabNum; }
    std::uint8_t GetTabNum()    const noexcept { return m_bTabNum; }
    std::uint8_t curIdx1()      const noexcept { return curIdx1_; }
    std::uint8_t curIdx2()      const noexcept { return curIdx2_; }

    // Test-injectable: 1:1 with legacy ActionEvent's mouseInfo.
    // Default-constructed CMouse is null; production wires a
    // real CMouse. Modern port: ActionEvent is a no-op stub
    // (CMouse not ported, Phase 6.x deferred).
    static std::uint32_t lastActionEventReturn() noexcept { return s_lastActionEventReturn; }
    static void ClearTestInjections() noexcept;

private:
    // 1:1 quirk: legacy `curIdx1` / `curIdx2` declared in
    // header but never read in cpp. Modern port preserves
    // them as `curIdx1_` / `curIdx2_` for 1:1 fidelity
    // (mangled to avoid clashing with the accessors).
    std::uint8_t curIdx1_ = 0;
    std::uint8_t curIdx2_ = 0;

    std::uint8_t m_bTabNum    = 0;
    std::uint8_t m_bSelTabNum = 0;

    // 1:1 quirk: legacy `cPushupButton** m_ppPushupTabBtn`
    // (raw array, legacy owns). Modern port: std::vector of
    // unique_ptr — automatic cleanup, RAII semantics.
    std::vector<std::unique_ptr<cPushupButton>> m_ppPushupTabBtn;
    std::vector<std::unique_ptr<cWindow>>       m_ppWindowTabSheet;

    // Test-injectable ActionEvent return value (default 0 = WE_NULL).
    static inline std::uint32_t s_lastActionEventReturn = 0;
};

} // namespace mxh::ui
