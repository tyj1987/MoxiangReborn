// Tests for mxh::compat::BmhmMap - .bmhm / .mhm map descriptor parser.
//
// Test surface:
//   - synthesize-and-parse: hand-build text blobs, then encrypt/parse
//   - roundtrip:            encrypt -> parse -> serialize -> re-encrypt -> parse
//   - real-resource:        decode Map0.bmhm and Map101.bmhm from PlayDH
//
// Notes:
//   - Tests skip the real-resource cases if PlayDH is missing (CI-friendly).
//   - All tests use hardcoded LR"..." paths; the workspace is fixed.

#include "mxh/compat/bmhm_map.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace mxh::compat;

namespace {

// Hardcoded project paths (workspace is fixed).  Use C:\moxiang\ (the
// canonical project root on this machine); the original D:\Moxian\
// hardcode was a stale path from a prior mount that no longer exists.
const std::filesystem::path kRealMap0   = LR"(C:\moxiang\墨香【源码配套资源】\PlayDH\Resource\Map\Map0.bmhm)";
const std::filesystem::path kRealMap101 = LR"(C:\moxiang\墨香【源码配套资源】\PlayDH\Resource\Map\Map101.bmhm)";

// Read a file fully into a byte vector.
std::vector<std::uint8_t> read_file(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto sz = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(sz);
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    return buf;
}

// Build a full .bmhm byte blob (header + crc1 + encrypted + crc2) from text.
std::vector<std::uint8_t> build_bmhm_blob(std::string_view plaintext,
                                          std::uint32_t type = 1) {
    MhFileHeader hdr{};
    hdr.version = 0x00000001;
    hdr.type = type;
    hdr.file_size = static_cast<std::uint32_t>(plaintext.size());

    std::vector<std::uint8_t> raw(plaintext.begin(), plaintext.end());
    auto enc = encrypt_bin_payload(raw, type);
    const std::uint8_t crc = compute_crc8({enc.data(), enc.size()});

    std::vector<std::uint8_t> out;
    out.reserve(14 + enc.size());
    const auto* hb = reinterpret_cast<const std::uint8_t*>(&hdr);
    out.insert(out.end(), hb, hb + sizeof(hdr));
    out.push_back(crc);
    out.insert(out.end(), enc.begin(), enc.end());
    out.push_back(crc);
    return out;
}

}  // namespace

// ---------------------------------------------------------------------
// Sniffing / size rejection
// ---------------------------------------------------------------------

TEST(BmhmMap, RejectTooShort) {
    std::vector<std::uint8_t> tiny(sizeof(MhFileHeader) + 1, 0);
    EXPECT_FALSE(BmhmMap::is_mh_desc(tiny));

    // Empty blob also rejected.
    EXPECT_FALSE(BmhmMap::is_mh_desc({}));
}

TEST(BmhmMap, RejectZeroFileSize) {
    // header is valid but file_size = 0 -> payload is empty.
    MhFileHeader h{};
    h.version = 1;
    h.type = 1;
    h.file_size = 0;
    std::vector<std::uint8_t> blob(sizeof(MhFileHeader) + 2, 0);
    std::memcpy(blob.data(), &h, sizeof(h));
    EXPECT_FALSE(BmhmMap::is_mh_desc(blob));
    EXPECT_FALSE(BmhmMap::parse(blob).has_value());
}

// ---------------------------------------------------------------------
// Synthesis: minimal Map0-style config
// ---------------------------------------------------------------------

