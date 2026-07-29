#pragma once
#include <cstdint>
namespace mxh::crypto {
// ggsrv25_vendor_stub.hpp - Phase 6.3 nProtect GameGuard v2.5 vendor stub.
//
// Source-of-truth: legacy [Server]Agent/ggsrv25.h + ggsrv25_vendor_stub.cpp.
// GameGuard v2.5 is a vendor SDK that authenticates game clients. Legacy
// ggsrv25_vendor_stub.cpp is the offline-replaceable stub that the server
// links against when the vendor SDK is not present (e.g., dev/CI builds).
//
// Locked invariants (1:1 with legacy):
//   - ggsrv25_init() returns 1 (success) on every call. The vendor stub never
//     fails initialization; legacy callers treat 1 as 'OK'.
//   - ggsrv25_end() is idempotent. Calling it twice is a no-op (matches
//     legacy GGSRVRundown).
//   - ggsrv25_check_auth() returns 1 if the supplied hex string is non-empty
//     (legacy check: at least one hex nibble submitted) and 0 otherwise.
//   - ggsrv25_get_error_code() always returns 0 in the vendor stub. Real
//     vendor SDK returns nProtect-specific error codes; the stub is offline.
//   - kGGSrv25AuthMinLen = 16 hex characters (legacy minimum auth packet).
inline constexpr std::uint32_t kGGSrv25AuthMinLen = 16u;
inline constexpr std::uint32_t kGGSrv25AuthMaxLen = 64u;
inline constexpr std::uint32_t kGGSrv25ErrorOk = 0u;
// Initialize the vendor stub. Always succeeds.
int ggsrv25_init() noexcept;
// Rundown. Idempotent.
void ggsrv25_end() noexcept;
// Validate an auth packet. Returns 1 if hex_string length is within
// [kGGSrv25AuthMinLen, kGGSrv25AuthMaxLen], 0 otherwise.
int ggsrv25_check_auth(const char* hex_string) noexcept;
// Last error code (always 0 in stub).
int ggsrv25_get_error_code() noexcept;
// True when ggsrv25_end() has been called (mirrors legacy m_bEnd flag).
bool ggsrv25_is_ended() noexcept;
}
