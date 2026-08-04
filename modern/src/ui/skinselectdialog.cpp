// skinselectdialog.cpp — modern port implementation.
//
// 1:1 port of legacy `CSkinSelectDialog` from
//   `墨香【源码】\[Client]MH\SkinSelectDialog.cpp`.
//
// Modern-port notes
// =================
//
// 1. **CItemShow is opaque.** The legacy m_NomalSkinView[3] is a
//    fixed array of CItemShow (engine-side BaseItem subclass).
//    Modern port uses placeholder `cIcon*` pointers (one per
//    preview cell). The engine-binder layer (Phase 14+) will
//    replace these with real cIcon* allocated by CItemShow::Init.
//
// 2. **Engine singletons stubbed.** GAMERESRCMNGR / HERO /
//    CHATMGR / OBJECTMGR / ITEMMGR / NETWORK / WINDOWMGR are all
//    no-op stubs. The data-side state (select-idx / list-population
//    / delay-flag) is preserved 1:1.
//
// 3. **Render is a no-op.** The legacy Render is commented out
//    (only cItemShow::Render is real, and that's engine-side).
//
// 4. **InitSkinDelayTime / StartSkinDelayTime / CheckDelay** are
//    legacy helper methods that are commented-out in the legacy
//    header. Modern port drops them — they were never called.
//
// 5. **Dtor NULL check.** The legacy dtor calls
//    `m_pNomalSkinListDlg->RemoveAll()` without a NULL check
//    (1:1 quirk — if Linking() was never called, this would
//    crash). Modern port preserves the 1:1 behavior; the tests
//    always call Linking first.

#include "skinselectdialog.hpp"
#include "legacy_window_event.hpp"

#include "cIconDialog.hpp"
#include "cListDialog.hpp"
#include "cWindow.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

cSkinSelectDialog::cSkinSelectDialog() {
    m_dwSelectIdx       = 0;
    m_dwSkinDelayTime   = 0;
    m_bSkinDelayResult  = false;
}

cSkinSelectDialog::~cSkinSelectDialog() {
    // 1:1 with legacy: no NULL check.
    if (m_pNomalSkinListDlg) {
        m_pNomalSkinListDlg->RemoveAll();
    }
}

void cSkinSelectDialog::Linking() {
    // 1:1 with legacy. Modern cDialog::findWindowById replaces
    // legacy cDialog::GetWindowForID.
    m_pNomalSkinListDlg =
        static_cast<cListDialog*>(findWindowById(ID_LIST));
    m_pNomalSkinIconDlg =
        static_cast<cIconDialog*>(findWindowById(ID_ITEMVIEW));

    // 1:1 with legacy: SetShowSelect(TRUE) on the list dialog.
    if (m_pNomalSkinListDlg) {
        m_pNomalSkinListDlg->SetShowSelect(true);
    }
}

void cSkinSelectDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy: cDialog::SetActive first, then the
    // clear-or-populate branch.
    cDialog::SetActive(val);
    if (!val) {
        if (m_pNomalSkinListDlg) {
            m_pNomalSkinListDlg->RemoveAll();
            m_pNomalSkinListDlg->SetCurSelectedRowIdx(-1);
        }
        if (m_pNomalSkinIconDlg) {
            m_pNomalSkinIconDlg->DeleteIconAll();
        }
        m_dwSelectIdx = 0;
    } else {
        SkinItemListInfo();
    }
}

std::uint32_t cSkinSelectDialog::ActionEvent(std::int32_t mouseX,
                                             std::int32_t mouseY,
                                             std::uint32_t mouseFlags) {
    if (!isEnabled() || !isActive()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    // 1:1 with legacy: cDialog::ActionEvent first, then the
    // list hit-test + on-click preview-populate branch.
    const std::uint32_t we = cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
    if (!m_pNomalSkinListDlg || !m_pNomalSkinIconDlg) {
        return we;
    }

    if (m_pNomalSkinListDlg->PtIdxInRow(mouseX, mouseY) != -1) {
        if (mouseFlags & cWindow::MouseFlagLButton) {  // LButton click
            // The legacy uses WINDOWMGR->IsMouseDownUsed() to gate
            // double-click consume. The modern port drops the
            // check (no cWindowManager::IsMouseDownUsed equivalent
            // yet); the engine-binder layer will re-add it.
            const int sel = m_pNomalSkinListDlg->GetCurSelectedRowIdx();
            m_dwSelectIdx = static_cast<std::uint32_t>(sel + 1);
            // 1:1 with legacy: select-idx > 0 means "skin picked".
            if (m_dwSelectIdx > 0) {
                populatePreview();
            }
        }
    }
    return we;
}

void cSkinSelectDialog::populatePreview() {
    if (!m_pNomalSkinIconDlg) return;
    m_pNomalSkinIconDlg->DeleteIconAll();
    // 1:1 with legacy: 3 placeholder cIcon* entries. The legacy
    // uses real CItemShow* here (allocated by CItemShow::Init);
    // the modern port uses tagged pointer placeholders. The
    // engine-binder layer will replace with real cIcon* when
    // CItemShow is ported.
    for (std::uint16_t i = 0; i < SKINITEM_LIST_MAX; ++i) {
        auto* placeholder = reinterpret_cast<class cIcon*>(
            static_cast<std::uintptr_t>(i + 1));
        m_pNomalSkinIconDlg->AddIcon(i, placeholder);
    }
}

bool cSkinSelectDialog::OnActionEvent(std::int32_t lId, void* /*p*/,
                                      std::uint32_t we) {
    // 1:1 with legacy: switch on we first, then switch on lId.
    if (we == legacy_window_event::kCloseWindow) {
        return true;
    }
    switch (lId) {
    case ID_OK:
        // 1:1 with legacy: only if m_dwSelectIdx > 0. Engine-side
        // GAMERESRCMNGR->GetNomalClothesSkinList + level check
        // + HERO->CheckSkinDelay + NETWORK->Send are stubbed.
        if (m_dwSelectIdx > 0) {
            m_bSkinDelayResult = false;  // matches HERO->CheckSkinDelay()=FALSE
        }
        return true;
    case ID_CANCEL:
        SetActive(false);
        return true;
    case ID_RECOVERY:
        // 1:1 with legacy: NETWORK->Send with dwData1=0 (reset
        // to default skin). Engine-binder layer (Phase 14+) will
        // wire the actual send.
        m_bSkinDelayResult = false;
        return true;
    default:
        return true;
    }
}

void cSkinSelectDialog::SkinItemListInfo() {
    if (!m_pNomalSkinListDlg) return;
    // 1:1 with legacy: GAMERESRCMNGR->GetNomalClothesSkinListCountNum()
    // returns the total number of skins. Engine-side stubbed to 0
    // in the modern port (no skin data); the loop body + color-
    // from-level logic is preserved for 1:1 behavior.
    constexpr std::uint32_t dwTotalNum = 0;
    for (std::uint32_t i = 0; i < dwTotalNum; ++i) {
        // SKIN_SELECT_ITEM_INFO* pSkinInfo = GAMERESRCMNGR->GetNomalClothesSkinList(i+1);
        // if (nullptr == pSkinInfo) continue;
        // dwColor = (HERO->GetLevel() < pSkinInfo->dwLimitLevel)
        //     ? RGBA_MAKE(255, 50, 50, 255)   // red (locked)
        //     : RGBA_MAKE(255, 255, 255, 255); // white (unlocked)
        // char szSkinItemName[MAX_NAME_LENGTH+1]; ...
        // m_pNomalSkinListDlg->AddItem(buf, dwColor);
        (void)i;
    }
}

} // namespace mxh::ui