TEST(BmhmMap, SynthesizeMap0Config) {
    const std::string text =
        "*FOG 0\n"
        "*FOGCOLOR 128 200 255 128\n"
        "*FOGDENSITY 1\n"
        "*FOGSTART 200\n"
        "*FOGEND 8000\n"
        "*MAP login.map\n"
        "*BRIGHT 140\n"
        "*SKYBOX 0\n"
        "*BGM 1667\n"
        "*SIGHT 10000\n"
        "*SUNPOS 800 1000 -200\n";

    auto blob = build_bmhm_blob(text, /*type=*/154);  // matches Map0.bmhm in wild
    ASSERT_TRUE(BmhmMap::is_mh_desc(blob));

    auto parsed = BmhmMap::parse(blob);
    ASSERT_TRUE(parsed.has_value());

    const auto& d = parsed->desc();
    EXPECT_EQ(d.fog_enabled, false);
    EXPECT_EQ(d.fog_color, (128u << 24) | (128u << 16) | (200u << 8) | 255u);
    EXPECT_FLOAT_EQ(d.fog_density, 1.0f);
    EXPECT_FLOAT_EQ(d.fog_start, 200.0f);
    EXPECT_FLOAT_EQ(d.fog_end, 8000.0f);
    EXPECT_STREQ(d.map_file_name, "login.map");
    EXPECT_EQ(d.sky_box_enabled, false);
    EXPECT_EQ(d.bgm_sound_num, 1667u);
    EXPECT_FLOAT_EQ(d.default_sight, 10000.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_x, 800.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_y, 1000.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_z, -200.0f);
    // BRIGHT sets ambient = (dd,dd,dd,dd) packed as RGBA.
    EXPECT_EQ(d.ambient, (140u << 24) | (140u << 16) | (140u << 8) | 140u);
    // Plaintext preserved verbatim.
    EXPECT_EQ(parsed->plaintext(), text);
}

// ---------------------------------------------------------------------
// Synthesis: full key set (all 25 keys covered in apply_key)
// ---------------------------------------------------------------------

TEST(BmhmMap, SynthesizeFullKeys) {
    const std::string text =
        "*SIGHT 12000\n"
        "*FOV 55\n"
        "*FOG 1\n"
        "*FOGCOLOR 80 101 0 150\n"
        "*FOGDENSITY 0.5\n"
        "*FOGSTART 1000\n"
        "*FOGEND 80000\n"
        "*MAP 101.map\n"
        "*TILE 101.ttb\n"
        "*SKYMOD sky_101_00.MOD\n"
        "*SKYANM sky_101_00.anm\n"
        "*SKYBOX 1\n"
        "*BGM 1658\n"
        "*COLOR 255 255 255 255\n"
        "*SUNPOS 5000 5000 5000\n"
        "*SUNOBJECT Moon02.chr\n"
        "*SUN 1\n"
        "*SUNDISTANCE 7000\n"
        "*BACKCOLOR 210 231 0 150\n"
        "*FIXHEIGHT 5910\n"
        "*CLOUD 64\n"
        "*CLOUDLIST cloud.bin\n"
        "*CLOUDHEIGHT 100 200\n"
        "*CAMERAFILTER filter.chr\n"
        "*CAMERAFILTERDIST 100\n"
        "*SKYOFFSET 1 2 3\n";

    auto blob = build_bmhm_blob(text, /*type=*/185);  // matches Map101.bmhm in wild
    auto parsed = BmhmMap::parse(blob);
    ASSERT_TRUE(parsed.has_value());

    const auto& d = parsed->desc();
    EXPECT_FLOAT_EQ(d.default_sight, 12000.0f);
    EXPECT_FLOAT_EQ(d.fov, 55.0f);
    EXPECT_TRUE(d.fog_enabled);
    EXPECT_FLOAT_EQ(d.fog_density, 0.5f);
    EXPECT_FLOAT_EQ(d.fog_start, 1000.0f);
    EXPECT_FLOAT_EQ(d.fog_end, 80000.0f);
    EXPECT_STREQ(d.map_file_name, "101.map");
    // TILE is prepended with "Map/" in legacy client.
    EXPECT_STREQ(d.tile_file_name, "Map/101.ttb");
    EXPECT_STREQ(d.sky_mod, "sky_101_00.MOD");
    EXPECT_STREQ(d.sky_anm, "sky_101_00.anm");
    EXPECT_TRUE(d.sky_box_enabled);
    EXPECT_EQ(d.bgm_sound_num, 1658u);
    // *COLOR forces alpha = 255.
    EXPECT_EQ(d.ambient, (0xFFu << 24) | (255u << 16) | (255u << 8) | 255u);
    EXPECT_FLOAT_EQ(d.sun_pos_x, 5000.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_y, 5000.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_z, 5000.0f);
    EXPECT_STREQ(d.sun_object, "Moon02.chr");
    EXPECT_TRUE(d.sun_enabled);
    EXPECT_FLOAT_EQ(d.sun_distance, 7000.0f);
    EXPECT_EQ(d.back_color, (150u << 24) | (210u << 16) | (231u << 8) | 0u);
    EXPECT_TRUE(d.fix_height_enabled);
    EXPECT_FLOAT_EQ(d.fix_height, 5910.0f);
    EXPECT_EQ(d.cloud_num, 64u);
    EXPECT_STREQ(d.cloud_list, "cloud.bin");
    EXPECT_EQ(d.cloud_h_min, 100);
    EXPECT_EQ(d.cloud_h_max, 200);
    EXPECT_STREQ(d.camera_filter, "filter.chr");
    EXPECT_FLOAT_EQ(d.camera_filter_dist, 100.0f);
    EXPECT_FLOAT_EQ(d.sky_offset_x, 1.0f);
    EXPECT_FLOAT_EQ(d.sky_offset_y, 2.0f);
    EXPECT_FLOAT_EQ(d.sky_offset_z, 3.0f);
}

