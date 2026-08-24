// gtregistdialog.hpp — modern port of 墨香 CGTRegistDialog
// (guild tournament registration dialog: 2 cStatic + 1 cButton).
//
// 1:1 port of legacy `CGTRegistDialog` from
//   `墨香【源码】\[Client]MH\GTRegistDialog.h` (865 B) and
//   `墨香【源码】\[Client]MH\GTRegistDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_GTREGIST_DLG (legacy cWindow
//     type tag).
//   - Dtor: empty body.
//   - Linking: resolve 2 cStatic (m_pRegistGuild by
//     GDT_ENTRY1, m_pRegistableGuild by GDT_ENTRY2)
//     + 1 cButton (m_pRegistBtn by GDT_ENTRYBTN).
//   - SetActive override: call cDialog::SetActive,
//     then if val == FALSE and HERO->GetState() ==
//     eObjectState_Deal → OBJECTSTATEMGR->
//     EndObjectState(HERO, eObjectState_Deal).
//   - TournamentRegistSyn: if HERO->
//     GetGuildMemberRank() != GUILD_MASTER → return
//     eGTError_NOGUILDMASTER; (commented-out level
//     + member checks in legacy); build MSGBASE +
//     set Category = MP_GTOURNAMENT, Protocol =
//     MP_GTOURNAMENT_REGIST_SYN, dwObjectID =
//     HEROID, send via NETWORK; return
//     eGTError_NOERROR.
//   - SetRegistGuildCount(DWORD count): call
//     m_pRegistGuild->SetStaticValue(count) +
//     m_pRegistableGuild->SetStaticValue(
//     MAXGUILD_INTOURNAMENT - count).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type = WT_GTREGIST_DLG
//     drop, modern cWindow does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 3 children by id.
//   - SetActive override: REAL through HERO + OBJECTSTATEMGR
//     host callbacks; base call order matches legacy.
//   - TournamentRegistSyn: REAL through guild-rank, HEROID,
//     and NETWORK callbacks. Commented-out legacy guild-level/member
//     gates remain disabled exactly as in the source.
//   - SetRegistGuildCount: REAL through cStatic::SetStaticValue,
//     including DWORD subtraction and LONG conversion behavior.
//   - eGTError enum: 1:1 with legacy (kErrorNoError = 0,
//     kErrorNoGuildMaster = 1, kErrorUnderLevel =
//     2, kErrorUnderLimitMember = 3,
//     kErrorNotRegistDay = 4, etc.).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 35th **Tier 2** dialog port (after
// cGTRegistcancelDialog). The dialog has no service
// dependency on the modern service interface
// dependency on the modern service interface (Phase 13).
// Legacy globals are supplied through optional host callbacks.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;
class cButton;

class cGTRegistDialog : public cDialog {
public:
    cGTRegistDialog();
    ~cGTRegistDialog() override;

    // ----- 1:1 with legacy CGTRegistDialog::Linking -----

    // 1:1 with legacy Linking. Resolve 2 cStatic
    // (m_pRegistGuild by kIdRegistGuild,
    // m_pRegistableGuild by kIdRegistableGuild) +
    // 1 cButton (m_pRegistBtn by kIdRegistBtn) by id.
    void Linking();

    // ----- 1:1 with legacy CGTRegistDialog::SetActive override -----

    // 1:1 with legacy SetActive override. Call base SetActive;
    // if val == FALSE, then if the hero-state provider reports
    // the hero is currently in eObjectState_Deal (legacy:
    // HERO->GetState() == eObjectState_Deal), the end-deal-state
    // callback is invoked (legacy: OBJECTSTATEMGR->
    // EndObjectState(HERO, eObjectState_Deal)). The host callbacks
    // are OPTIONAL (same pattern as cGTRegistcancelDialog).
    void SetActive(bool val) noexcept override;

    // ----- Host-injected callbacks (legacy: HERO + OBJECTSTATEMGR singletons) -----

    using GetHeroStateFn = std::int32_t (*)(void* userData);
    using EndDealStateFn = void (*)(void* userData);

