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
//   - SetActive override: 1:1 with legacy. The
//     HERO + OBJECTSTATEMGR dispatch is TODO
//     (R-12.x deferred). The base SetActive is
//     always called (matches legacy call order).
//   - TournamentRegistSyn: TODO (3-singleton: HERO
//     + GUILDMGR + NETWORK not ported, R-12.x
//     deferred). The modern port returns
//     kErrorNoGuildMaster (matching the legacy
//     early-return path for non-master). When
//     ported, the body becomes the legacy code.
//   - SetRegistGuildCount: TODO (cStatic::SetStaticValue
//     not yet ported, R-12.x deferred). Modern
//     port is a no-op. When ported, the body
//     becomes the legacy code with kMaxGuildInTournament
//     - count.
//   - eGTError enum: 1:1 with legacy (kErrorNoError = 0,
//     kErrorNoGuildMaster = 1, kErrorUnderLevel =
//     2, kErrorUnderLimitMember = 3,
//     kErrorNotRegistDay = 4, etc.).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 35th **Tier 2** dialog port (after
// cGTRegistcancelDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only HERO + OBJECTSTATEMGR + GUILDMGR
// + NETWORK singletons (R-12.x deferred).

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

    // 1:1 with legacy SetActive override. Call
    // base SetActive; if val == FALSE, the
    // HERO + OBJECTSTATEMGR dispatch is TODO
    // (R-12.x deferred). The base SetActive is
    // always called.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CGTRegistDialog::TournamentRegistSyn -----

    // 1:1 with legacy TournamentRegistSyn. The
    // whole method is TODO (3-singleton: HERO +
    // GUILDMGR + NETWORK not ported, R-12.x
    // deferred). The modern port returns
    // kErrorNoGuildMaster (matching the legacy
    // early-return path for non-master). When
    // ported, the body becomes the legacy code.
    std::uint32_t TournamentRegistSyn();

    // ----- 1:1 with legacy CGTRegistDialog::SetRegistGuildCount -----

    // 1:1 with legacy SetRegistGuildCount(DWORD
    // count). The whole method is TODO (cStatic::
    // SetStaticValue not yet ported, R-12.x
    // deferred). Modern port is a no-op. When
    // ported, the body becomes the legacy code.
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

private:
    // 1:1 with legacy m_pRegistGuild /
    // m_pRegistableGuild / m_pRegistBtn (resolved
    // in Linking).
    cStatic* m_pRegistGuild     = nullptr;
    cStatic* m_pRegistableGuild = nullptr;
    cButton* m_pRegistBtn       = nullptr;
};

}  // namespace mxh::ui