// ---------------------------------------------------------------------
// Lines without leading '*' are silently skipped
// ---------------------------------------------------------------------

TEST(BmhmMap, SkipNonStarLines) {
    const std::string text =
        "# comment line\n"
        "FOGCOLOR 99 99 99 99  (no star; legacy skips)\n"
        "*SIGHT 5000\n"
        "\n"
        "garbage data\n"
        "*BGM 42\n";
    auto blob = build_bmhm_blob(text, /*type=*/1);
    auto parsed = BmhmMap::parse(blob);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_FLOAT_EQ(parsed->desc().default_sight, 5000.0f);
    EXPECT_EQ(parsed->desc().bgm_sound_num, 42u);
    // The non-* FOGCOLOR line should NOT have been applied.
    EXPECT_NE(parsed->desc().fog_color, (99u << 24) | (99u << 16) | (99u << 8) | 99u);
}

// ---------------------------------------------------------------------
// Round-trip: serialize -> encrypt -> parse -> serialize gives same text
// ---------------------------------------------------------------------

TEST(BmhmMap, RoundtripSerializeEncryptParse) {
    MapDesc src{};
    src.default_sight = 9999.0f;
    src.fov = 50.0f;
    src.fog_enabled = true;
    src.fog_density = 0.25f;
    src.fog_color = (10u << 24) | (20u << 16) | (30u << 8) | 40u;
    src.fog_start = 100.0f;
    src.fog_end = 9000.0f;
    src.bgm_sound_num = 7;
    src.sun_pos_x = 1.5f; src.sun_pos_y = 2.5f; src.sun_pos_z = 3.5f;
    src.sun_enabled = true;
    src.fix_height_enabled = true;
    src.fix_height = 1234.0f;
    src.cloud_num = 4;
    src.cloud_h_min = 50; src.cloud_h_max = 150;
    std::strcpy(src.map_file_name, "test.map");
    std::strcpy(src.tile_file_name, "test.ttb");
    std::strcpy(src.sky_mod, "sky.MOD");
    std::strcpy(src.sun_object, "Sun.chr");
    std::strcpy(src.camera_filter, "Filter.chr");
    std::strcpy(src.cloud_list, "clouds.bin");

    // 1. Serialize -> encrypt -> parse.
    auto blob = BmhmMap::encrypt_to_bin(src, /*type=*/42);
    auto parsed = BmhmMap::parse(blob);
    ASSERT_TRUE(parsed.has_value());

    const auto& d = parsed->desc();
    EXPECT_FLOAT_EQ(d.default_sight, 9999.0f);
    EXPECT_FLOAT_EQ(d.fov, 50.0f);
    EXPECT_TRUE(d.fog_enabled);
    EXPECT_FLOAT_EQ(d.fog_density, 0.25f);
    EXPECT_EQ(d.fog_color, (10u << 24) | (20u << 16) | (30u << 8) | 40u);
    EXPECT_EQ(d.bgm_sound_num, 7u);
    EXPECT_FLOAT_EQ(d.sun_pos_x, 1.5f);
    EXPECT_FLOAT_EQ(d.sun_pos_y, 2.5f);
    EXPECT_FLOAT_EQ(d.sun_pos_z, 3.5f);
    EXPECT_TRUE(d.sun_enabled);
    EXPECT_TRUE(d.fix_height_enabled);
    EXPECT_FLOAT_EQ(d.fix_height, 1234.0f);
    EXPECT_EQ(d.cloud_num, 4u);
    EXPECT_EQ(d.cloud_h_min, 50);
    EXPECT_EQ(d.cloud_h_max, 150);
    EXPECT_STREQ(d.map_file_name, "test.map");
    EXPECT_STREQ(d.tile_file_name, "Map/test.ttb");
    EXPECT_STREQ(d.sky_mod, "sky.MOD");
    EXPECT_STREQ(d.sun_object, "Sun.chr");
    EXPECT_STREQ(d.camera_filter, "Filter.chr");
    EXPECT_STREQ(d.cloud_list, "clouds.bin");

    // 2. Re-serialize and re-encrypt; second parse should yield the same text.
    auto blob2 = BmhmMap::encrypt_to_bin(d, /*type=*/42);
    auto parsed2 = BmhmMap::parse(blob2);
    ASSERT_TRUE(parsed2.has_value());
    EXPECT_EQ(parsed2->plaintext(), parsed->plaintext());
}

