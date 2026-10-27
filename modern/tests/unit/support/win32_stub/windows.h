// Linux test stub. Only the types filesystem_file_storage.cpp reaches
// through render_typedef.hpp / file_storage_typedef.hpp. Not a Windows SDK.
#pragma once

#include <cstdint>

using BOOL = int;
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

struct RECT {
    long left;
    long top;
    long right;
    long bottom;
};

using INT = int;
using HFONT = void*;
