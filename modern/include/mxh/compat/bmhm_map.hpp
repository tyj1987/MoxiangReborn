// BmhmMap.hpp - Modern C++ reimplementation of MAP%d.bmhm / .mhm map descriptor.
//
// Original source:
//   - 墨香ï¼æºç ï¼\4DyuchiFileStorage\PackFile.cpp (XOR layout)
//   - 墨香ï¼æºç ï¼\[Client]MH\MHMap.cpp        (MAPDESC keys)
//   - 墨香ï¼æºç ï¼\[Client]MH\MHFile.cpp        (decryption algorithm)
//
// Format (verified empirically against Map0.bmhm / Map101.bmhm):
//   [12 bytes]  MhFileHeader { u32 version, u32 type, u32 file_size }
//   [1 byte ]   crc1 (informational; legacy code never validates it)
//   [N bytes ]  XOR-encrypted text payload, N = file_size
//                decrypt: p[i] -= i; if (i % type == 0) p[i] -= type
//   [1 byte ]   crc2 (trailing; legacy code never validates it either)
//
// Payload is a UTF-8 / ASCII text config file with one key per line:
//   *SIGHT 10000
//   *FOV 55
//   *FOG 1
//   *FOGCOLOR 80 101 0 150
//   *FOGDENSITY 0.5
//   *FOGSTART 1000
//   *FOGEND 80000
//   *MAP 101.map
//   *TILE 101.ttb
//   *SKYMOD sky_101_00.MOD
//   *SKYANM sky_101_00.anm
//   *SKYBOX 1
//   *BGM 1658
//   *COLOR 255 255 255 255
//   *SUNPOS 5000 5000 5000
//   *SUNOBJECT Moon02.chr
//   *SUN 0
//   *SUNDISTANCE 7000
//   *BRIGHT 140
//   *BACKCOLOR 210 231 0 150
//   *FIXHEIGHT 5910
//   *CLOUD 64
//   *CLOUDLIST cloud.bin
//   *CLOUDHEIGHT 100 200
//   *CAMERAFILTER filter.chr
//   *CAMERAFILTERDIST 100
//   *SKYOFFSET 0 0 0
//
// Lines without leading '*' or '//' comment are silently skipped (defensive:
// some MHM files contain a few stray duplicate keys that lack the '*' prefix).

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "mxh/compat/mh_file_ex.hpp"

namespace mxh::compat {

// MAPDESC - one-to-one mirror of MHMap.cpp:mapDesc (selected fields used by
// modern render code). Vectors of fixed-length char arrays to keep the type
// trivially-copyable + easy to serialize.
#pragma pack(push, 1)
struct MapDesc {
    // Camera / fog
    float    default_sight     = 8000.0f;
    float    fov               = 60.0f;     // degrees
    bool     fog_enabled       = false;
    float    fog_density       = 1.0f;
    float    fog_start         = 2000.0f;
    float    fog_end           = 8000.0f;
    std::uint32_t fog_color    = 0x80808080u;  // RGBA packed

    // File references (null-terminated ASCII in original C struct)
    char     map_file_name[64] = {0};        // e.g. "Map/login.map"
    char     tile_file_name[64] = {0};       // e.g. "Map/101.ttb"
    char     sky_mod[64]        = {0};       // e.g. "sky_101_00.MOD"
    char     sky_anm[64]        = {0};       // e.g. "sky_101_00.anm"
    bool     sky_box_enabled    = false;
    char     sun_object[64]     = {0};       // e.g. "Moon02.chr"
    bool     sun_enabled        = false;
    float    sun_distance       = 2000.0f;
    char     camera_filter[64]  = {0};
    float    camera_filter_dist = 100.0f;
    char     cloud_list[64]     = {0};

    // Audio / BGM (BGM is u16 in old code: max 65535)
    std::uint16_t bgm_sound_num = 0;

    // Color / ambient
    std::uint32_t ambient    = 0xFFFFFFFFu;
    std::uint32_t back_color = 0x00000000u;

    // Sun position (VECTOR3)
    float sun_pos_x = 0.0f;
    float sun_pos_y = 0.0f;
    float sun_pos_z = 0.0f;

    // Sky offset (VECTOR3)
    float sky_offset_x = 0.0f;
    float sky_offset_y = 0.0f;
    float sky_offset_z = 0.0f;

    // Cloud
    std::uint32_t cloud_num     = 0;
    int            cloud_h_min   = 0;
    int            cloud_h_max   = 0;

    // Fix-height (clip plane below ground)
    bool     fix_height_enabled = false;
    float    fix_height         = 0.0f;
};
#pragma pack(pop)
static_assert(sizeof(MapDesc) <= 2048, "MapDesc should stay small (in-memory only)");

// BmhmMap - high-level facade for one map descriptor file (.bmhm XOR-encrypted
// or .mhm plaintext). Exposes both raw bytes (for tooling) and parsed fields.
class BmhmMap {
public:
    // Factory: detect .bmhm / .mhm magic and parse from raw bytes.
    // Returns std::nullopt if input is too small or has an invalid header.
    [[nodiscard]] static std::optional<BmhmMap> parse(
        std::span<const std::uint8_t> bytes);

    // Convenience: open + parse from disk. .bmhm => XOR-decrypt then parse text;
    // .mhm => parse text directly.
    [[nodiscard]] static std::optional<BmhmMap> load(
        const std::filesystem::path& path);

    // Sniff: returns true if bytes look like a .bmhm / .mhm descriptor.
    // Both formats start with an MhFileHeader, so the check is the same.
    [[nodiscard]] static bool is_mh_desc(std::span<const std::uint8_t> bytes) noexcept;

    // Field accessors
    [[nodiscard]] const MapDesc& desc() const noexcept { return desc_; }
    [[nodiscard]] MapDesc& desc() noexcept { return desc_; }
    [[nodiscard]] const std::string& plaintext() const noexcept { return plaintext_; }
    [[nodiscard]] bool has_bmhm_header() const noexcept {
        return header_.has_value();
    }
    [[nodiscard]] const MhFileHeader& header() const noexcept {
        return header_.value();
    }

    // Encode helpers (round-trip; useful for tools that regenerate resources).
    [[nodiscard]] static std::vector<std::uint8_t> serialize_text(
        const MapDesc& d) noexcept;

    [[nodiscard]] static std::vector<std::uint8_t> encrypt_to_bin(
        const MapDesc& d, std::uint32_t xor_type = 1) noexcept;

    [[nodiscard]] static MhError save_to_file(
        const std::filesystem::path& path, const MapDesc& d,
        std::uint32_t xor_type = 1) noexcept;

private:
    std::optional<MhFileHeader> header_;  // present iff source was .bmhm
    std::string                 plaintext_;
    MapDesc                     desc_{};
};

// Internal helpers exposed for testing.
namespace detail {

// Strip leading whitespace + optional BOM.
[[nodiscard]] std::string_view trim(std::string_view s) noexcept;

// Split a line by ASCII whitespace into tokens (preserving tokens, ignoring
// empty runs).
[[nodiscard]] std::vector<std::string_view> tokenize(std::string_view line) noexcept;

// Parse "key [args...]" line into MAPDESC. Lines that don't start with '*'
// are silently skipped. Returns true if a recognized key was matched.
bool apply_key(BmhmMap& map, std::string_view key, std::span<std::string_view> args);

}  // namespace detail

}  // namespace mxh::compat
