// mh_error.cpp - Phase D6 MHError 1:1 port implementations.

#include "mxh/server/mh_error.hpp"

#include <cstdarg>
#include <cstdio>

namespace mxh::server {

int mh_error_format(char* buffer, std::size_t cap, const char* fmt, ...) {
    if (buffer == nullptr || fmt == nullptr || cap == 0) return -1;
    std::va_list args;
    va_start(args, fmt);
    // vsnprintf: returns the would-be length (excluding NUL).
    int n = std::vsnprintf(buffer, cap, fmt, args);
    va_end(args);
    return n;
}

std::size_t mh_error_format_length(const char* fmt, ...) {
    if (fmt == nullptr) return 0u;
    std::va_list args1;
    va_start(args1, fmt);
    std::va_list args2;
    va_copy(args2, args1);
    int n = std::vsnprintf(nullptr, 0, fmt, args1);
    va_end(args1);
    va_end(args2);
    return (n < 0) ? 0u : static_cast<std::size_t>(n);
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int mh_error_translation_unit_anchor = 0;
}
