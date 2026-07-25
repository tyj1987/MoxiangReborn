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
#include "CLoginState.hpp"
#include "CCharSelectState.hpp"
#include "CInGameState.hpp"

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
// CConnecting was the placeholder for eGS_CONNECT (= 2).  Phase B.2.1
// replaces it with CLoginState (see CLoginState.hpp) which drives the
// real login handshake against MoxianLoginServer.  The CConnecting
// class is gone; registrations should use CLoginState.
//
// CCharSelect (eGS_CHARSELECT = 4) is now CCharSelectState (Phase B.2.2,
// see CCharSelectState.hpp) which drives the real character-list +
// character-select handshake against MoxianAgentServer.  The CCharSelect
// stub class is gone.
// -------------------------------------------------------------------------

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
// CGameIn (eGS_GAMEIN = 7) was the placeholder until Phase B.2.3
// replaced it with CInGameState (see CInGameState.hpp) which drives
// the GameIn handshake against MoxianMapServer.  The CGameIn stub
// class is gone.
// -------------------------------------------------------------------------

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
