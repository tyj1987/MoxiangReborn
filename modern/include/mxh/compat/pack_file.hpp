// PackFile.hpp - Modern C++ reimplementation of 4DyuchiFileStorage (.pak)
//
// Original source: 墨香【源码】\4DyuchiFileStorage\PackFile.cpp + typedef.h
//
// Two headers. Both layouts are probed on every open. A layout is used
// only when its entry walk ends at exact EOF and the other layout does
// not. If neither walk validates, or both do, the pak is not loaded.
// The probe and the open walk seek to entry headers and filenames.
// Payloads stay on disk until read() / read_at(). See docs/formats/pak.md.
//
//   Variant A (worktree / PlayDH, 92-byte header):
//     count = uint32 at offset 4, entries at offset 92.
//     The loop advances by the on-disk layout, not by dwTotalSize.
//     A count of 0 is not a validating walk.
//
//   Variant B (original client, 96-byte header):
//     count = uint32 at offset 92 (original Map.pak: 0x2C03 = 11267).
//     Entries start at offset 96 and the next entry is at cur + dwTotalSize.
//     total must equal 32+name_len+1+real, a NUL follows the name, and the
//     walk must end at EOF. A count of 0 is not a validating walk.
//     A nonzero uint32 at offset 4 does not by itself select variant A.
//
// When the uint32 at offset 4 is 0 and neither walk validates, open still
// runs the original-96 parser so the existing truncated, zero-count, and
// overrun errors stay.
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

struct PakFileBytes;

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

// Result of trying both entry walks. Exactly one true flag is a usable
// header. Both true, or both false, means the pak is not loaded.
struct PakLayoutProbe {
    bool playdh92_exact_eof = false;
    bool original96_exact_eof = false;
};

// PlayDH92: uint32 at offset 4, layout walk from offset 92.
// Original96: uint32 at offset 92, strict walk from offset 96.
// A count of 0 does not validate. Version must be 1.
// The span overload only reads entry headers (and the original-96 NUL).
// It does not require the caller to have skipped payloads itself.
[[nodiscard]] PakLayoutProbe probe_pak_layouts(
    std::span<const std::uint8_t> bytes) noexcept;

// Same decision as probe_pak_layouts, reading the file by seek.
// Payloads are not read. `opened == false` means the file could not be
// opened. `read_error` means a later header read failed.
struct PakFileProbe {
    PakLayoutProbe layout;
    bool opened = false;
    bool read_error = false;
    std::string error;
    std::uint64_t file_size = 0;
    std::uint32_t count_at_offset_4 = 0;
    std::uint32_t count_at_offset_92 = 0;
    bool has_count_at_offset_92 = false;
};

[[nodiscard]] PakFileProbe probe_pak_file(const std::filesystem::path& path);

struct PackEntry {
    std::string name;                       // e.g. "Map\\Map0.bmhm"
    std::uint32_t entry_offset = 0;         // position of this entry in .pak
    std::uint32_t data_offset = 0;          // position of file data in .pak
    std::uint32_t size = 0;                 // raw data size
    // open_buffer() aliases bytes owned by that PackFile.
    // open(path) does not retain payloads, so this span stays empty.
    // Non-empty spans are valid only while that PackFile is alive.
    std::span<const std::uint8_t> raw_view;
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
    // PlayDH92: dwFileItemNum at offset 4. A successful original-96 open
    // reports the number of entries parsed from the count at offset 92,
    // including when the on-disk uint32 at offset 4 is a nonzero decoy.
    [[nodiscard]] std::uint32_t file_count() const noexcept {
        if (report_parsed_count_ || header_.file_item_num == 0) {
            return static_cast<std::uint32_t>(entries_.size());
        }
        return header_.file_item_num;
    }
    [[nodiscard]] std::size_t parsed_count() const noexcept { return entries_.size(); }

    [[nodiscard]] const PackEntry* find(std::string_view name) const noexcept;
    [[nodiscard]] const std::vector<PackEntry>& entries() const noexcept { return entries_; }

    [[nodiscard]] std::vector<std::uint8_t> read(std::string_view name) const;
    // True when the entry's payload range sits inside the archive and
    // size > 0. This is the file-backed replacement for a non-empty raw_view.
    [[nodiscard]] bool has_payload(const PackEntry& entry) const noexcept;
    // Positional read of exactly `length` bytes at `offset`.
    // Concurrent callers do not share a file pointer.
    // `length == 0` succeeds. A range past EOF or a short read returns false.
    [[nodiscard]] bool read_at(std::uint64_t offset, void* dst, std::size_t length) const;
    [[nodiscard]] std::vector<std::string> list_names() const;

    [[nodiscard]] static bool is_pak(std::span<const std::uint8_t> bytes) noexcept;

private:
    PackFileHeader header_{};
    std::vector<PackEntry> entries_;
    // Whole-archive bytes only when the caller passed them to open_buffer.
    // open(path) leaves this empty and reads through PakFileBytes instead.
    std::vector<std::uint8_t> buffer_;
    std::unique_ptr<PakFileBytes> file_;
    std::uint64_t source_size_ = 0;
    bool from_buffer_ = false;
    // Set on a successful original-96 open so file_count() ignores a decoy
    // uint32 at offset 4. header().file_item_num stays the on-disk value.
    bool report_parsed_count_ = false;
};

}  // namespace mxh::compat
