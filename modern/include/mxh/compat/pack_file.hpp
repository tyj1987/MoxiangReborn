// PackFile.hpp - Modern C++ reimplementation of 4DyuchiFileStorage (.pak)
//
// Original source: 墨香【源码】\4DyuchiFileStorage\PackFile.cpp + typedef.h
//
// Two headers, selected by dwFileItemNum at offset 4. See docs/formats/pak.md.
//
//   Variant A (worktree, offset 4 != 0):
//     [92 bytes] PACK_FILE_HEADER, then entries at offset 92.
//     The loop advances by the on-disk layout, not by dwTotalSize.
//
//   Variant B (original client, offset 4 == 0):
//     [96 bytes] header. The entry count is the uint32 at offset 92
//     (original Map.pak: 0x2C03 = 11267). Entries start at offset 96
//     and the next entry is at cur + dwTotalSize. A short or inconsistent
//     header fails closed.
//
//   [92 bytes]  PACK_FILE_HEADER
//     - dwVersion (u32) = CURRENT_VERSION = 0x00000001
//     - dwFileItemNum (u32) = number of entries, or 0 for variant B
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
// Variant B stores its entry count in the next uint32 (file offset 92),
// outside this struct. sizeof stays 92.
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

    // `error`, when non-null, is cleared on success and set on failure.
    // Existing callers omit it. Failure is still a null return.
    [[nodiscard]] static std::unique_ptr<PackFile> open(
        const std::filesystem::path& path, std::string* error = nullptr);
    [[nodiscard]] static std::unique_ptr<PackFile> open_buffer(
        std::vector<std::uint8_t> bytes, std::string* error = nullptr);

    [[nodiscard]] const PackFileHeader& header() const noexcept { return header_; }
    // Variant A: dwFileItemNum at offset 4. Variant B (that field is 0):
    // the number of entries actually parsed from the count at offset 92.
    [[nodiscard]] std::uint32_t file_count() const noexcept {
        if (header_.file_item_num == 0) {
            return static_cast<std::uint32_t>(entries_.size());
        }
        return header_.file_item_num;
    }
    [[nodiscard]] std::size_t parsed_count() const noexcept { return entries_.size(); }

    [[nodiscard]] const PackEntry* find(std::string_view name) const noexcept;
    [[nodiscard]] const std::vector<PackEntry>& entries() const noexcept { return entries_; }

    [[nodiscard]] std::vector<std::uint8_t> read(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> list_names() const;

    [[nodiscard]] static bool is_pak(std::span<const std::uint8_t> bytes) noexcept;

private:
    // Offset 4 is 0. Count is at offset 92, entries start at 96.
    // Returns null instead of a partial pack.
    [[nodiscard]] static std::unique_ptr<PackFile> open_original_96(
        std::vector<std::uint8_t> bytes, std::string* error);

    PackFileHeader header_{};
    std::vector<PackEntry> entries_;
    std::vector<std::uint8_t> raw_;
};

}  // namespace mxh::compat
