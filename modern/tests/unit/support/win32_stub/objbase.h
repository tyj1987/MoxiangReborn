// Linux test stub for the COM names IFileStorage.hpp includes.
#pragma once

#include "windows.h"

#include <cstring>

using HRESULT = long;
using ULONG = unsigned long;

#ifndef __stdcall
#define __stdcall
#endif
#define STDMETHODIMP HRESULT
#define STDMETHODIMP_(type) type
#ifndef interface
#define interface struct
#endif

struct GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
    bool operator==(const GUID& other) const {
        return Data1 == other.Data1 && Data2 == other.Data2 && Data3 == other.Data3 &&
               std::memcmp(Data4, other.Data4, 8) == 0;
    }
};

using IID = GUID;
using REFIID = const GUID&;

struct IUnknown {
    virtual HRESULT QueryInterface(REFIID, void**) = 0;
    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;
};

#ifndef S_OK
#define S_OK (static_cast<HRESULT>(0))
#endif
#ifndef E_POINTER
#define E_POINTER (static_cast<HRESULT>(0x80004003L))
#endif
#ifndef E_NOINTERFACE
#define E_NOINTERFACE (static_cast<HRESULT>(0x80004002L))
#endif

inline const GUID IID_IUnknown = {0, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
