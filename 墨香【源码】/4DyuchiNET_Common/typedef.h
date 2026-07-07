#pragma once
//
// 4DyuchiNET — shared callback typedefs, packet structures and
// DESCRIPTION_NETWORK layout. Recreated for Phase 7.2 because the
// legacy 4DyuchiNET_Common directory is missing from the source
// tree.
//
// Values match [CC]ServerModule/typedef.h where the same names are
// also exported; the legacy game sever side was copied from the
// original 4DyuchiNET_Common tree, so the wire layout is identical.
//
// Note: Net_Common headers do NOT route every TU through StdAfx.h.
// connect_que.h, connection.h, pre_connect.h, network.h, etc.
// include this file directly, so it must transitively surface the
// Winsock 2 types (SOCKET, sockaddr_in, SOCKADDR_IN, SOCKADDR)
// used by CONNECT_ITEM, CConnection, INET_BUF etc. Without this,
// the headers compile but the .cpp files see "socket: not a member
// of CONNECT_ITEM" because the type never became visible.
//
#include <winsock2.h>

typedef void (*ACCEPTFUNC)(DWORD);
typedef void (*RECVFUNC)(DWORD, char*, DWORD);
typedef void (*DISCONNECTFUNC)(DWORD);
typedef void (*CONNECTSUCCESSFUNC)(DWORD, void*);
typedef void (*CONNECTFAILFUNC)(void*);
typedef void (*VOIDFUNC)(void);

// User-defined event callback fired from the main thread (legacy
// "by chan78 at 2001/10/17"). mainthread.cpp:234 invokes with one
// DWORD argument, so the type must accept (DWORD).
typedef void (*EVENTCALLBACK)(DWORD);

// Called once when the main network thread starts.
// The legacy mainthread.cpp invokes it as `g_pOnInitialFunc(NULL);`,
// so the signature is (void* pExt) — pExt is forwarded from
// Co4DyuchiNET::CreateNetwork(...).
typedef void (*OnIntialFunc)(void* pExt);


// PPVOID (void**) — used by STDAPI QueryInterface / CreateInstance
// in factory.cpp / dllmain.cpp / conetwork.cpp. The WinSDK 2003 era
// declared it via <wtypes.h>; modern SDK no longer surfaces the name.
// Tracked in KNOWN_BUGS.md Bug C-25.
typedef void** PPVOID;


// TCHAR buffer size constants used by dllmain.cpp's DllRegisterServer /
// DllUnregisterServer. Originally lived in typedef.h (legacy).
//   MAX_STRING_LENGTH — sized to hold the longest CLSID path key,
//                        e.g. "CLSID\\{11C02A88-...-CBA3}".
//   GUID_SIZE         — `TCHAR szID[GUID_SIZE+1]` buffer for
//                        StringFromGUID2 output (40 incl. NUL).
// NETDDSC_DEBUG_LOG_MASK — bit mask mirror of NETDDSC_DEBUG_LOG,
//                          matches legacy conetwork.cpp:185
//                          `if ((desc->dwFlag & NETDDSC_DEBUG_LOG_MASK) == NETDDSC_DEBUG_LOG)`.
#define MAX_STRING_LENGTH         256
#define GUID_SIZE                 39
#define NETDDSC_DEBUG_LOG_MASK    0x00000010


// Send flags (also used as the Send() "flag" parameter on CConnection /
// CNetwork).
// ddesc_flag mask bits for DESC_NETWORK::dwFlag.
#define NETDDSC_ENCRYPTION         0x00000001
#define NETDDSC_ENCRYPTION_MASK    0x00000001
#define NETDDSC_DEBUG_LOG          0x00000010

// Send flags (also used as the Send() "flag" parameter on CConnection /
// CNetwork).
enum FLAG_SEND
{
    FLAG_SEND_ENCRYPTION     = 0x00000001,
    FLAG_SEND_NOT_ENCRYPTION = 0x00000000,
};


struct CUSTOM_EVENT
{
    DWORD          dwPeriodicTime;   // ms
    EVENTCALLBACK  pEventFunc;       // called from the main thread
};


struct DESC_NETWORK
{
    DWORD    dwMaxUserNum;
    DWORD    dwMaxServerNum;

    void     (*OnRecvFromUserTCP)(DWORD dwConnectionIndex, char* pMsg, DWORD dwLength);
    void     (*OnRecvFromServerTCP)(DWORD dwConnectionIndex, char* pMsg, DWORD dwLength);

    void     (*OnAcceptUser)(DWORD dwConnectionIndex);
    void     (*OnAcceptServer)(DWORD dwConnectionIndex);

    void     (*OnDisconnectUser)(DWORD dwConnectionIndex);
    void     (*OnDisconnectServer)(DWORD dwConnectionIndex);

    DWORD    dwServerMaxTransferSize;
    DWORD    dwUserMaxTransferSize;
    DWORD    dwServerBufferSizePerConnection;
    DWORD    dwUserBufferSizePerConnection;
    DWORD    dwMainMsgQueMaxBufferSize;
    DWORD    dwConnectNumAtSameTime;
    DWORD    dwFlag;
    DWORD    dwCustomDefineEventNum;

    CUSTOM_EVENT* pEvent;
};


// Multi-buffer send variant.
struct INET_BUF
{
    char* pBuf;
    DWORD dwLen;
};

// Linked-list of zero-copy packet headers. CConnection::Send walks
// the list, PushMsg-queues every segment, and issues one WSASend.
struct PACKET_LIST
{
    DWORD        dwLen;
    char*        pMsg;
    PACKET_LIST* pNext;
};
