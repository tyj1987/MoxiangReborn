// survivalcountdialog.hpp — modern port of 墨香
// CSurvivalCountDialog (survival-mode alive
// counter + winner name dialog: 2 cStatic).
//
// 1:1 port of legacy `CSurvivalCountDialog` from
//   `墨香【源码】\[Client]MH\SurvivalCountDialog.h`
//   and `墨香【源码】\[Client]MH\SurvivalCountDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_pCounterNum = NULL; m_pWinnerName = NULL.
//     (1:1 quirk: ctor body has commented-out
//     m_pCounterNum[FrontNum/BackNum] = NULL lines
//     for the legacy 2-cStatic array that's been
//     collapsed to a single cStatic in the modern
//     port).
//   - Dtor: empty body.
//   - Linking: resolve 1 cStatic (m_pCounterNum by
//     SVV_ALIVECOUNTER) + 1 cStatic (m_pWinnerName
//     by SVV_WINNERNAME). SetCounterNumber(0) +
//     m_pWinnerName->SetStaticText(CHATMGR->GetChatMsg
//     (484)).
//   - InitSurvivalCountDlg(MAPTYPE MapNum): if
//     MAP->IsMapKind(eSurvivalMap) → SetActive(TRUE);
//     else SetActive(FALSE).
//   - SetCounterNumber(DWORD num): clamp to 99;
//     sprintf "%d%d" with num/10 + num%10 →
//     m_pCounterNum->SetStaticText.
//   - SetWinnerName(char* pName): if pName
//     m_pWinnerName->SetStaticText(pName);
//     else m_pWinnerName->SetStaticText(CHATMGR
//     ->GetChatMsg(484)).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_pCounterNum /
//     m_pWinnerName null-init via default member
//     init).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 2 children by id +
//     SetCounterNumber(0) +
//     m_pWinnerName->SetScriptText placeholder for
//     CHATMGR msg 484 (but cStatic has SetStaticText
//     not SetScriptText, so we use SetStaticText
//     with placeholder).
//   - InitSurvivalCountDlg: TODO (MAP singleton
//     + MAPTYPE not ported, R-12.x deferred).
//     Modern port always SetActive(false) for now.
//   - SetCounterNumber: REAL — clamp to 99 +
//     sprintf "%d%d" with c2 = num/10 + c1 = num%10
//     + SetStaticText.
//   - SetWinnerName: REAL — defensive null check +
//     SetStaticText with kSurvivalDefaultName
//     placeholder for CHATMGR msg 484 fallback.
//   - 1:1 quirk: legacy 2-cStatic array
//     (m_pCounterNum[2]) collapsed to single
//     cStatic in modern port (1:1 with legacy
//     active 1-cStatic implementation; commented-out
//     2-array code is documented in modern header).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 53rd **Tier 2** dialog port (after
// cPointSaveDialog). The dialog has 2 cStatic
// (m_pCounterNum + m_pWinnerName). MAP + MAPTYPE
// are R-12.x deferred.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;

class cSurvivalCountDialog : public cDialog {
public:
    cSurvivalCountDialog();
    ~cSurvivalCountDialog() override;

    // ----- 1:1 with legacy CSurvivalCountDialog::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cStatic
    // (m_pCounterNum by kIdAliveCounter) + 1 cStatic
    // (m_pWinnerName by kIdWinnerName). Call
    // SetCounterNumber(0) + SetStaticText on
    // m_pWinnerName with kSurvivalDefaultName
    // placeholder for CHATMGR msg 484.
    void Linking();

    // ----- 1:1 with legacy CSurvivalCountDialog::InitSurvivalCountDlg -----

    // 1:1 with legacy InitSurvivalCountDlg(MAPTYPE).
    // The MAP singleton + MAPTYPE / eSurvivalMap
    // dispatch is TODO (R-12.x deferred). Modern
    // port always SetActive(false) for now.
    void InitSurvivalCountDlg(int mapNum);

    // ----- 1:1 with legacy CSurvivalCountDialog::SetCounterNumber -----

    // 1:1 with legacy SetCounterNumber(DWORD num).
    // Clamp to 99 + sprintf "%d%d" with c2=num/10 +
    // c1=num%10 + SetStaticText.
    void SetCounterNumber(std::uint32_t num);

    // ----- 1:1 with legacy CSurvivalCountDialog::SetWinnerName -----

    // 1:1 with legacy SetWinnerName(char* pName).
    // Defensive null check + SetStaticText. Fallback
    // to kSurvivalDefaultName placeholder for
    // CHATMGR msg 484.
    void SetWinnerName(const char* pName);

    // ----- 1:1 with legacy state accessors -----

    // 1:1 with legacy m_pCounterNum getter (1:1 with
    // legacy single-cStatic implementation).
    cStatic* GetCounterNum() const noexcept {
        return m_pCounterNum;
    }

    // 1:1 with legacy m_pWinnerName getter.
    cStatic* GetWinnerName() const noexcept {
        return m_pWinnerName;
    }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h SVV_ALIVECOUNTER
    // / SVV_WINNERNAME. Local 720-721.
    static constexpr std::int32_t kIdAliveCounter = 720;
    static constexpr std::int32_t kIdWinnerName   = 721;

    // 1:1 with legacy CHATMGR->GetChatMsg(484) for
    // default winner name. Modern port uses literal
    // placeholder until CHATMGR is ported.
    static constexpr const char* kSurvivalDefaultName =
        "SURVIVAL_DEFAULT_NAME";  // CHATMGR msg 484

    // 1:1 with legacy SetCounterNumber clamp.
    static constexpr std::uint32_t kMaxCounterNumber = 99;

private:
    // 1:1 with legacy m_pCounterNum (resolved in
    // Linking by SVV_ALIVECOUNTER id). The legacy
    // 2-cStatic array (m_pCounterNum[2]) is
    // collapsed to a single cStatic in modern port
    // (1:1 with legacy active 1-cStatic
    // implementation).
    cStatic* m_pCounterNum = nullptr;

    // 1:1 with legacy m_pWinnerName (resolved in
    // Linking by SVV_WINNERNAME id).
    cStatic* m_pWinnerName = nullptr;
};

}  // namespace mxh::ui