// ---------------------------------------------------------------------
// File round-trip: save_to_file -> load
// ---------------------------------------------------------------------

TEST(BmhmMap, SaveLoadFile) {
    auto tmp = std::filesystem::temp_directory_path() / "mxh_bmhm_test.bin";
    std::filesystem::remove(tmp);

    MapDesc d{};
    d.default_sight = 12345.0f;
    d.bgm_sound_num = 99;
    std::strcpy(d.map_file_name, "roundtrip.map");
    d.fog_enabled = true;

    ASSERT_EQ(BmhmMap::save_to_file(tmp, d, /*type=*/7), MhError::Ok);

    auto loaded = BmhmMap::load(tmp);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_FLOAT_EQ(loaded->desc().default_sight, 12345.0f);
    EXPECT_EQ(loaded->desc().bgm_sound_num, 99u);
    EXPECT_STREQ(loaded->desc().map_file_name, "roundtrip.map");
    EXPECT_TRUE(loaded->desc().fog_enabled);
    EXPECT_TRUE(loaded->has_bmhm_header());
    EXPECT_EQ(loaded->header().type, 7u);

    std::filesystem::remove(tmp);
}

TEST(BmhmMap, LoadMissingFile) {
    auto bogus = std::filesystem::temp_directory_path() / "mxh_bmhm_no_such_file_xyz.bin";
    std::filesystem::remove(bogus);
    EXPECT_FALSE(BmhmMap::load(bogus).has_value());
}

// ---------------------------------------------------------------------
// Real .bmhm files from PlayDH (CI-friendly: skip if not present)
// ---------------------------------------------------------------------

