// stallkindselectdlg.hpp — modern port of 墨香
// CStallKindSelectDlg (street stall kind selector
// dialog: 3 button — sell / buy / cancel).
//
// 1:1 port of legacy `CStallKindSelectDlg` from
//   `墨香【源码】\[Client]MH\StallKindSelectDlg.h` (898 B) and
//   `墨香【源码】\[Client]MH\StallKindSelectDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_pSellBtn = m_pBuyBtn = m_pCancelBtn
//     = NULL.
//   - Dtor: empty body.
//   - Linking: resolve 3 cButton children
//     (m_pSellBtn by SO_SELLBTN, m_pBuyBtn by
//     SO_BUYBTN, m_pCancelBtn by SO_CANCELBTN).
//   - Show: SetActive(TRUE) + 3 cButton
//     SetActive(TRUE).
//   - Close: SetActive(FALSE) + 3 cButton
//     SetActive(FALSE).
//   - OnActionEvent: 3 button dispatch via
//     STREETSTALLMGR singleton:
//     * SO_SELLBTN: STREETSTALLMGR->SetStallKind
//       (eSK_SELL) + OpenStreetStall()
//     * SO_BUYBTN: STREETSTALLMGR->SetStallKind
//       (eSK_BUY) + OpenStreetStall()
//     * SO_CANCELBTN: STREETSTALLMGR->SetStallKind
//       (eSK_NULL) + SetOpenMsgBox(TRUE)
//     All 3 branches fall through to Close() at
//     the end (1:1 quirk: `else return;` for
//     unknown ids means Close() is NOT called).
//
// The modern port covers:
//   - Ctor / dtor: empty (1:1 quirk: legacy NULL
//     out 3 pointers; modern port uses default
//     member init).
//   - Linking: REAL — resolve 3 cButton children
//     by id.
//   - Show: REAL — SetActive(TRUE) + 3 cButton
//     SetActive(TRUE).
//   - Close: REAL — SetActive(FALSE) + 3 cButton
//     SetActive(FALSE).
//   - OnActionEvent: 1:1 with legacy 3-button
//     dispatch + final Close() for known ids.
//     All 3 branches are TODO (STREETSTALLMGR
//     not ported, R-12.x deferred). When ported,
//     the body becomes the legacy code.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 30th **Tier 2** dialog port (after
// cGuildInviteDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only STREETSTALLMGR singleton
// (R-12.x deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;

class cStallKindSelectDlg : public cDialog {
public:
    cStallKindSelectDlg();
    ~cStallKindSelectDlg() override;

    // ----- 1:1 with legacy CStallKindSelectDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 3 cButton
    // children (m_pSellBtn by kIdSellBtn, m_pBuyBtn
    // by kIdBuyBtn, m_pCancelBtn by kIdCancelBtn).
    void Linking();

    // ----- 1:1 with legacy CStallKindSelectDlg::Show -----

    // 1:1 with legacy Show. SetActive(TRUE) + 3
    // cButton SetActive(TRUE).
    void Show();

    // ----- 1:1 with legacy CStallKindSelectDlg::Close -----

    // 1:1 with legacy Close. SetActive(FALSE) + 3
    // cButton SetActive(FALSE).
    void Close();

    // ----- 1:1 with legacy CStallKindSelectDlg::OnActionEvent -----

    // 1:1 with legacy OnActionEvent (note: legacy
    // typo'd as "OnActionEvnet" — modern port uses
    // correct spelling). 3 button id dispatch
    // (SELL → SetStallKind(eSK_SELL) + OpenStreetStall;
    // BUY → SetStallKind(eSK_BUY) + OpenStreetStall;
    // CANCEL → SetStallKind(eSK_NULL) + SetOpenMsgBox(TRUE)).
    // All 3 branches fall through to Close() at the
    // end. Unknown ids are silently ignored (1:1
    // with legacy `else return;` — no Close() call).
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (SO_SELLBTN / SO_BUYBTN / SO_CANCELBTN).
    // Local 430-432 — distinct from 200-420 used
    // by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdSellBtn   = 430;
    static constexpr std::int32_t kIdBuyBtn    = 431;
    static constexpr std::int32_t kIdCancelBtn = 432;

private:
    // 1:1 with legacy m_pSellBtn / m_pBuyBtn /
    // m_pCancelBtn (resolved in Linking).
    cButton* m_pSellBtn   = nullptr;
    cButton* m_pBuyBtn    = nullptr;
    cButton* m_pCancelBtn = nullptr;
};

}  // namespace mxh::ui
