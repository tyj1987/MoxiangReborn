// mxh/client/GameStateStubs.hpp
// Phase A.1.7 — 1:1 port of the 9 eGAMESTATE concrete states from
// 墨香【源码】\[Client]MH\MainGame.h, in the same order.
//
// Each state ships as a thin CGameState subclass for A.1.7 so
// CMainGame's state table is fully populated at boot and the legacy
// state ID values continue to map to the same objects.  The bodies
// are intentionally empty — A.1.8+ replaces them with the real Init /
// Release / Process / network parse logic from the corresponding
// legacy CMainTitle.cpp / CGameIn.cpp / etc.
//
// The class names match the legacy naming (CMainTitle, CGameIn, ...)
// 1:1 so a search-and-replace across the legacy source translates
// directly into modern C++.

#pragma once

#include "CGameState.hpp"
#include "CMainTitle.hpp"

namespace mxh::client {

// -------------------------------------------------------------------------
// CIntroReplay (legacy: CIntroReplayDlg) — Phase A.1.7 stub
// -------------------------------------------------------------------------
class CIntroReplay : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

// -------------------------------------------------------------------------
// CConnecting (legacy: CONNECT — no dedicated class, engine handled
// the Distribute connect dialog inline).  A.1.7 lifts it into its own
// stub so the CMainGame state table matches 1:1 with eGAMESTATE.
// -------------------------------------------------------------------------
class CConnecting : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

// -------------------------------------------------------------------------
// CMainTitle — login screen, server list, agent connect.  A.1.8 ships
// the real implementation in CMainTitle.hpp / .cpp; this header
// is included at the top of GameStateStubs.hpp so the registration
// code in MoxianClient can keep using the same GameStateStubs.hpp
// include.
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// CCharSelect — character select screen.  Lands in A.2+ (Phase B).
// -------------------------------------------------------------------------
class CCharSelect : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

// -------------------------------------------------------------------------
// CCharMake — character creation.  Lands in A.2+.
// -------------------------------------------------------------------------
class CCharMake : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

// -------------------------------------------------------------------------
// CGameLoading — "Loading map..." screen with progress bar.  Lands in
// B.3 (map load).
// -------------------------------------------------------------------------
class CGameLoading : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

// -------------------------------------------------------------------------
// CGameIn — in-game state.  The biggest one (Player, ObjectManager,
// ChatManager, etc.).  Lands in B.4-B.6.
// -------------------------------------------------------------------------
class CGameIn : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

// -------------------------------------------------------------------------
// CMapChange — between-map transition state.  Lands in B.3.
// -------------------------------------------------------------------------
class CMapChange : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

// -------------------------------------------------------------------------
// CMurimNet — MurimNet PvP lobby.  Lands in D.5 (Phase D).
// -------------------------------------------------------------------------
class CMurimNet : public CGameState {
public:
    void Init(void* p) override;
    void Release() override;
    void Process() override;
};

} // namespace mxh::client
