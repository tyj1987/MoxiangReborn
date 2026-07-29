// ggsrv25_vendor_stub.cpp - Phase 6.3 nProtect GameGuard v2.5 vendor stub impl.
#include "mxh/crypto/ggsrv25_vendor_stub.hpp"
#include <cstring>
namespace mxh::crypto {
namespace { bool g_ended = false; }
int ggsrv25_init() noexcept { g_ended=false; return 1; }
void ggsrv25_end() noexcept { g_ended=true; }
int ggsrv25_check_auth(const char* s) noexcept {
    if (s==nullptr){ return 0; }
    std::uint32_t n=0; while (s[n] && n<=kGGSrv25AuthMaxLen){ n++; }
    return (n>=kGGSrv25AuthMinLen && n<=kGGSrv25AuthMaxLen) ? 1 : 0;
}
int ggsrv25_get_error_code() noexcept { return static_cast<int>(kGGSrv25ErrorOk); }
bool ggsrv25_is_ended() noexcept { return g_ended; }
}
namespace { [[maybe_unused]] constexpr int ggsrv25_vendor_stub_translation_unit_anchor=0; }
