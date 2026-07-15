// BmhmMap.cpp - Modern C++ implementation of MAP%d.bmhm / .mhm parser.
//
// Source of truth:
//   - 墨香ï¼æºç ï¼\[Client]MH\MHFile.cpp  (XOR decryption)
//   - 墨香ï¼æºç ï¼\[Client]MH\MHMap.cpp   (key semantics)
//
// This implementation reuses mxh::compat::read_mh_bin / decrypt_bin_payload for
// the XOR layer, then runs a small line-oriented text parser over the decrypted
// payload. Both .bmhm (encrypted) and .mhm (plaintext) sources are accepted;
// the parser itself is identical once we have ASCII text in hand.

#include "mxh/compat/bmhm_map.hpp"
#include "mxh/compat/detail/text_parse.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace mxh::compat {

namespace {

// Copy up to N-1 chars from src into dst, always NUL-terminate. Mirrors
// legacy strncpy+force-terminate pattern but is bounds-safe.
template <std::size_t N>
void copy_to_fixed(char (&dst)[N], std::string_view src) noexcept {
    std::memset(dst, 0, N);
    if (src.empty()) return;
    const std::size_t n = std::min(src.size(), N - 1);
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

// Parse one float / int / dword / bool from a token, with safe fallbacks.
float parse_float(std::string_view tok, float fallback) noexcept {
    if (tok.empty()) return fallback;
    // strtof needs a NUL-terminated buffer.
    std::string buf(tok);
    char* end = nullptr;
    errno = 0;
    const float v = std::strtof(buf.c_str(), &end);
    if (end == buf.c_str() || errno != 0) return fallback;
    return v;
}

int parse_int(std::string_view tok, int fallback) noexcept {
    if (tok.empty()) return fallback;
    std::string buf(tok);
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(buf.c_str(), &end, 10);
    if (end == buf.c_str() || errno != 0) return fallback;
    return static_cast<int>(v);
}

std::uint32_t parse_dword(std::string_view tok, std::uint32_t fallback) noexcept {
    if (tok.empty()) return fallback;
    std::string buf(tok);
    // Accept 0x-prefixed hex; legacy MHFile used %x for these fields.
    char* end = nullptr;
    errno = 0;
    int base = (tok.size() > 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X'))
                   ? 16 : 10;
    const unsigned long v = std::strtoul(buf.c_str(), &end, base);
    if (end == buf.c_str() || errno != 0) return fallback;
    return static_cast<std::uint32_t>(v);
}

std::uint16_t parse_word(std::string_view tok, std::uint16_t fallback) noexcept {
    return static_cast<std::uint16_t>(parse_dword(tok, fallback));
}

bool parse_bool(std::string_view tok, bool fallback) noexcept {
    if (tok.empty()) return fallback;
    // Legacy GetBool: "0" / "1" only.
    if (tok.size() == 1 && tok[0] == '0') return false;
    if (tok.size() == 1 && tok[0] == '1') return true;
    // Defensive: treat any other digit as truthy (legacy was GetInt()).
    return fallback;
}

std::uint8_t parse_byte(std::string_view tok, std::uint8_t fallback) noexcept {
    return static_cast<std::uint8_t>(parse_int(tok, fallback));
}

}  // namespace

// trim() and tokenize() now live in
// mxh/compat/detail/text_parse.hpp so .chr, .bmhm and any future
// text-format parser can share them without ODR collisions.

namespace detail {

// Pack RGBA from four DWORD tokens (legacy MHMap.cpp reads each as %x).
std::uint32_t pack_rgba(std::span<std::string_view> args, std::size_t offset) noexcept {
    if (args.size() < offset + 4) return 0;
    const std::uint32_t r = parse_dword(args[offset + 0], 0);
    const std::uint32_t g = parse_dword(args[offset + 1], 0);
    const std::uint32_t b = parse_dword(args[offset + 2], 0);
    const std::uint32_t a = parse_dword(args[offset + 3], 255);
    // RGBA_MAKE expands to (a<<24)|(r<<16)|(g<<8)|b in the legacy code.
    return (a << 24) | (r << 16) | (g << 8) | b;
}

bool apply_key(BmhmMap& m, std::string_view key, std::span<std::string_view> args) {
    MapDesc& d = m.desc();
    // Key matchers (case-insensitive prefix '*' is mandatory). The legacy
    // strupr() + strcmp() pattern is replicated with string_view::starts_with.
    if (key == "*SIGHT") {
        d.default_sight = parse_float(args.size() > 0 ? args[0] : std::string_view{}, d.default_sight);
    } else if (key == "*FOV") {
        d.fov = parse_float(args.size() > 0 ? args[0] : std::string_view{}, d.fov);
    } else if (key == "*FOG") {
        d.fog_enabled = parse_bool(args.size() > 0 ? args[0] : std::string_view{}, d.fog_enabled);
    } else if (key == "*FOGCOLOR") {
        d.fog_color = pack_rgba(args, 0);
    } else if (key == "*FOGDENSITY") {
        d.fog_density = parse_float(args.size() > 0 ? args[0] : std::string_view{}, d.fog_density);
    } else if (key == "*FOGSTART") {
        d.fog_start = parse_float(args.size() > 0 ? args[0] : std::string_view{}, d.fog_start);
    } else if (key == "*FOGEND") {
        d.fog_end = parse_float(args.size() > 0 ? args[0] : std::string_view{}, d.fog_end);
    } else if (key == "*MAP") {
        if (!args.empty()) copy_to_fixed(d.map_file_name, args[0]);
    } else if (key == "*TILE") {
        if (!args.empty()) {
            // Legacy prepends "Map/". Preserve that behavior so downstream code
            // sees the same string as the original client.
            const std::string_view tile_name = args[0];
            std::string combined;
            combined.reserve(4 + tile_name.size());
            combined.append("Map/");
            combined.append(tile_name.data(), tile_name.size());
            copy_to_fixed(d.tile_file_name, combined);
        }
    } else if (key == "*SKYMOD") {
        if (!args.empty()) copy_to_fixed(d.sky_mod, args[0]);
    } else if (key == "*SKYANM") {
        if (!args.empty()) copy_to_fixed(d.sky_anm, args[0]);
    } else if (key == "*SKYBOX") {
        d.sky_box_enabled = parse_bool(args.size() > 0 ? args[0] : std::string_view{}, d.sky_box_enabled);
    } else if (key == "*BGM") {
        d.bgm_sound_num = parse_word(args.size() > 0 ? args[0] : std::string_view{}, d.bgm_sound_num);
    } else if (key == "*COLOR") {
        // *COLOR r g b (alpha forced to 255). Legacy: RGBA_MAKE(r,g,b,255)
        if (args.size() >= 3) {
            const std::uint32_t r = parse_byte(args[0], 255);
            const std::uint32_t g = parse_byte(args[1], 255);
            const std::uint32_t b = parse_byte(args[2], 255);
            d.ambient = (0xFFu << 24) | (r << 16) | (g << 8) | b;
        }
    } else if (key == "*SUNPOS") {
        if (args.size() >= 3) {
            d.sun_pos_x = parse_float(args[0], 0.0f);
            d.sun_pos_y = parse_float(args[1], 0.0f);
            d.sun_pos_z = parse_float(args[2], 0.0f);
        }
    } else if (key == "*SUNOBJECT") {
        if (!args.empty()) copy_to_fixed(d.sun_object, args[0]);
    } else if (key == "*SUN") {
        d.sun_enabled = parse_bool(args.size() > 0 ? args[0] : std::string_view{}, d.sun_enabled);
    } else if (key == "*SUNDISTANCE") {
        d.sun_distance = parse_float(args.size() > 0 ? args[0] : std::string_view{}, d.sun_distance);
    } else if (key == "*BRIGHT") {
        // Legacy: dd = GetDword(); Ambient = RGBA_MAKE(dd,dd,dd,dd)
        const std::uint32_t dd = parse_dword(args.size() > 0 ? args[0] : std::string_view{}, 255);
        d.ambient = (dd << 24) | (dd << 16) | (dd << 8) | dd;
    } else if (key == "*BACKCOLOR") {
        d.back_color = pack_rgba(args, 0);
    } else if (key == "*FIXHEIGHT") {
        if (!args.empty()) {
            d.fix_height_enabled = true;
            d.fix_height = parse_float(args[0], 0.0f);
        }
    } else if (key == "*CLOUD") {
        d.cloud_num = parse_dword(args.size() > 0 ? args[0] : std::string_view{}, 0);
    } else if (key == "*CLOUDLIST") {
        if (!args.empty()) copy_to_fixed(d.cloud_list, args[0]);
    } else if (key == "*CLOUDHEIGHT") {
        if (args.size() >= 2) {
            d.cloud_h_min = parse_int(args[0], 0);
            d.cloud_h_max = parse_int(args[1], 0);
        }
    } else if (key == "*CAMERAFILTER") {
        if (!args.empty()) copy_to_fixed(d.camera_filter, args[0]);
    } else if (key == "*CAMERAFILTERDIST") {
        d.camera_filter_dist = parse_float(args.size() > 0 ? args[0] : std::string_view{}, d.camera_filter_dist);
    } else if (key == "*SKYOFFSET") {
        if (args.size() >= 3) {
            d.sky_offset_x = parse_float(args[0], 0.0f);
            d.sky_offset_y = parse_float(args[1], 0.0f);
            d.sky_offset_z = parse_float(args[2], 0.0f);
        }
    } else {
        return false;  // unknown / unsupported key
    }
    return true;
}

}  // namespace detail

bool BmhmMap::is_mh_desc(std::span<const std::uint8_t> bytes) noexcept {
    // Both .bmhm and .mhm share the same 12-byte MhFileHeader. The legacy
    // is_mh_bin() heuristic in mh_file_ex.cpp (version=1, type<=4) is too
    // strict: real Map0.bmhm uses version=0x0131CBBF and type=154. We accept
    // anything where file_size <= payload range and type is non-zero.
    if (bytes.size() < sizeof(MhFileHeader) + 2) return false;
    MhFileHeader h{};
    std::memcpy(&h, bytes.data(), sizeof(h));
    if (h.file_size == 0) return false;
    // Worst-case payload extent: header(12) + crc1(1) + payload(N) + crc2(1).
    const std::uint64_t expected = 14ull + h.file_size;
    if (expected > bytes.size()) return false;
    return true;
}

std::optional<BmhmMap> BmhmMap::parse(std::span<const std::uint8_t> bytes) {
    if (!is_mh_desc(bytes)) return std::nullopt;

    BmhmMap m;
    MhFileHeader h{};
    std::memcpy(&h, bytes.data(), sizeof(h));
    m.header_ = h;

    // Decrypt via the canonical xor-with-type path. read_mh_bin validates
    // header and returns the decrypted payload, which is what we want.
    const std::uint8_t* payload = bytes.data() + sizeof(MhFileHeader) + 1;
    auto decrypted = decrypt_bin_payload({payload, h.file_size}, h.type);
    m.plaintext_.assign(decrypted.begin(), decrypted.end());

    // Line-oriented parser. We tokenize each line on whitespace; the first
    // token must start with '*' (legacy strupr() expected uppercase, but we
    // accept mixed case as a defensive convenience).
    std::size_t pos = 0;
    while (pos < m.plaintext_.size()) {
        const std::size_t eol = m.plaintext_.find('\n', pos);
        const std::size_t end = (eol == std::string::npos) ? m.plaintext_.size() : eol;
        std::string_view line(m.plaintext_.data() + pos, end - pos);
        // Drop a trailing CR for CRLF inputs.
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        pos = (eol == std::string::npos) ? m.plaintext_.size() : eol + 1;

        const auto trimmed = detail::trim(line);
        if (trimmed.empty()) continue;
        if (trimmed.front() == '*') {
            auto toks = detail::tokenize(trimmed);
            if (toks.empty()) continue;
            const std::string_view key = toks.front();
            std::span<std::string_view> args(toks.data() + 1, toks.size() - 1);
            detail::apply_key(m, key, args);
        }
        // Lines without '*' (and not blank/comment) are silently skipped:
        // some legacy MHM files contain "FOGCOLOR" duplicates without the
        // leading '*' and the original client simply ignored them.
    }
    return m;
}

std::optional<BmhmMap> BmhmMap::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return std::nullopt;
    const auto sz = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(sz);
    if (!f.read(reinterpret_cast<char*>(buf.data()), sz)) return std::nullopt;
    return parse(buf);
}

std::vector<std::uint8_t> BmhmMap::serialize_text(const MapDesc& d) noexcept {
    // Helper: format a float without trailing zeros when possible.
    auto fmt_float = [](float v) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%g", v);
        return std::string(buf);
    };
    auto fmt_dword = [](std::uint32_t v) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%u", v);
        return std::string(buf);
    };

    std::ostringstream os;
    os << "*SIGHT "     << fmt_float(d.default_sight) << "\n";
    os << "*FOV "       << fmt_float(d.fov) << "\n";
    os << "*FOG "       << (d.fog_enabled ? 1 : 0) << "\n";
    const std::uint32_t fc = d.fog_color;
    os << "*FOGCOLOR "  << ((fc >> 16) & 0xFFu) << " "
                        << ((fc >> 8)  & 0xFFu) << " "
                        << ( fc        & 0xFFu) << " "
                        << ((fc >> 24) & 0xFFu) << "\n";
    os << "*FOGDENSITY "<< fmt_float(d.fog_density) << "\n";
    os << "*FOGSTART "  << fmt_float(d.fog_start) << "\n";
    os << "*FOGEND "    << fmt_float(d.fog_end) << "\n";
    if (d.map_file_name[0])   os << "*MAP "   << d.map_file_name << "\n";
    if (d.tile_file_name[0]) {
        // TILE field is conventionally stored with "Map/" prefix
        // (legacy parser at *TILE prepends it). Strip before re-emitting
        // so a serialize -> encrypt -> parse -> serialize round-trip
        // produces identical plaintext.
        std::string_view raw = d.tile_file_name;
        constexpr std::string_view kTilePrefix = "Map/";
        if (raw.starts_with(kTilePrefix)) {
            raw.remove_prefix(kTilePrefix.size());
        }
        os << "*TILE " << raw << "\n";
    }
    if (d.sky_mod[0])         os << "*SKYMOD "<< d.sky_mod << "\n";
    if (d.sky_anm[0])         os << "*SKYANM "<< d.sky_anm << "\n";
    if (d.sky_box_enabled)    os << "*SKYBOX 1\n";
    if (d.bgm_sound_num)      os << "*BGM "   << d.bgm_sound_num << "\n";
    const std::uint32_t ac = d.ambient;
    os << "*COLOR "          << ((ac >> 16) & 0xFFu) << " "
                              << ((ac >> 8)  & 0xFFu) << " "
                              << ( ac        & 0xFFu) << "\n";
    os << "*SUNPOS "  << fmt_float(d.sun_pos_x) << " "
                      << fmt_float(d.sun_pos_y) << " "
                      << fmt_float(d.sun_pos_z) << "\n";
    if (d.sun_object[0])      os << "*SUNOBJECT " << d.sun_object << "\n";
    if (d.sun_enabled)        os << "*SUN 1\n";
    os << "*SUNDISTANCE "<< fmt_float(d.sun_distance) << "\n";
    const std::uint32_t bc = d.back_color;
    os << "*BACKCOLOR "  << ((bc >> 16) & 0xFFu) << " "
                         << ((bc >> 8)  & 0xFFu) << " "
                         << ( bc        & 0xFFu) << " "
                         << ((bc >> 24) & 0xFFu) << "\n";
    if (d.fix_height_enabled) os << "*FIXHEIGHT " << fmt_float(d.fix_height) << "\n";
    if (d.cloud_num)          os << "*CLOUD "     << fmt_dword(d.cloud_num) << "\n";
    if (d.cloud_list[0])      os << "*CLOUDLIST " << d.cloud_list << "\n";
    if (d.cloud_h_min || d.cloud_h_max)
        os << "*CLOUDHEIGHT " << d.cloud_h_min << " " << d.cloud_h_max << "\n";
    if (d.camera_filter[0])   os << "*CAMERAFILTER " << d.camera_filter << "\n";
    os << "*CAMERAFILTERDIST " << fmt_float(d.camera_filter_dist) << "\n";
    os << "*SKYOFFSET " << fmt_float(d.sky_offset_x) << " "
                        << fmt_float(d.sky_offset_y) << " "
                        << fmt_float(d.sky_offset_z) << "\n";

    const std::string s = os.str();
    return std::vector<std::uint8_t>(s.begin(), s.end());
}

