// cstallkindselectdlg.hpp -- modern port of Moxiang
//   CStallKindSelectDlg (street stall kind selector).
//
// 1:1 port of legacy `CStallKindSelectDlg` from
//   `[Client]MH\StallKindSelectDlg.{h,cpp}`.
//
// 3 cButton children (Sell / Buy / Cancel) + a click
// dispatcher that -- in the legacy -- fires a
// STREETSTALLMGR singleton.  The modern port replaces
// the singleton with 3 host-injected callbacks so the
// dialog remains drivable in unit tests + from any
// service adapter that integrates with the modern
// StreetStallManager.
//
// 1:1 dependencies:
//   * 3 cButton children (m_pSellBtn / m_pBuyBtn /
//     m_pCancelBtn)
//   * STREETSTALLMGR->SetStallKind(eSK_SELL | eSK_BUY |
//     eSK_NULL)
//   * STREETSTALLMGR->OpenStreetStall() (Sell / Buy)
//   * STREETSTALLMGR->SetOpenMsgBox(TRUE) (Cancel)
//   * Close() at the end of every handled branch
//     (1:1 with legacy `Close();` after the singleton
//     dispatch).
//
// 1:1 quirks:
//   - 1:1 with legacy `m_pSellBtn = m_pBuyBtn = m_pCancelBtn
//     = NULL;` -- modern port uses default member init.
//   - 1:1 with legacy Show: SetActive(TRUE) + 3 cButton
//     SetActive(TRUE).  Modern cButton does not have
//     SetActive (cButton extends cWindow which has no
//     SetActive).  Modern port uses cWindow::SetVisible
//     as the 1:1 equivalent (see R-12 fix).
//   - 1:1 with legacy Close: SetActive(FALSE) + 3 cButton
//     SetActive(FALSE).  Same SetVisible mapping.
//   - 1:1 with legacy `else return;` for unknown ids --
//     Close() is NOT called for unknown ids.  Modern port
//     uses `if (handled) Close();` to preserve the
//     semantics.
//   - 1:1 with legacy WE_BTNCLICK constant (legacy
//     value 0x0001).  Modern port uses a local constant
//     `kWeBtnClick = 0x0001` (the legacy WE_BTNCLICK
//     was repurposed in cWindowDef.h and is not
//     re-exported by modern cWindowEvent).

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;

// 1:1 with legacy STALL_KIND enum (StallKindSelectDlg.h).
//   eSK_NULL  = 0
//   eSK_SELL  = 1
//   eSK_BUY   = 2
enum class StallKind : std::int32_t {
    Null = 0,
    Sell = 1,
    Buy  = 2,
};

class cStallKindSelectDlg : public cDialog {
public:
    cStallKindSelectDlg();
    ~cStallKindSelectDlg() override;

    cStallKindSelectDlg(const cStallKindSelectDlg&) = delete;
    cStallKindSelectDlg& operator=(const cStallKindSelectDlg&) = delete;

    // 1:1 with legacy CStallKindSelectDlg::Linking.
    // Resolves the 3 cButton children by id.
    void Linking();

    // 1:1 with legacy CStallKindSelectDlg::Show.
    // SetActive(true) + 3 cButton visible(true).
    void Show();

    // 1:1 with legacy CStallKindSelectDlg::Close.
    // SetActive(false) + 3 cButton visible(false).
    void Close();

    // 1:1 with legacy CStallKindSelectDlg::OnActionEvent
    // (legacy typo'd as "OnActionEvnet" -- modern port uses
    // the correct spelling).  Dispatches the 3 button ids
    // to the host-injected callbacks, then calls Close().
    // Unknown ids are silently ignored (the legacy `else
    // return;` early-out -- no Close() call for unknown).
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ---- 1:1 id constants (legacy WindowIDs.h) ----
    // Local 430-432 (same as the Tier 2 stub; distinct from
    // 200-420 used by previous Tier 2 dialogs).
    static constexpr std::int32_t kIdSellBtn   = 430;
    static constexpr std::int32_t kIdBuyBtn    = 431;
    static constexpr std::int32_t kIdCancelBtn = 432;

    // 1:1 with legacy cWindowDef.h WE_BTNCLICK (= 0x0001).
    static constexpr std::uint32_t kWeBtnClick = 0x0001u;

    // Test hooks -- inject the 3 cButton pointers
    // (replaces the legacy GetWindowForID lookups).  The
    // host calls Linking + then 3 setters in any order.
    void SetSellBtnForTest(cButton* b)   noexcept { m_pSellBtn   = b; }
    void SetBuyBtnForTest(cButton* b)    noexcept { m_pBuyBtn    = b; }
    void SetCancelBtnForTest(cButton* b) noexcept { m_pCancelBtn = b; }
    cButton* GetSellBtnForTest()   const noexcept { return m_pSellBtn; }
    cButton* GetBuyBtnForTest()    const noexcept { return m_pBuyBtn; }
    cButton* GetCancelBtnForTest() const noexcept { return m_pCancelBtn; }

    // ---- 1:1 callbacks for STREETSTALLMGR singleton ----
    // The legacy invokes 3 STREESTSTALLMGR methods in
    // different combinations across the 3 button branches.
    // Modern port exposes 3 host-injected callbacks so the
    // dispatcher is fully observable from tests.
    using SetStallKindCallback    = void(*)(StallKind kind, void* user);
    using OpenStreetStallCallback = void(*)(void* user);
    using SetOpenMsgBoxCallback   = void(*)(bool open, void* user);
    void SetStallKindCallbackForTest(SetStallKindCallback cb, void* user) {
        m_setStallKindCb = cb; m_setStallKindUser = user;
    }
    void SetOpenStreetStallCallbackForTest(OpenStreetStallCallback cb, void* user) {
        m_openStreetStallCb = cb; m_openStreetStallUser = user;
    }
    void SetOpenMsgBoxCallbackForTest(SetOpenMsgBoxCallback cb, void* user) {
        m_setOpenMsgBoxCb = cb; m_setOpenMsgBoxUser = user;
    }

    // Test accessors.
    bool IsSellDispatched()   const noexcept { return m_sellDispatched; }
    bool IsBuyDispatched()    const noexcept { return m_buyDispatched; }
    bool IsCancelDispatched() const noexcept { return m_cancelDispatched; }
    StallKind LastDispatchKind() const noexcept { return m_lastKind; }

private:
    cButton* m_pSellBtn   = nullptr;
    cButton* m_pBuyBtn    = nullptr;
    cButton* m_pCancelBtn = nullptr;

    SetStallKindCallback    m_setStallKindCb    = nullptr;
    void*                   m_setStallKindUser  = nullptr;
    OpenStreetStallCallback m_openStreetStallCb = nullptr;
    void*                   m_openStreetStallUser = nullptr;
    SetOpenMsgBoxCallback   m_setOpenMsgBoxCb   = nullptr;
    void*                   m_setOpenMsgBoxUser = nullptr;

    // 1:1 quirks: the modern port records which branch
    // fired the dispatcher so tests can verify the legacy
    // STREESTSTALLMGR dispatch chain (SetStallKind ->
    // OpenStreetStall -> SetOpenMsgBox) without inspecting
    // each callback individually.
    bool      m_sellDispatched   = false;
    bool      m_buyDispatched    = false;
    bool      m_cancelDispatched = false;
    StallKind m_lastKind         = StallKind::Null;
};

} // namespace mxh::ui
