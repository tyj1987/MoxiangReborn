// MhFileEx.hpp - Modern C++ reimplementation of MHFileEx (.bin format)
//
// Source of truth: 墨香【源码】\[Tool]PackingMan\MHFileEx.cpp
// Original algorithm (verbatim from old source):
//   For .bin files:
//     - Header: MHFILE_HEADER { DWORD version, type, size }
//     - Optional CRC bytes (1-2)
//     - Data of `size` bytes, decrypted by: data[i] -= (char)i
//     - Trailing CRC byte (sometimes)
//
// This file provides:
//   - read_bin(): decode .bin from disk into memory
//   - write_bin(): encode memory back to .bin (for tools that modify resources)
//   - is_bin(): sniff first 12 bytes to detect format

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

// Mirrors old MHFILE_HEADER struct (12 bytes, packed).
#pragma pack(push, 1)
struct MhFileHeader {
    std::uint32_t version;   // 0x00000001 for classic .bin
    std::uint32_t type;      // 0=normal, others = region-specific XOR variant
    std::uint32_t file_size; // payload size in bytes (post-decryption)
};
static_assert(sizeof(MhFileHeader) == 12, "MhFileHeader must be 12 bytes (1:1 with old code)");

// Some variants (DOF) use a slightly different header.
struct MhFileHeaderDof {
    std::uint32_t version;
    std::uint32_t type;
    std::uint32_t file_size;
    std::uint32_t unknown;  // seen in some region-specific variants
};
#pragma pack(pop)

// Error code returned by all operations (no exceptions).
enum class MhError {
    Ok = 0,
    FileNotFound,
    PermissionDenied,
    IoError,
    InvalidHeader,
    UnsupportedVersion,
    CrcMismatch,
};

// Decode a .bin file from disk into a freshly allocated byte buffer.
// The returned vector contains the decrypted (raw) payload bytes.
struct MhFile {
    MhFileHeader header;
    std::vector<std::uint8_t> data;  // decrypted payload
    std::uint32_t crc1 = 0;          // stored CRC (informational)
    std::uint32_t crc2 = 0;
};

// Result<T> for value-or-error pattern (compatible with old-style API).
template <typename T>
struct Result {
    T value;
    MhError error = MhError::Ok;

    [[nodiscard]] bool ok() const noexcept { return error == MhError::Ok; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok(); }
};

// Sniff first 12 bytes: returns true if it looks like a .bin file.
// Heuristic: version is 1 (most common), type is one of {0,1,2,3,4}, file_size is plausible.
[[nodiscard]] bool is_mh_bin(std::span<const std::uint8_t> bytes) noexcept;

// Load a .bin file from disk, performing XOR decryption.
[[nodiscard]] Result<MhFile> read_mh_bin(const std::filesystem::path& path);

// Save raw bytes to .bin file (with the XOR encryption applied).
[[nodiscard]] MhError write_mh_bin(const std::filesystem::path& path,
                                   std::span<const std::uint8_t> data,
                                   std::uint32_t type = 0);

// In-memory decryption (used by read_mh_bin).
// XOR algorithm: for i in [0, size): data[i] -= (byte)i
// If type == 1, also: if (i % type == 0) data[i] -= type
// (Old code path, region-dependent.)
[[nodiscard]] std::vector<std::uint8_t> decrypt_bin_payload(
    std::span<const std::uint8_t> encrypted,
    std::uint32_t type) noexcept;

// Inverse of decrypt_bin_payload.
[[nodiscard]] std::vector<std::uint8_t> encrypt_bin_payload(
    std::span<const std::uint8_t> raw,
    std::uint32_t type) noexcept;

// CRC-8 (sum mod 256) used by old code (CRC is mostly commented out in original,
// but kept here for round-trip correctness).
[[nodiscard]] std::uint8_t compute_crc8(std::span<const std::uint8_t> bytes) noexcept;

}  // namespace mxh::compat