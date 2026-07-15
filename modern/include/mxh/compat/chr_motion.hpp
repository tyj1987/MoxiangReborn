// ChrModel.hpp - Modern C++ reimplementation of the .chr character manifest.
//
// Original source:
//   - 墨香【源码】\4DYUCHIGXEXECUTIVE\executive.cpp
//       CoExecutive::LoadGXObject() lines 1485-1510
//       CoExecutive::LoadModelData() lines 1585-1645
//   - 墨香【源码】\4DyuchiGRX_common\typedef.h
//       #define PID_MOD_FILENAME "*MOD_FILE_NAME"
//       #define PID_MOTION_NUM    "*MOTION_NUM"
//       #define PID_MATERIAL_NUM  "*MATERIAL_NUM"
//
// Format (verified empirically against test-extract/11160.chr):
//   The .chr file is a plain-text manifest (whitespace-separated tokens).
//   It contains ONE OR MORE model sections, each shaped:
//
//       *MOD_FILE_NAME <path>            # required, e.g. "11160.MOD"
//       *MOTION_NUM    <N>               # optional, default 0
//           <motion_path_1>              # exactly N lines, one path each
//           <motion_path_2>
//           ...
//       *MATERIAL_NUM  <N>               # optional, default 0
//           <material_path_1>            # exactly N lines, one path each
//           ...
//       *MOD_FILE_NAME <path>            # next model section begins
//       ...
//
//   A section ends either at the next *MOD_FILE_NAME token or at EOF.
//   Lines that don't match any PID_* token (or that contain fewer tokens
//   than expected) are silently tolerated by the legacy parser, so we do
//   the same. Out-of-range motion/material counts are clamped to 0.
//
//   This file does NOT contain bone-track data. Bone tracks live in the
//   .mod file (FILE_SCENE_HEADER + mesh/light/camera/bone objects), and
//   motion keyframes live in the .ANM file referenced by *MOTION_NUM.

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

// Max number of motion paths we'll store per model section. The legacy
// game never needs more than a few dozen; we cap defensively to avoid
// runaway allocations on malformed input.
inline constexpr std::size_t kChrModelMaxMotions   = 1024;
inline constexpr std::size_t kChrModelMaxMaterials = 1024;
inline constexpr std::size_t kChrModelMaxModPath   = 260;   // _MAX_PATH on Windows
inline constexpr std::size_t kChrModelMaxSections  = 64;

// One model section inside a .chr file. Corresponds 1:1 with a
// "*MOD_FILE_NAME" block and its trailing *MOTION_NUM / *MATERIAL_NUM
// lists.
//
// NOTE: No #pragma pack here. The struct holds std::vector (which
// requires 8-byte alignment) so a packed struct would misalign the
// vectors' internal pointers and trip C4315. This type is in-memory
// only; the on-disk .chr is plain text and never sees this layout.
struct ChrModelSection {
    char mod_file[kChrModelMaxModPath] = {0};   // e.g. "11160.MOD"
    std::vector<std::string> motions;            // e.g. {"11160.ANM"}
    std::vector<std::string> materials;          // e.g. {"weapon.MML"}
};

// High-level facade for one .chr manifest. Exposes a list of parsed
// model sections plus the original text (for round-trip tools).
class ChrModel {
public:
    // Factory: parse from raw bytes. .chr is plain text so we never
    // reject on the "binary magic" check the legacy 32-byte packed
    // header stub used to do. Returns std::nullopt only if the
    // payload is empty.
    [[nodiscard]] static std::optional<ChrModel> parse(
        std::span<const std::uint8_t> bytes);

    // Convenience: read + parse from disk. Returns std::nullopt on
    // missing file / empty file.
    [[nodiscard]] static std::optional<ChrModel> load(
        const std::filesystem::path& path);

    // Field accessors
    [[nodiscard]] const std::vector<ChrModelSection>& sections() const noexcept {
        return sections_;
    }
    [[nodiscard]] const std::string& plaintext() const noexcept {
        return plaintext_;
    }

    // detail::consume_line needs a mutable handle to push new
    // sections. Not part of the public API; named with an
    // underscore suffix and intentionally undocumented to discourage
    // use outside the parser.
    std::vector<ChrModelSection>& sections_internal() noexcept {
        return sections_;
    }
    [[nodiscard]] std::size_t size() const noexcept { return sections_.size(); }
    [[nodiscard]] bool empty() const noexcept { return sections_.empty(); }

    // Encode helper (round-trip; useful for tooling). Produces a
    // plain-text .chr payload with the same line shape as the legacy
    // resource. The output is whitespace-stable and key-lower-case
    // to match what FSOpenFile(... ACCESSMODE_TEXT) produces.
    [[nodiscard]] static std::string serialize_text(
        const std::vector<ChrModelSection>& sections) noexcept;

    // Save a .chr file to disk. Truncates existing files.
    // Returns true on success, false on I/O failure.
    [[nodiscard]] static bool save_to_file(
        const std::filesystem::path& path,
        const std::vector<ChrModelSection>& sections) noexcept;

private:
    std::string                       plaintext_;
    std::vector<ChrModelSection>      sections_;
};

// Internal helpers exposed for testing.
namespace detail {

// Strip leading/trailing ASCII whitespace + optional UTF-8 BOM.
[[nodiscard]] std::string_view trim(std::string_view s) noexcept;

// Split a line by ASCII whitespace into tokens (preserving tokens,
// ignoring empty runs).
[[nodiscard]] std::vector<std::string_view> tokenize(std::string_view line) noexcept;

// Parse one "key [args...]" line. Recognized keys (legacy PIDs):
//   *MOD_FILE_NAME <path>     -> push new section, set mod_file
//   *MOTION_NUM    <N>        -> set remaining_motion_count on current section
//   *MATERIAL_NUM  <N>        -> set remaining_material_count on current section
//   <motion_path>             -> pop into current section's motions
//   <material_path>           -> pop into current section's materials
// Returns true if the line was consumed by a recognized key, false
// if the line was a stray path token or a comment.
bool consume_line(ChrModel& model,
                  std::string_view token,
                  std::span<std::string_view> rest_of_line) noexcept;

}  // namespace detail

}  // namespace mxh::compat
