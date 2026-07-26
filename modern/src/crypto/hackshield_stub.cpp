// hackshield_stub.cpp - Software-only HackShield 4.0 stub implementation.
//
// All calls return HS_ERR_OK (0) to keep the modern client boot path
// unblocked. _AhnHS_MakeAckMsg and _AhnHS_MakeGuidAckMsg zero-fill the
// output buffer (legacy behavior when no real driver is loaded).

#include "mxh/crypto/hackshield_stub.hpp"
#include <cstddef>
#include <cstring>

extern "C" {

int __stdcall _AhnHS_Initialize(const char*, PFN_AhnHS_Callback,
                               int, const char*,
                               unsigned int, unsigned int) {
    return 0; // HS_ERR_OK
}

int __stdcall _AhnHS_StartService()  { return 0; }
int __stdcall _AhnHS_StopService()   { return 0; }
int __stdcall _AhnHS_PauseService(unsigned int)  { return 0; }
int __stdcall _AhnHS_ResumeService(unsigned int) { return 0; }
int __stdcall _AhnHS_Uninitialize()  { return 0; }

int __stdcall _AhnHS_MakeAckMsg(unsigned char* pbyReqMsg,
                             unsigned char* pbyAckMsg) {
    if (pbyAckMsg) std::memset(pbyAckMsg, 0, 56); // SIZEOF_ACKMSG
    return 0;
}

int __stdcall _AhnHS_MakeGuidAckMsg(unsigned char* pbyGuidReqMsg,
                                   unsigned char* pbyGuidAckMsg) {
    if (pbyGuidAckMsg) std::memset(pbyGuidAckMsg, 0, 20); // SIZEOF_GUIDACKMSG
    return 0;
}

int __stdcall _AhnHS_SaveFuncAddress(unsigned int, ...) {
    // No-op: modern client does not have an EHsvc.dll to provide
    // function-address CRC computation against.
    return 0;
}

int __stdcall _AhnHS_CheckAPIHooked(const char*, const char*, const char*) {
    return 0; // No APIs hooked (no EHsvc.dll loaded).
}

int __stdcall HS_CallbackProc(long, long, void*) {
    return 0;
}

}  // extern "C"
