//
// 4DyuchiNET — precompiled header for the legacy IOCP network module.
//
// The legacy vcproj (I4DyuchiNET.vcproj) referenced
// ../4DyuchiNET_Common/StdAfx.h as the PCH root. The original source
// tree did not ship those headers — Phase 7.2 rebuilt them from the
// .cpp/.h call sites in 4DyuchiNET_Latest/.
//
// WinSock 1.x must be suppressed BEFORE <windows.h> because pulling
// <windows.h> first drags in <winsock.h> which collides with
// <winsock2.h>.
//
#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <winsock2.h>
#include <windows.h>
#include <mstcpip.h>     // tcp_keepalive, SIO_KEEPALIVE_VALS

#include <objbase.h>
#include <combaseapi.h>
#include <tchar.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward-only declarations of the COM interfaces — the actual
// definitions come from the headers below. INetwork_GUID.h must come
// before INetwork.h so DEFINE_GUID() expansions resolve before the
// interface declaration references them.
#include "INetwork_GUID.h"
#include "typedef.h"
#include "net_define.h"
#include "icode.h"
#include "INetwork.h"
