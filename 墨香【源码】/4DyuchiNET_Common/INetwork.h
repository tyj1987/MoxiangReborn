#pragma once
//
// 4DyuchiNET — COM interface I4DyuchiNET.
//
// Recreated for Phase 7.2 from the I4DyuchiNET_Latest/.cpp call sites,
// notably:
//
//   - conetwork.h / conetwork.cpp — complete method list (4-arg
//     CreateNetwork, 2 send variants, all pause / resume hooks).
//   - factory.h / factory.cpp — IClassFactory implementation needs
//     IID_IUnknown + IID_4DyuchiNET at minimum.
//
// This header is intentionally narrower than
// [CC]ServerModule/inetwork.h — the server-side copy kept a
// simplified 1-arg CreateNetwork that does NOT match Co4DyuchiNET's
// actual implementation. Bug C-19 (Phase 7.2): the [CC]ServerModule
// client expects the legacy 1-arg interface and will fail to link
// against this DLL until that header is updated.
//
// This header is included by conetwork.h DIRECTLY (not via StdAfx.h),
// so it must transitively pull in the WinSDK types it needs:
//   - INetwork_GUID.h for IID_4DyuchiNET
//   - typedef.h for DESC_NETWORK / INET_BUF / PACKET_LIST
//   - <winsock2.h> for sockaddr_in (method signatures use it)
//   - <windows.h> / <objbase.h> for IUnknown and DEFINE_GUID-aware
//     expansion (in case IUnknown is the only routed class).
//
#include "INetwork_GUID.h"
#include "typedef.h"

// icode.h pulls in stdtype + IUnknown.
#include "icode.h"

struct I4DyuchiNET : public IUnknown
{
    // Legacy 4-argument CreateNetwork (desc + accept intervals +
    // initial callback). Used by every server via CoCreateInstance.
    virtual BOOL  __stdcall CreateNetwork(DESC_NETWORK* desc,
                                          DWORD dwUserAcceptInterval,
                                          DWORD dwServerAcceptInterval,
                                          OnIntialFunc pFunc)               = 0;

    virtual void  __stdcall BreakMainThread()                                 = 0;
    virtual void  __stdcall ResumeMainThread()                                = 0;

    virtual void  __stdcall SetUserInfo(DWORD dwConnectionIndex, void* user)  = 0;
    virtual void* __stdcall GetUserInfo(DWORD dwConnectionIndex)              = 0;
    virtual void  __stdcall SetServerInfo(DWORD dwConnectionIndex, void* s)  = 0;
    virtual void* __stdcall GetServerInfo(DWORD dwConnectionIndex)            = 0;

    virtual sockaddr_in* __stdcall GetServerAddress(DWORD dwConnectionIndex) = 0;
    virtual sockaddr_in* __stdcall GetUserAddress(DWORD dwConnectionIndex)   = 0;
    virtual BOOL  __stdcall GetServerAddress(DWORD dwConnectionIndex, char* pIP, WORD* pwPort) = 0;
    virtual BOOL  __stdcall GetUserAddress(DWORD dwConnectionIndex, char* pIP, WORD* pwPort)   = 0;

    virtual BOOL  __stdcall SendToServer(DWORD dwConnectionIndex, char* msg,
                                         DWORD length, DWORD flag)       = 0;
    virtual BOOL  __stdcall SendToUser(DWORD dwConnectionIndex, char* msg,
                                       DWORD length, DWORD flag)         = 0;
    virtual void  __stdcall CompulsiveDisconnectServer(DWORD dwConnectionIndex) = 0;
    virtual void  __stdcall CompulsiveDisconnectUser(DWORD dwConnectionIndex) = 0;

    virtual int   __stdcall GetServerMaxTransferRecvSize()                    = 0;
    virtual int   __stdcall GetServerMaxTransferSendSize()                    = 0;
    virtual int   __stdcall GetUserMaxTransferRecvSize()                      = 0;
    virtual int   __stdcall GetUserMaxTransferSendSize()                      = 0;

    virtual void  __stdcall BroadcastServer(char* pMsg, DWORD len, DWORD flag) = 0;
    virtual void  __stdcall BroadcastUser(char* pMsg, DWORD len, DWORD flag)   = 0;

    virtual DWORD __stdcall GetConnectedServerNum()                           = 0;
    virtual DWORD __stdcall GetConnectedUserNum()                             = 0;
    virtual WORD  __stdcall GetBindedPortServerSide()                         = 0;
    virtual WORD  __stdcall GetBindedPortUserSide()                           = 0;

    virtual BOOL  __stdcall ConnectToServerWithUserSide(char* szIP, WORD port,
                                                        CONNECTSUCCESSFUNC,
                                                        CONNECTFAILFUNC,
                                                        void* pExt)         = 0;
    virtual BOOL  __stdcall ConnectToServerWithServerSide(char* szIP, WORD port,
                                                          CONNECTSUCCESSFUNC,
                                                          CONNECTFAILFUNC,
                                                          void* pExt)       = 0;

    virtual BOOL  __stdcall StartServerWithUserSide(char* ip, WORD port)      = 0;
    virtual BOOL  __stdcall StartServerWithServerSide(char* ip, WORD port)    = 0;

    virtual HANDLE __stdcall GetCustomEventHandle(DWORD index)                = 0;

    // PauseTimer / ResumeTimer — REMOVED 2007/12/19 by yuchi
    // (conetwork.h:51-52 declared them removed). The .cpp impl is still
    // present at conetwork.cpp:416/420 as dead code that doesn't
    // override anything. Leaving them OFF the public I4DyuchiNET
    // surface keeps Co4DyuchiNET non-abstract against the real
    // header. Tracked in KNOWN_BUGS.md Bug C-24.

    // Multi-buffer / packet-list send variants used by the legacy
    // server (HSEL HL send path); see conetwork.cpp:318-330.
    virtual BOOL   __stdcall SendToServer(DWORD dwConnectionIndex, INET_BUF* pBuf, DWORD dwNum, DWORD flag)  = 0;
    virtual BOOL   __stdcall SendToUser(DWORD dwConnectionIndex, INET_BUF* pBuf, DWORD dwNum, DWORD flag)    = 0;
    virtual BOOL   __stdcall SendToServer(DWORD dwConnectionIndex, PACKET_LIST* pList, DWORD flag)          = 0;
    virtual BOOL   __stdcall SendToUser(DWORD dwConnectionIndex, PACKET_LIST* pList, DWORD flag)            = 0;

    virtual BOOL   __stdcall GetMyAddress(char* szOutIP, DWORD dwMaxLen)       = 0;
};
