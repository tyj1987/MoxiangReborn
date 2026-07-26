// mh_error.hpp - Phase D6 MHError 1:1 port.
//
// Source-of-truth: legacy [Server]Map/MHError.h + .cpp.
// Mirrors legacy CMHError.GetStringArg (vsprintf format).  The
// OutputFile method is intentionally not ported: legacy writes to
// "./Log/Debug_<map>_<date>.txt" via the C runtime, which depends
// on a runtime fs layout that modern's framework owns.  Tests can
// assert the formatted output directly via mh_error_format.

#pragma once

#include <cstddef>

namespace mxh::server {

// Format a C-style vararg string into a caller-provided buffer.
// Equivalent to legacy vsprintf behaviour but bounded by cap.
int mh_error_format(char* buffer, std::size_t cap, const char* fmt, ...);

// Return the length of the formatted output without writing.
std::size_t mh_error_format_length(const char* fmt, ...);

}  // namespace mxh::server
