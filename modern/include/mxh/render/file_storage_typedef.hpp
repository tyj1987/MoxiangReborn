// mxh/render/file_storage_typedef.hpp
// 1:1 with original 4DyuchiGRX_common/FileStorage_typedef.h.
#pragma once

#include <cstdint>

namespace mxh::gx {

enum FILE_ACCESS_METHOD : std::uint32_t {
    FILE_ACCESS_METHOD_ONLY_FILE    = 0,
    FILE_ACCESS_METHOD_ONLY_PACK    = 1,
    FILE_ACCESS_METHOD_FILE_AND_PACK = 2,
};

enum FSFILE_SEEK : std::uint32_t {
    FSFILE_SEEK_SET = 0,
    FSFILE_SEEK_CUR = 1,
    FSFILE_SEEK_END = 2,
};

// File open mode for FSOpenFile.
enum FSFILE_ACCESSMODE : std::uint32_t {
    FSFILE_ACCESSMODE_BINARY    = 0x00000000,
    FSFILE_ACCESSMODE_TEXT      = 0x00000001,
};

#pragma pack(push, 1)

// 32-byte header for each file entry inside a .pak file.
struct FSFILE_HEADER {
    char     szFileName[24];
    std::uint32_t dwFileNameLen;
    std::uint32_t dwRealFileSize;
};

struct FSFILE_ATOM_INFO {
    char     szFileName[_MAX_PATH];
    std::uint32_t dwRealFileSize;
    std::uint32_t dwFileDataOffset;
};

struct FSPACK_FILE_INFO {
    char     szPackFileName[_MAX_PATH];
    std::uint32_t dwFileNum;
    std::uint32_t dwVersion;
    std::uint32_t dwFlags;
};

#pragma pack(pop)

} // namespace mxh::gx