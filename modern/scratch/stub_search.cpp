// Stub for the STL vectorized search helper functions used by GoogleTest
// when compiled with /std:c++20. The functions (__std_search_1 / __std_search_2)
// are normally inline in the MSVC STL, but the googletest CMake target
// disables inline expansion (/Ob0) which forces them to be external —
// and the MSVC STL ships no separate lib that defines them.
//
// We provide tiny fallbacks here so the link succeeds.  These are only
// used inside the test binary; production code is unaffected.
#include <cstddef>

extern "C" {

void* __std_search_1(const void* first, std::size_t count, const void* value) {
    const char* p = static_cast<const char*>(first);
    const char* v = static_cast<const char*>(value);
    for (std::size_t i = 0; i < count; ++i) {
        if (p[i] == *v) return const_cast<char*>(p + i);
    }
    return nullptr;
}

void* __std_search_2(const void* first, std::size_t count, const void* value) {
    // Wide-char variant — same shape as __std_search_1.
    const wchar_t* p = static_cast<const wchar_t*>(first);
    const wchar_t v = *static_cast<const wchar_t*>(value);
    for (std::size_t i = 0; i < count; ++i) {
        if (p[i] == v) return const_cast<wchar_t*>(p + i);
    }
    return nullptr;
}

} // extern "C"
