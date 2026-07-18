// ctabdialog.cpp — modern port of 墨香 cTabDialog (tab container).
//
// 1:1 port body. See legacy `cTabDialog.cpp` for the original.

#include "ctabdialog.hpp"

#include "cpushupbutton.hpp"
#include "cwindow.hpp"

#include <cstdint>
#include <cstring>
#include <memory>

namespace mxh::ui {

cTabDialog::cTabDialog() = default;

cTabDialog::~cTabDialog() {
    // 1:1 with legacy destructor: iterate m_bTabNum tabs and
    // SAFE_DELETE each btn + sheet, then SAFE_DELETE_ARRAY the
    // pointers. Modern port: std::vector<std::unique_ptr<...>>
    // handles cleanup automatically — no manual delete.
}

void cTabDialog::ClearTestInjections() noexcept {
    s_lastActionEventReturn = 0;
}

void cTabDialog::InitTab(std::uint8_t tabNum) {
    // 1:1 with legacy InitTab:
    //   curIdx1 = 0;
    //   curIdx2 = 0;
    //   m_bTabNum = tabNum;
    //   m_ppPushupTabBtn = new cPushupButton*[m_bTabNum];
    //   m_ppWindowTabSheet = new cWindow*[m_bTabNum];
    //   memset(m_ppPushupTabBtn, 0, sizeof(cPushupButton*)*m_bTabNum);
    //   memset(m_ppWindowTabSheet, 0, sizeof(cWindow*)*m_bTabNum);
    curIdx1_ = 0;
    curIdx2_ = 0;
    m_bTabNum    = tabNum;
    m_bSelTabNum = 0;  // 1:1 quirk: legacy m_bSelTabNum=0 in ctor
                       // is preserved as the initial selected tab.

    // 1:1 with legacy cPushupButton** allocation. Modern port
    // uses std::vector<std::unique_ptr<...>> of size m_bTabNum
    // (each slot starts as nullptr, matching memset(0, ...)).
    m_ppPushupTabBtn.clear();
    m_ppPushupTabBtn.resize(m_bTabNum);
    m_ppWindowTabSheet.clear();
    m_ppWindowTabSheet.resize(m_bTabNum);
}

void cTabDialog::SetAlpha(std::uint8_t al) {
    // 1:1 with legacy SetAlpha(BYTE al):
    //   cDialog::SetAlpha(al);
    //   for (i = 0; i < m_bTabNum; i++) {
    //       m_ppPushupTabBtn[i]->SetAlpha(al);
    //       m_ppWindowTabSheet[i]->SetAlpha(al);
    //   }
    //
    // 1:1 quirk: legacy cWindow has SetAlpha, but modern
    // cWindow doesn't (Phase 6 deferred alpha blending —
    // cDialog is the only class with SetAlpha/SetOptionAlpha).
    // Modern port: only cDialog's SetAlpha is called. The
    // cascade to tab btns + sheets is documented as 1:1
    // fidelity but no-op in the modern port.
    cDialog::SetAlpha(al);
}

void cTabDialog::SetOptionAlpha(std::uint32_t dwAlpha) {
    // 1:1 with legacy SetOptionAlpha(DWORD dwAlpha): same
    // cascade shape as SetAlpha. Modern port: cWindow doesn't
    // have SetOptionAlpha — only cDialog does. Cascade to
    // tab btns + sheets is a documented 1:1 quirk but
    // no-op in the modern port.
    cDialog::SetOptionAlpha(dwAlpha);
}

void cTabDialog::AddTabBtn(std::uint8_t idx,
                           std::unique_ptr<cPushupButton> btn) {
    // 1:1 with legacy AddTabBtn:
    //   ASSERT(idx < m_bTabNum);
    //   ASSERT(!m_ppPushupTabBtn[idx]);
    //   btn->SetAbsXY(m_absPos.x+btn->m_relPos.x, m_absPos.y+btn->m_relPos.y);
    //   btn->SetParent(this);
    //   btn->SetPassive(TRUE);
    //   if (idx == m_bSelTabNum)
    //       btn->SetPush(TRUE);
    //   else
    //       btn->SetPush(FALSE);
    //   m_ppPushupTabBtn[idx] = btn;
    //
    // 1:1 quirks:
    //   - legacy ASSERT — modern port uses silent no-op (test
    //     pattern, no exceptions in modern UI).
    //   - legacy SetParent — modern cWindow::Add auto-parent-
    //     links, but tab btns are stored in our std::vector
    //     (NOT in cDialog's children list). The override
    //     GetWindowForID is the lookup path.
    //   - legacy m_relPos.cPOINT → modern relX()/relY().
    if (idx >= m_bTabNum) { return; }
    if (!btn) { return; }
    if (m_ppPushupTabBtn[idx]) { return; }  // legacy ASSERT(!...) — silent no-op
    btn->SetAbsXY(absX() + btn->relX(), absY() + btn->relY());
    btn->SetPassive(true);
    if (idx == m_bSelTabNum) {
        btn->SetPush(true);
    } else {
        btn->SetPush(false);
    }
    m_ppPushupTabBtn[idx] = std::move(btn);
}

void cTabDialog::AddTabSheet(std::uint8_t idx,
                             std::unique_ptr<cWindow> sheet) {
    // 1:1 with legacy AddTabSheet:
    //   ASSERT(idx < m_bTabNum);
    //   ASSERT(!m_ppWindowTabSheet[idx]);
    //   sheet->SetAbsXY(m_absPos.x+sheet->m_relPos.x, m_absPos.y+sheet->m_relPos.y);
    //   sheet->SetParent(this);
    //   m_ppWindowTabSheet[idx] = sheet;
    if (idx >= m_bTabNum) { return; }
    if (!sheet) { return; }
    if (m_ppWindowTabSheet[idx]) { return; }  // legacy ASSERT(!...) — silent no-op
    sheet->SetAbsXY(absX() + sheet->relX(), absY() + sheet->relY());
    m_ppWindowTabSheet[idx] = std::move(sheet);
}

std::uint32_t cTabDialog::ActionEvent(CMouse* /*mouseInfo*/) {
    // 1:1 with legacy ActionEvent:
    //   DWORD we = WE_NULL;
    //   if (!m_bActive) return we;
    //   we = cDialog::ActionEvent(mouseInfo);
    //   DWORD we2 = WE_NULL;
    //   for (i = 0; i < m_bTabNum; i++) {
    //       we2 = m_ppPushupTabBtn[i]->ActionEvent(mouseInfo);
    //       if (we2 & WE_PUSHDOWN && m_bSelTabNum != i) {
    //           SelectTab(i);
    //           m_bSelTabNum = i;
    //       }
    //   }
    //   we |= m_ppWindowTabSheet[m_bSelTabNum]->ActionEvent(mouseInfo);
    //   return we;
    //
    // Modern port: CMouse is a stub (Phase 6.x deferred), so
    // ActionEvent is a no-op that returns WE_NULL (0) for 1:1
    // fidelity with the "no event consumed" return value.
    // Production code would link a real CMouse and call into
    // cDialog::ActionEvent + the tab btn loops.
    if (!isActive()) {
        s_lastActionEventReturn = 0;
        return 0;
    }
    // Stub: just return the test-injectable value.
    s_lastActionEventReturn = 0;
    return 0;
}

void cTabDialog::SelectTab(std::uint8_t idx) {
    // 1:1 with legacy SelectTab(BYTE idx):
    //   if (idx >= m_bTabNum) return;
    //   m_ppPushupTabBtn[idx]->SetPush(TRUE);
    //   m_ppWindowTabSheet[idx]->SetActive(TRUE);
    //   m_bSelTabNum = idx;
    //   for (j = 0; j < m_bTabNum; j++) {
    //       if (idx != j) {
    //           m_ppPushupTabBtn[j]->SetPush(FALSE);
    //           m_ppWindowTabSheet[j]->SetActive(FALSE);
    //       }
    //   }
    //
    // 1:1 quirk: legacy cWindow::SetActive → modern cWindow
    // doesn't have SetActive. Modern port: sheets use
    // SetVisible instead (per R-12, same as SetActive in
    // cTabDialog::SetActive).
    if (idx >= m_bTabNum) { return; }
    if (m_ppPushupTabBtn[idx]) {
        m_ppPushupTabBtn[idx]->SetPush(true);
    }
    if (m_ppWindowTabSheet[idx]) {
        m_ppWindowTabSheet[idx]->SetVisible(true);
    }
    m_bSelTabNum = idx;
    for (std::uint8_t j = 0; j < m_bTabNum; ++j) {
        if (idx != j) {
            if (m_ppPushupTabBtn[j]) {
                m_ppPushupTabBtn[j]->SetPush(false);
            }
            if (m_ppWindowTabSheet[j]) {
                m_ppWindowTabSheet[j]->SetVisible(false);
            }
        }
    }
}

void cTabDialog::Render() {
    // 1:1 with legacy Render:
    //   cDialog::RenderWindow();
    //   cTabDialog::RenderTabComponent();
    //   cDialog::RenderComponent();
    //
    // Modern port: all Render methods are no-op stubs (Phase
    // 6.x render wiring deferred).
    cDialog::Render();
    RenderTabComponent();
}

void cTabDialog::RenderTabComponent() {
    // 1:1 with legacy RenderTabComponent:
    //   if (!m_bActive) return;
    //   for (i = 0; i < m_bTabNum; i++) {
    //       if (m_bSelTabNum == i) m_ppWindowTabSheet[i]->Render();
    //       m_ppPushupTabBtn[i]->Render();
    //   }
    if (!isActive()) { return; }
    for (std::uint8_t i = 0; i < m_bTabNum; ++i) {
        if (m_bSelTabNum == i) {
            if (m_ppWindowTabSheet[i]) {
                m_ppWindowTabSheet[i]->Render();
            }
        }
        if (m_ppPushupTabBtn[i]) {
            m_ppPushupTabBtn[i]->Render();
        }
    }
}

void cTabDialog::SetAbsXY(std::int32_t x, std::int32_t y) noexcept {
    // 1:1 with legacy SetAbsXY:
    //   LONG tmpX = x - m_absPos.x;
    //   LONG tmpY = y - m_absPos.y;
    //   for (i = 0; i < m_bTabNum; i++) {
    //       m_ppPushupTabBtn[i]->SetAbsXY(GetAbsX()+tmpX, GetAbsY()+tmpY);
    //       m_ppWindowTabSheet[i]->SetAbsXY(GetAbsX()+tmpX, GetAbsY()+tmpY);
    //   }
    //   cDialog::SetAbsXY(x, y);
    const std::int32_t tmpX = x - absX();
    const std::int32_t tmpY = y - absY();
    for (std::uint8_t i = 0; i < m_bTabNum; ++i) {
        if (m_ppPushupTabBtn[i]) {
            m_ppPushupTabBtn[i]->SetAbsXY(
                m_ppPushupTabBtn[i]->absX() + tmpX,
                m_ppPushupTabBtn[i]->absY() + tmpY);
        }
        if (m_ppWindowTabSheet[i]) {
            m_ppWindowTabSheet[i]->SetAbsXY(
                m_ppWindowTabSheet[i]->absX() + tmpX,
                m_ppWindowTabSheet[i]->absY() + tmpY);
        }
    }
    cDialog::SetAbsXY(x, y);
}

void cTabDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive(BOOL val):
    //   if (m_bDisable) return;
    //   for (i = 0; i < m_bTabNum; i++) {
    //       m_ppPushupTabBtn[i]->SetActive(val);
    //       if (val && i == m_bSelTabNum)
    //           m_ppWindowTabSheet[i]->SetActive(val);
    //       else if (!val)
    //           m_ppWindowTabSheet[i]->SetActive(val);
    //   }
    //   cDialog::SetActiveRecursive(val);
    //
    // 1:1 quirks:
    //   - legacy `if (m_bDisable) return;` guard. Modern port:
    //     cDialog doesn't have m_bDisable (Phase 6 removed
    //     this field as a 1:1 quirk — same pattern as m_type).
    //     The guard is documented but no-op in the modern port.
    //   - legacy cWindow::SetActive → modern cWindow doesn't
    //     have SetActive (Phase 6 R-12 fix moved SetActive
    //     to cDialog). Tab btns (cPushupButton→cButton→cWindow)
    //     use SetVisible instead (per R-12 pattern, same as
    //     cGuageDialog SetActive(cStatic)→SetVisible).
    //   - legacy `cDialog::SetActiveRecursive(val)` is preserved
    //     1:1.
    for (std::uint8_t i = 0; i < m_bTabNum; ++i) {
        if (m_ppPushupTabBtn[i]) {
            m_ppPushupTabBtn[i]->SetVisible(val);
        }
        if (val && i == m_bSelTabNum) {
            if (m_ppWindowTabSheet[i]) {
                m_ppWindowTabSheet[i]->SetVisible(val);
            }
        } else if (!val) {
            if (m_ppWindowTabSheet[i]) {
                m_ppWindowTabSheet[i]->SetVisible(val);
            }
        }
    }
    cDialog::SetActiveRecursive(val);
}