    void SetCallbacks(GetHeroStateFn getHeroState,
                      EndDealStateFn endDealState,
                      void* userData = nullptr) noexcept;

    using GetGuildMemberRankFn = std::int32_t (*)(void* userData);
    using GetHeroObjectIdFn = std::uint32_t (*)(void* userData);
    using SendTournamentRegistFn = bool (*)(std::uint32_t objectId,
                                            void* userData);

    void SetTournamentCallbacks(GetGuildMemberRankFn getGuildMemberRank,
                                GetHeroObjectIdFn getHeroObjectId,
                                SendTournamentRegistFn sendTournamentRegist,
                                void* userData = nullptr) noexcept;

    // 1:1 with legacy eObjectState_Deal. The host GetHeroStateFn
    // returns this value when the hero is in a deal state.
    static constexpr std::int32_t kObjectStateDeal = 6;

    // ----- 1:1 with legacy CGTRegistDialog::TournamentRegistSyn -----

    // 1:1 with legacy TournamentRegistSyn. Non-master rank
    // returns kErrorNoGuildMaster. Guild masters send the legacy
    // registration MSGBASE through optional host callbacks and return
    // kErrorNoError; the network result is intentionally ignored.
    std::uint32_t TournamentRegistSyn();

    // ----- 1:1 with legacy CGTRegistDialog::SetRegistGuildCount -----

    // 1:1 with legacy SetRegistGuildCount(DWORD count).
    // Writes count and DWORD(32-count) through cStatic::SetStaticValue.
    void SetRegistGuildCount(std::uint32_t count);

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (GDT_ENTRY1 / GDT_ENTRY2 / GDT_ENTRYBTN).
    // Local 470-472 — distinct from 200-460 used
    // by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdRegistGuild      = 470;
    static constexpr std::int32_t kIdRegistableGuild  = 471;
    static constexpr std::int32_t kIdRegistBtn        = 472;

    // ----- 1:1 with legacy eGTError enum -----

    // Tournament registration error codes
    // (1:1 with legacy eGTError_ enum). The modern
    // port inlines the enum values (no shared
    // header dependency) — the values match the
    // legacy common header.
    static constexpr std::uint32_t kErrorNoError          = 0;
    static constexpr std::uint32_t kErrorNoGuildMaster    = 1;
    static constexpr std::uint32_t kErrorUnderLevel       = 2;
    static constexpr std::uint32_t kErrorUnderLimitMember = 3;
    static constexpr std::uint32_t kErrorNotRegistDay     = 4;

    // MAXGUILD_INTOURNAMENT (1:1 with legacy
    // common header constant). Used by
    // SetRegistGuildCount to compute the
    // "registable guild" count.
    static constexpr std::uint32_t kMaxGuildInTournament = 32;
    static constexpr std::int32_t kGuildMasterRank = 50;
    static constexpr std::uint8_t kGTournamentCategory = 60;
    static constexpr std::uint8_t kTournamentRegistProtocol = 1;

private:
    // 1:1 with legacy m_pRegistGuild /
    // m_pRegistableGuild / m_pRegistBtn (resolved
    // in Linking).
    cStatic* m_pRegistGuild     = nullptr;
    cStatic* m_pRegistableGuild = nullptr;
    cButton* m_pRegistBtn       = nullptr;

    // Host-injected callbacks (replaces HERO + OBJECTSTATEMGR
    // singletons). A null pointer preserves the safe no-op
    // behavior so the dialog can be exercised in tests without
    // wiring the full host singletons.
    GetHeroStateFn         m_getHeroStateFn = nullptr;
    EndDealStateFn         m_endDealStateFn = nullptr;
    void*                  m_callbackUserData = nullptr;
    GetGuildMemberRankFn   m_getGuildMemberRankFn = nullptr;
    GetHeroObjectIdFn      m_getHeroObjectIdFn = nullptr;
    SendTournamentRegistFn m_sendTournamentRegistFn = nullptr;
    void*                  m_tournamentUserData = nullptr;
};

}  // namespace mxh::ui