TEST(BmhmMap, RealMap0) {
    if (!std::filesystem::exists(kRealMap0)) {
        GTEST_SKIP() << "Map0.bmhm not found at " << kRealMap0.string();
    }
    auto loaded = BmhmMap::load(kRealMap0);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_TRUE(loaded->has_bmhm_header());
    EXPECT_EQ(loaded->header().file_size, loaded->plaintext().size());
    EXPECT_GT(loaded->plaintext().size(), 50u);

    // Map0.bmhm is small (191 bytes; payload=177). All keys in the wild carry
    // the '*' prefix, so the parser sees every line.
    const auto& d = loaded->desc();
    EXPECT_LE(loaded->header().file_size, 300u);
    EXPECT_FALSE(d.fog_enabled);            // *FOG 0
    EXPECT_EQ(d.fog_color, (128u << 24) | (128u << 16) | (200u << 8) | 255u);
    EXPECT_FLOAT_EQ(d.fog_density, 1.0f);
    EXPECT_FLOAT_EQ(d.fog_end, 8000.0f);
    EXPECT_FLOAT_EQ(d.fog_start, 200.0f);
    EXPECT_STREQ(d.map_file_name, "login.map");
    EXPECT_FALSE(d.sky_box_enabled);
    EXPECT_EQ(d.bgm_sound_num, 1667u);
    EXPECT_FLOAT_EQ(d.default_sight, 10000.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_x, 800.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_y, 1000.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_z, -200.0f);
    // *BRIGHT 140 -> ambient = (140,140,140,140) packed as RGBA.
    EXPECT_EQ(d.ambient, (140u << 24) | (140u << 16) | (140u << 8) | 140u);
}

TEST(BmhmMap, RealMap101) {
    if (!std::filesystem::exists(kRealMap101)) {
        GTEST_SKIP() << "Map101.bmhm not found at " << kRealMap101.string();
    }
    auto loaded = BmhmMap::load(kRealMap101);
    ASSERT_TRUE(loaded.has_value());

    // Map101.bmhm (517 bytes; payload=503) is the canonical example of a
    // resource where some keys are MISSING the leading '*' prefix in the wild.
    // The current parser deliberately skips such lines (matches legacy client
    // semantics). See KNOWN_BUGS.md R-3 for details. We assert only on the
    // *-prefixed subset below.
    const auto& d = loaded->desc();
    EXPECT_STREQ(d.map_file_name, "101.map");
    EXPECT_STREQ(d.tile_file_name, "Map/101.ttb");
    EXPECT_STREQ(d.sky_mod, "sky_101_00.MOD");
    EXPECT_TRUE(d.sky_box_enabled);
    EXPECT_TRUE(d.fog_enabled);
    EXPECT_FLOAT_EQ(d.fog_density, 0.5f);
    EXPECT_FLOAT_EQ(d.fog_start, 1000.0f);
    EXPECT_FLOAT_EQ(d.fog_end, 80000.0f);
    EXPECT_FLOAT_EQ(d.default_sight, 50000.0f);
    EXPECT_EQ(d.bgm_sound_num, 1658u);
    EXPECT_FLOAT_EQ(d.sun_pos_x, 5000.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_y, 5000.0f);
    EXPECT_FLOAT_EQ(d.sun_pos_z, 5000.0f);
    EXPECT_EQ(d.fog_color, (150u << 24) | (80u << 16) | (101u << 8) | 0u);
    EXPECT_EQ(d.back_color, (150u << 24) | (210u << 16) | (231u << 8) | 0u);
    EXPECT_EQ(d.ambient, (0xFFu << 24) | (255u << 16) | (255u << 8) | 255u);

    // Fields whose tokens exist in Map101.bmhm WITHOUT the '*' prefix (and so
    // fall back to defaults). Documenting the expected behavior:
    EXPECT_FLOAT_EQ(d.fov, 60.0f);             // FOV 55 -> skipped (default 60)
    EXPECT_FALSE(d.fix_height_enabled);        // FIXHEIGHT 5910 -> skipped
    EXPECT_FLOAT_EQ(d.fix_height, 0.0f);
    EXPECT_FLOAT_EQ(d.sun_distance, 2000.0f);  // SUNDISTANCE 7000 -> skipped
    EXPECT_FALSE(d.sun_enabled);               // SUN 0 -> skipped (default false)
    EXPECT_FLOAT_EQ(d.sky_offset_x, 0.0f);     // SKYOFFSET 0 0 0 -> skipped
    EXPECT_FLOAT_EQ(d.sky_offset_y, 0.0f);
    EXPECT_FLOAT_EQ(d.sky_offset_z, 0.0f);
}