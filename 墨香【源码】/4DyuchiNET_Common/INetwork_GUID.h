#pragma once
//
// 4DyuchiNET COM GUID definitions. Recreated for Phase 7.2 — values
// copied verbatim from [CC]ServerModule/network_guid.h (which was
// forked from the original 4DyuchiNET_Common/INetwork_GUID.h).
//
// {11C02A88-8BF9-4863-A7DE-1BF66D60CBA3}
// {D41BD0F8-07BF-4dbc-8AD3-A3524CC43E5E}
//
// This header is included by conetwork.h DIRECTLY (not via StdAfx.h),
// so it must transitively pull in the WinSDK types it needs:
//   - <winsock2.h> BEFORE <windows.h>: without the WinSock 1.x guard
//     <windows.h> pulls <winsock.h> which collides with later
//     <winsock2.h> (a typedef redefinition cascade).
//   - <objbase.h> for DEFINE_GUID()'s expansion.
// Without these, every conetwork.cpp compile hits "CLSID_4DyuchiNET:
// undeclared identifier". Tracked in PHASE7_MIGRATION_RECIPE.md
// convention #2 (winsock1/2 ordering).
//
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_           // suppress WinSock 1.x pulled by windows.h
#endif
#include <winsock2.h>
#include <windows.h>
#include <objbase.h>

#ifndef INCLUDE_GUARD_NETWORK_GUID
#define INCLUDE_GUARD_NETWORK_GUID

#include <initguid.h>

DEFINE_GUID(CLSID_4DyuchiNET,
    0x11c02a88, 0x8bf9, 0x4863, 0xa7, 0xde, 0x1b, 0xf6, 0x6d, 0x60, 0xcb, 0xa3);

DEFINE_GUID(IID_4DyuchiNET,
    0xd41bd0f8, 0x07bf, 0x4dbc, 0x8a, 0xd3, 0xa3, 0x52, 0x4c, 0xc4, 0x3e, 0x5e);

#endif