cPushupButton* cTabDialog::GetTabBtn(std::uint8_t idx) const {
    if (idx >= m_bTabNum) { return nullptr; }
    return m_ppPushupTabBtn[idx].get();
}

cWindow* cTabDialog::GetTabSheet(std::uint8_t idx) const {
    if (idx >= m_bTabNum) { return nullptr; }
    return m_ppWindowTabSheet[idx].get();
}

void cTabDialog::SetDisable(bool val) noexcept {
    // 1:1 with legacy SetDisable(BOOL val):
    //   cDialog::SetDisable(val);
    //   for (i = 0; i < m_bTabNum; i++) {
    //       m_ppPushupTabBtn[i]->SetDisable(val);
    //       m_ppWindowTabSheet[i]->SetDisable(val);
    //   }
    cDialog::SetDisable(val);
    for (std::uint8_t i = 0; i < m_bTabNum; ++i) {
        if (m_ppPushupTabBtn[i]) {
            m_ppPushupTabBtn[i]->SetDisable(val);
        }
        if (m_ppWindowTabSheet[i]) {
            m_ppWindowTabSheet[i]->SetDisable(val);
        }
    }
}

cWindow* cTabDialog::FindAnyWindowForID(std::int32_t id) {
    // 1:1 with legacy GetWindowForID (renamed FindAnyWindowForID
    // because modern cDialog has no virtual GetWindowForID to
    // override; the lookup semantics are identical):
    //   cWindow* pWindow = cDialog::GetWindowForID(id);
    //   if (!pWindow) {
    //       for (i = 0; i < m_bTabNum; i++) {
    //           if (m_ppPushupTabBtn[i]->GetID() == id) { pWindow = m_ppPushupTabBtn[i]; break; }
    //           if (m_ppWindowTabSheet[i]->GetID() == id) { pWindow = m_ppWindowTabSheet[i]; break; }
    //       }
    //   }
    //   return pWindow;
    //
    // 1:1 quirks:
    //   - legacy cDialog::GetWindowForID → modern cDialog has
    //     no virtual GetWindowForID; modern port uses
    //     findWindowById (searches direct children).
    //   - legacy tab btn + sheet lookup loop is preserved 1:1.
    cWindow* pWindow = findWindowById(id);
    if (!pWindow) {
        for (std::uint8_t i = 0; i < m_bTabNum; ++i) {
            if (m_ppPushupTabBtn[i] &&
                m_ppPushupTabBtn[i]->id() == id) {
                pWindow = m_ppPushupTabBtn[i].get();
                break;
            }
            if (m_ppWindowTabSheet[i] &&
                m_ppWindowTabSheet[i]->id() == id) {
                pWindow = m_ppWindowTabSheet[i].get();
                break;
            }
        }
    }
    return pWindow;
}

} // namespace mxh::ui
