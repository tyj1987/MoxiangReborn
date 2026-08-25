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
#include <string_view>
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
    // The current PlayDH server profile uses a size-prefixed opaque container
    // which is not the classic MHFileEx wire format.  Keep it explicit so a
    // caller cannot mistake an undecoded payload for valid game data.
    UnsupportedOpaqueServerProfile,
};

// Detect the current PlayDH server container without attempting to decode it.
// Layout: [uint32 total_file_size][opaque payload].  This is deliberately
// separate from the classic reader until its transform is recovered exactly.
[[nodiscard]] bool is_size_prefixed_opaque_server_profile(
    std::span<const std::uint8_t> bytes) noexcept;

// Decode a .bin file from disk into a freshly allocated byte buffer.
// The returned vector contains the decrypted (raw) payload bytes.
struct MhFile {
    MhFileHeader header;
    std::vector<std::uint8_t> data;  // decrypted payload
    std::uint32_t crc1 = 0;          // stored CRC (informational)
    std::uint32_t crc2 = 0;
};

// Raw container for the current PlayDH server variant.  This API deliberately
// performs no transform: it exposes the bytes after the size prefix so a
// future decoder can be implemented and verified independently from the
// classic MHFileEx reader.  It must never be passed to gameplay parsers as
// decoded text.
struct OpaqueServerContainer {
    std::uint32_t total_size = 0;
    std::vector<std::uint8_t> payload;
};

// Result<T> for value-or-error pattern (compatible with old-style API).
template <typename T>
struct Result {
    T value;
    MhError error = MhError::Ok;

    [[nodiscard]] bool ok() const noexcept { return error == MhError::Ok; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok(); }
};

// Decode the current server-size-prefixed-opaque-v1 container. The
// container has a 20-byte profile header followed by an AIGroup text body
// encrypted with an eight-byte repeating XOR key. The key is recovered from
// the format's mandatory "$Group 1" preamble and the decoded body is
// structurally validated before it is exposed to gameplay parsers.
[[nodiscard]] Result<std::vector<std::uint8_t>> decode_opaque_server_payload(
    std::span<const std::uint8_t> payload) noexcept;

// Sniff first 12 bytes: returns true if it looks like a .bin file.
// Heuristic: version is 1 (most common), type is one of {0,1,2,3,4}, file_size is plausible.
[[nodiscard]] bool is_mh_bin(std::span<const std::uint8_t> bytes) noexcept;

// Load a .bin file from disk, performing XOR decryption.
[[nodiscard]] Result<MhFile> read_mh_bin(const std::filesystem::path& path);

// Profile-aware server resource entry point. The current PlayDH server
// profile uses its recovered opaque AIGroup transform; the 2008 reference
// profile uses the recovered MHFileEx layout. Profiles remain explicit and
// are never silently substituted.
[[nodiscard]] Result<MhFile> read_server_mh_bin(
    const std::filesystem::path& path, std::string_view profile_id);

// Read the raw size-prefixed payload of a server profile without decoding it.
// Returns UnsupportedOpaqueServerProfile when the structural marker is absent.
[[nodiscard]] Result<OpaqueServerContainer> read_opaque_server_container(
    const std::filesystem::path& path);

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
