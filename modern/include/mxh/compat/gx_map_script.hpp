// gx_map_script.hpp — PlayDH GX map script (.map) loader.
//
// PlayDH entries are ASCII and start with "GX_" (GX_METADATA / GX_MAP / ...).
// Original-client entries stored in 96-byte paks are obfuscated: the first
// four bytes are a little-endian uint32 equal to the entry's own size, then
// a 20-byte header, then a body XOR'd with an unknown rolling key. This
// loader does not decode that body. It rejects the obfuscated form before
// any parse so it cannot be treated as an empty map.

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace mxh::compat {

struct GxMapScript {
    bool has_metadata = false;
    float box_max[3] = {0.0f, 0.0f, 0.0f};
    float box_min[3] = {0.0f, 0.0f, 0.0f};
    std::string static_model;
    std::string height_field;
};

enum class GxMapReject : std::uint8_t {
    None = 0,
    Obfuscated,
    Malformed,
};

struct GxMapParseResult {
    bool ok = false;
    GxMapReject reject = GxMapReject::None;
    // Exact ERROR line logged on failure. Empty when ok.
    std::string message;
    GxMapScript script;
};

// Detects obfuscation first (size >= 24, LE uint32 prefix == size, and the
// buffer does not start with "GX_"). On that hit, logs
// `ERROR [map] obfuscated .map not supported: <name> (bytes=<n>)` and
// returns without parsing. Any other buffer that does not start with "GX_"
// or that is not a balanced GX script logs
// `ERROR [map] malformed .map: <name> (bytes=<n>)` and fails.
// `name` is only used in the log line.
[[nodiscard]] GxMapParseResult parse_gx_map_script(
    std::span<const std::uint8_t> bytes, std::string_view name);

// How the client found the .map entry. Empty is a found 0-byte file, not
// a missing file.
enum class GxMapScriptPresence : std::uint8_t {
    Absent = 0,
    Empty,
    Present,
};

// Client load gate. The accept/reject decision is the file header only:
// obfuscated, or not starting with "GX_" (including a found 0-byte file),
// logs the same ERROR as parse_gx_map_script and sets fail_load.
// A buffer that starts with "GX_" is accepted even when the grammar does
// not parse. That case logs
// `WARN [map] script parse incomplete: <name>` and fail_load stays false.
// Absent logs nothing and does not fail.
struct GxMapLoadGate {
    bool fail_load = false;
    std::string message;
};

[[nodiscard]] GxMapLoadGate gate_map_script_load(
    GxMapScriptPresence presence,
    std::span<const std::uint8_t> bytes,
    std::string_view name);

}  // namespace mxh::compat
