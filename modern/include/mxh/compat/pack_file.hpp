// PackFile.hpp - Modern C++ reimplementation of 4DyuchiFileStorage (.pak)
//
// Original source: 墨香【源码】\4DyuchiFileStorage\PackFile.cpp + typedef.h
//
// File layout (verified empirically from real .pak files):
//
//   [92 bytes]  PACK_FILE_HEADER
//     - dwVersion (u32) = CURRENT_VERSION = 0x00000001
//     - dwFileItemNum (u32) = number of entries
//     - dwFlag (u32)
//     - dwCRC[4] (u32 × 4)  = all zeros in practice
//     - dwReserved[16] (u32 × 16) = all zeros
//
//   For each file entry (sequential, packed):
//     [32 bytes]   FSFILE_HEADER (8 DWORDs, see struct below)
//     [N bytes]    filename (N = dwFileNameLen)
//     [1 byte]     NUL terminator / padding
//     [M bytes]    file data (M = dwRealFileSize)
//     Total entry size: dwTotalSize = 32 + N + 1 + M
//
//   dwFileDataOffset field is actually the offset of THIS entry's start
//   in the .pak file (despite the misleading name).
//
// FSFILE_HEADER (32 bytes, packed):
//   DWORD dwTotalSize;        // 32 + nameLen + 1 + realFileSize
//   DWORD dwRealFileSize;     // raw data size
//   DWORD dwFileNameLen;      // filename length (no NUL)
//   DWORD dwFileDataOffset;   // position of THIS entry in .pak file
//   DWORD dwFlag1;
//   DWORD dwFlag2;
//   DWORD dwFlag3;
//   DWORD dwFlag4;
//   (followed by szFileName[4] in the C++ struct, but those 4 bytes overlap
//    with the start of the actual filename written after the header)

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::compat {

#pragma pack(push, 1)

// PACK_FILE_HEADER: sizeof = 92 bytes (with reserved[]).
struct PackFileHeader {
    std::uint32_t version;
    std::uint32_t file_item_num;
    std::uint32_t flag;
    std::uint32_t crc[4];
    std::uint32_t reserved[16];
};
static_assert(sizeof(PackFileHeader) == 92, "PackFileHeader must be 92 bytes");

// FSFILE_HEADER: 32 bytes (the "fixed" portion; szFileName[4] follows).
struct PackFileDesc {
    std::uint32_t total_size;
    std::uint32_t real_file_size;
    std::uint32_t file_name_len;
    std::uint32_t file_data_offset;  // actually: entry start position in .pak
    std::uint32_t flag1;
    std::uint32_t flag2;
    std::uint32_t flag3;
    std::uint32_t flag4;
};
static_assert(sizeof(PackFileDesc) == 32, "PackFileDesc must be 32 bytes");

#pragma pack(pop)

struct PackEntry {
    std::string name;                       // e.g. "Map\\Map0.bmhm"
    std::uint32_t entry_offset = 0;         // position of this entry in .pak
    std::uint32_t data_offset = 0;          // position of file data in .pak
    std::uint32_t size = 0;                 // raw data size
    std::span<const std::uint8_t> raw_view; // only valid while PackFile is open
};

class PackFile {
public:
    PackFile() = default;
    ~PackFile();

    PackFile(const PackFile&) = delete;
    PackFile& operator=(const PackFile&) = delete;
    PackFile(PackFile&& other) noexcept;
    PackFile& operator=(PackFile&& other) noexcept;

    [[nodiscard]] static std::unique_ptr<PackFile> open(const std::filesystem::path& path);
    [[nodiscard]] static std::unique_ptr<PackFile> open_buffer(
        std::vector<std::uint8_t> bytes);

    [[nodiscard]] const PackFileHeader& header() const noexcept { return header_; }
    [[nodiscard]] std::uint32_t file_count() const noexcept { return header_.file_item_num; }
    [[nodiscard]] std::size_t parsed_count() const noexcept { return entries_.size(); }

    [[nodiscard]] const PackEntry* find(std::string_view name) const noexcept;
    [[nodiscard]] const std::vector<PackEntry>& entries() const noexcept { return entries_; }

    [[nodiscard]] std::vector<std::uint8_t> read(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> list_names() const;

    [[nodiscard]] static bool is_pak(std::span<const std::uint8_t> bytes) noexcept;

private:
    PackFileHeader header_{};
    std::vector<PackEntry> entries_;
    std::vector<std::uint8_t> raw_;
};

}  // namespace mxh::compat
