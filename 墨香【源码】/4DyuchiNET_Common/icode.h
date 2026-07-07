#pragma once
//
// 4DyuchiNET — ICode interface used by the connection send path
// when FLAG_SEND_ENCRYPTION is set. The implementation lives in the
// server-side HSEL-based DLL; this transport DLL only references the
// interface through a global pointer (g_pICode in net_global.cpp).
//

struct ICode : public IUnknown
{
    virtual BOOL __stdcall Init(BYTE /*kind*/, void* /*ext*/)                = 0;
    virtual BOOL __stdcall Encode(char* pOut, DWORD* pnOutLen,
                                  char* pIn, DWORD nInLen)                   = 0;
    virtual BOOL __stdcall Decode(char* pOut, DWORD* pnOutLen,
                                  char* pIn, DWORD nInLen)                   = 0;
    virtual BYTE __stdcall GetEnCRCConvertChar()                              = 0;
    virtual BYTE __stdcall GetDeCRCConvertChar()                              = 0;
};
