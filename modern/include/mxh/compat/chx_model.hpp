// ChxModel.hpp - Modern C++ reimplementation of the .chx character manifest.
//
// Original source:
//   - 墨香【源码】\4DYUCHIGXEXECUTIVE\executive.cpp
//       CoExecutive::PreLoadGXObject() lines 1512-1570
//       CoExecutive::LoadModelData() lines 1585-1645 (shared with .chr)
//   - 墨香【源码】\4DyuchiGRX_common\typedef.h
//       #define PID_MOD_FILENAME "*MOD_FILE_NAME"
//       #define PID_MOTION_NUM    "*MOTION_NUM"
//
// Format (verified empirically via test-extract Character.pak:man.chx):
//   The .chx file is a TAB-SEPARATED plain-text manifest that wraps
//   one or more model parts into a single character. Its shape is
//   similar to .chr but with a leading count:
//
//       *MOD_FILE_NUM    <N>            # required
//       *MOD_FILE_NAME   <path_1>       # exactly N
//       *MOD_FILE_NAME   <path_2>
//       ...
//       *MOD_FILE_NAME   <path_N>
//       *MOTION_NUM      <M>            # optional, default 0
//       <motion_path_1>                 # exactly M
//       ...
//       <motion_path_M>
//
//   Unlike .chr, a .chx file has at most one *MOD_FILE_NUM block and
//   the model parts share a single motion list (no per-section list,
//   no material list). Materials are referenced inside the individual
//   .MOD files. *MOD_FILE_NAME is the only section-style token; the
//   file is otherwise flat.
//
//   Verified with Character.pak:man.chx (the first line is
//   "*MOD_FILE_NUM\t5", followed by 5 *MOD_FILE_NAME lines).
//
//   This file does NOT contain vertex/index/mesh/bone data. Those
//   live in the .mod files referenced by *MOD_FILE_NAME. We only
//   parse the manifest here.

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

inline constexpr std::size_t kChxMaxModPath   = 260;     // _MAX_PATH on Windows
inline constexpr std::size_t kChxMaxMotions   = 8192;    // defensive cap
inline constexpr std::size_t kChxMaxModFiles  = 256;     // defensive cap

// Decoded view of one .chx file. mod_files lists the .MOD parts in
// the order they appear (this matches the order the engine uses to
// piece the character together). motions is the shared animation
// list (often empty for static / non-animated .chx).
struct ChxModel {
    std::vector<std::string> mod_files;     // e.g. {"M_HAIR01.MOD", "M_BODY01.MOD", ...}
    std::vector<std::string> motions;       // e.g. {"man_walk.ANM", ...}

    // Original plaintext, kept for round-trip tools.
    [[nodiscard]] const std::string& plaintext() const noexcept {
        return plaintext_;
    }
    [[nodiscard]] std::size_t mod_count() const noexcept { return mod_files.size(); }
    [[nodiscard]] std::size_t motion_count() const noexcept { return motions.size(); }
    [[nodiscard]] bool empty() const noexcept { return mod_files.empty(); }

    // Factory: parse from raw bytes. .chx is plain text so we
    // never reject on a "binary magic" check. Returns std::nullopt
    // only if the payload is empty or contains no *MOD_FILE_NAME
    // lines.
    [[nodiscard]] static std::optional<ChxModel> parse(
        std::span<const std::uint8_t> bytes);

    // Convenience: read + parse from disk. Returns std::nullopt on
    // missing file / empty file.
    [[nodiscard]] static std::optional<ChxModel> load(
        const std::filesystem::path& path);

    // Encode helper (round-trip; useful for tooling). Produces a
    // plain-text .chx payload matching the legacy file shape. The
    // leading *MOD_FILE_NUM count and trailing *MOTION_NUM block
    // are both written. motions is clamped to 0 if empty.
    [[nodiscard]] static std::string serialize_text(
        const std::vector<std::string>& mod_files,
        const std::vector<std::string>& motions) noexcept;

    // Save a .chx file to disk. Truncates existing files.
    [[nodiscard]] static bool save_to_file(
        const std::filesystem::path& path,
        const std::vector<std::string>& mod_files,
        const std::vector<std::string>& motions) noexcept;

private:
    std::string plaintext_;
};

}  // namespace mxh::compat