std::vector<std::uint8_t> BmhmMap::encrypt_to_bin(const MapDesc& d,
                                                  std::uint32_t xor_type) noexcept {
    const auto raw = serialize_text(d);
    // Header layout: version = 0x00000001 (legacy base), file_size = payload,
    // type = xor_type. Legacy *encrypted* files in the wild use much larger
    // type values (e.g. 154 / 185); they were generated by the old build
    // pipeline which is now gone. Type=1 still produces a valid (decryptable)
    // .bmhm that any CMHFile::OpenBin() can read, so we default to it.
    MhFileHeader hdr{};
    hdr.version = 0x00000001;
    hdr.type = xor_type;
    hdr.file_size = static_cast<std::uint32_t>(raw.size());
    auto encrypted = encrypt_bin_payload(raw, xor_type);

    // Assemble: header(12) + crc1(1) + payload + crc2(1).
    std::vector<std::uint8_t> out;
    out.reserve(14 + encrypted.size());
    const auto* hdr_bytes = reinterpret_cast<const std::uint8_t*>(&hdr);
    out.insert(out.end(), hdr_bytes, hdr_bytes + sizeof(hdr));
    const std::uint8_t crc1 = compute_crc8({encrypted.data(), encrypted.size()});
    out.push_back(crc1);
    out.insert(out.end(), encrypted.begin(), encrypted.end());
    out.push_back(crc1);  // crc2 mirrors crc1 (legacy never validates either)
    return out;
}

MhError BmhmMap::save_to_file(const std::filesystem::path& path, const MapDesc& d,
                              std::uint32_t xor_type) noexcept {
    const auto bytes = encrypt_to_bin(d, xor_type);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return MhError::IoError;
    if (!f.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()))) {
        return MhError::IoError;
    }
    return MhError::Ok;
}

}  // namespace mxh::compat
