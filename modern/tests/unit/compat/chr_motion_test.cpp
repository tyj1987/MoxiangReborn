// chr_motion_test.cpp - .chr character manifest parser tests (Phase 10.14 / 12.1)
//
// Covers modern/include/mxh/compat/chr_motion.hpp + src/chr_motion.cpp.
//
// The legacy 4Dyuchi .chr format is a plain-text manifest that lists
// one or more model sections. Each section has:
//   *MOD_FILE_NAME <path>          # required section header
//   *MOTION_NUM    <N>             # optional, default 0
//   <motion_path_1>                # exactly N motion paths
//   ...
//   *MATERIAL_NUM  <N>             # optional, default 0
//   <material_path_1>              # exactly N material paths
//   ...
//
// A real sample is test-extract/11160.chr. We pin both the parser and
// a round-trip serializer (serialize_text → parse → byte-equal).

#include "mxh/compat/chr_motion.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat::test {

namespace {

// Build a .chr text payload in-memory.
std::string make_chr_text(
    const std::vector<std::tuple<std::string,
                                  std::vector<std::string>,
                                  std::vector<std::string>>>& sections) {
    std::string out;
    for (const auto& [mod, motions, materials] : sections) {
        out += "*MOD_FILE_NAME\t" + mod + "\n";
        out += "\t*MOTION_NUM\t" + std::to_string(motions.size()) + "\n";
        for (const auto& m : motions) {
            out += "\t\t" + m + "\n";
        }
        out += "\t*MATERIAL_NUM\t" + std::to_string(materials.size()) + "\n";
        for (const auto& mat : materials) {
            out += "\t\t" + mat + "\n";
        }
    }
    return out;
}

std::vector<std::uint8_t> to_bytes(std::string_view s) {
    return std::vector<std::uint8_t>(s.begin(), s.end());
}

// Path to the bundled real-world sample (extracted from the original
// game resources). Created by Phase 7.5p and shipped with the repo.
const std::filesystem::path kRealChr = "test-extract/11160.chr";

}  // namespace

// ===========================================================================
// Empty / invalid input
// ===========================================================================

TEST(ChrModelParseTest, RejectsEmptyBuffer) {
    EXPECT_FALSE(ChrModel::parse(std::span<const std::uint8_t>{}).has_value());
}

TEST(ChrModelParseTest, RejectsWhitespaceOnly) {
    // Whitespace alone produces no tokens → no *MOD_FILE_NAME → no
    // sections → std::nullopt.
    auto bytes = to_bytes("   \n\t\n   \n");
    EXPECT_FALSE(ChrModel::parse(bytes).has_value());
}

TEST(ChrModelParseTest, RejectsLinesWithoutModFileName) {
    // The first recognized token is *MOTION_NUM with no preceding
    // section. Legacy tolerated this (set count to 0 silently), but
    // since no section was ever opened, sections() stays empty and
    // we return nullopt. This is a deliberate divergence — the
    // legacy path would have stored a count on a non-existent
    // section, which we refuse to do.
    auto bytes = to_bytes("*MOTION_NUM 5\nfoo.ANM\n");
    EXPECT_FALSE(ChrModel::parse(bytes).has_value());
}

TEST(ChrModelLoadTest, MissingFileReturnsNullopt) {
    auto m = ChrModel::load("D:/_does_not_exist_/nope.chr");
    EXPECT_FALSE(m.has_value());
}

TEST(ChrModelLoadTest, EmptyFileReturnsNullopt) {
    // Create a zero-byte temp file and confirm load() returns nullopt.
    auto tmp = std::filesystem::temp_directory_path() / "mxh_chr_empty.chr";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        // don't write anything
    }
    auto m = ChrModel::load(tmp);
    EXPECT_FALSE(m.has_value());
    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

// ===========================================================================
// Single section, simple case
// ===========================================================================

TEST(ChrModelParseTest, SingleSectionNoMotionsNoMaterials) {
    auto text = make_chr_text({{"fighter.MOD", {}, {}}});
    auto m = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->sections().size(), 1u);
    EXPECT_STREQ(m->sections()[0].mod_file, "fighter.MOD");
    EXPECT_TRUE(m->sections()[0].motions.empty());
    EXPECT_TRUE(m->sections()[0].materials.empty());
}

TEST(ChrModelParseTest, SingleSectionWithMotions) {
    auto text = make_chr_text({{"fighter.MOD", {"walk.ANM", "run.ANM", "idle.ANM"}, {}}});
    auto m = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->sections().size(), 1u);
    EXPECT_STREQ(m->sections()[0].mod_file, "fighter.MOD");
    ASSERT_EQ(m->sections()[0].motions.size(), 3u);
    EXPECT_EQ(m->sections()[0].motions[0], "walk.ANM");
    EXPECT_EQ(m->sections()[0].motions[1], "run.ANM");
    EXPECT_EQ(m->sections()[0].motions[2], "idle.ANM");
    EXPECT_TRUE(m->sections()[0].materials.empty());
}

TEST(ChrModelParseTest, SingleSectionWithMotionsAndMaterials) {
    auto text = make_chr_text({
        {"11160.MOD", {"11160.ANM"}, {}}
    });
    // Match the real test-extract/11160.chr exactly.
    auto m = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->sections().size(), 1u);
    EXPECT_STREQ(m->sections()[0].mod_file, "11160.MOD");
    ASSERT_EQ(m->sections()[0].motions.size(), 1u);
    EXPECT_EQ(m->sections()[0].motions[0], "11160.ANM");
    EXPECT_TRUE(m->sections()[0].materials.empty());
}

// ===========================================================================
// Multi-section
// ===========================================================================

TEST(ChrModelParseTest, MultiSectionResetsCounters) {
    // Section 1 has 2 motions, section 2 has 0 motions. The counter
    // from section 1 must NOT bleed into section 2's "no motions" path
    // (which would otherwise grab section 2's *MATERIAL_NUM as a
    // motion).
    auto text = make_chr_text({
        {"a.MOD", {"a1.ANM", "a2.ANM"}, {}},
        {"b.MOD", {}, {}}
    });
    auto m = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->sections().size(), 2u);
    EXPECT_STREQ(m->sections()[0].mod_file, "a.MOD");
    EXPECT_EQ(m->sections()[0].motions.size(), 2u);
    EXPECT_TRUE(m->sections()[0].materials.empty());
    EXPECT_STREQ(m->sections()[1].mod_file, "b.MOD");
    EXPECT_TRUE(m->sections()[1].motions.empty());
    EXPECT_TRUE(m->sections()[1].materials.empty());
}

TEST(ChrModelParseTest, MultiSectionKeepsIndependentLists) {
    auto text = make_chr_text({
        {"a.MOD", {"a.ANM"}, {"a.MML"}},
        {"b.MOD", {"b1.ANM", "b2.ANM"}, {"b1.MML", "b2.MML"}}
    });
    auto m = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->sections().size(), 2u);
    EXPECT_STREQ(m->sections()[0].mod_file, "a.MOD");
    EXPECT_EQ(m->sections()[0].motions.size(), 1u);
    EXPECT_EQ(m->sections()[0].motions[0], "a.ANM");
    EXPECT_EQ(m->sections()[0].materials.size(), 1u);
    EXPECT_EQ(m->sections()[0].materials[0], "a.MML");
    EXPECT_STREQ(m->sections()[1].mod_file, "b.MOD");
    EXPECT_EQ(m->sections()[1].motions.size(), 2u);
    EXPECT_EQ(m->sections()[1].materials.size(), 2u);
}

// ===========================================================================
// Tolerance
// ===========================================================================

TEST(ChrModelParseTest, SkipsBlankAndCommentLines) {
    auto text = std::string("\n") +
        "// pre-comment\n" +
        "\n" +
        "*MOD_FILE_NAME x.MOD\n" +
        "   \n" +
        "\t*MOTION_NUM\t1\n" +
        "// another comment\n" +
        "walk.ANM\n" +
        "\t*MATERIAL_NUM\t0\n";
    auto m = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->sections().size(), 1u);
    EXPECT_STREQ(m->sections()[0].mod_file, "x.MOD");
    EXPECT_EQ(m->sections()[0].motions.size(), 1u);
    EXPECT_EQ(m->sections()[0].motions[0], "walk.ANM");
}

TEST(ChrModelParseTest, NegativeMotionCountClampedToZero) {
    auto text = std::string(
        "*MOD_FILE_NAME x.MOD\n"
        "*MOTION_NUM -1\n"
        "this.ANM\n"        // should be dropped
        "*MATERIAL_NUM 1\n"
        "this.MML\n");
    auto m = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    EXPECT_TRUE(m->sections()[0].motions.empty());
    EXPECT_EQ(m->sections()[0].materials.size(), 1u);
    EXPECT_EQ(m->sections()[0].materials[0], "this.MML");
}

TEST(ChrModelParseTest, StrayPathTokenBeforeAnySectionIsDropped) {
    auto text = std::string("orphan.ANM\n*MOD_FILE_NAME x.MOD\n");
    auto m = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->sections().size(), 1u);
    EXPECT_STREQ(m->sections()[0].mod_file, "x.MOD");
    EXPECT_TRUE(m->sections()[0].motions.empty());
}

TEST(ChrModelParseTest, UnknownPidTokenIsIgnored) {
    // *FOO_BAR is not a recognized PID. The line should be dropped
    // and parsing should continue normally.
    auto text = std::string(
        "*FOO_BAR 1\n"
        "*MOD_FILE_NAME x.MOD\n"
        "*MOTION_NUM 1\n"
        "walk.ANM\n");
    auto m = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->sections().size(), 1u);
    EXPECT_STREQ(m->sections()[0].mod_file, "x.MOD");
    EXPECT_EQ(m->sections()[0].motions.size(), 1u);
    EXPECT_EQ(m->sections()[0].motions[0], "walk.ANM");
}

// ===========================================================================
// Round-trip
// ===========================================================================

TEST(ChrModelSerializeTest, EmptySectionsProducesEmptyString) {
    EXPECT_EQ(ChrModel::serialize_text({}), std::string{});
}

TEST(ChrModelRoundTripTest, SynthesizedMultiSection) {
    std::vector<ChrModelSection> in;
    {
        ChrModelSection a{};
        std::strcpy(a.mod_file, "hero.MOD");
        a.motions = {"hero_idle.ANM", "hero_run.ANM"};
        a.materials = {"hero_skin.MML"};
        in.push_back(std::move(a));
    }
    {
        ChrModelSection b{};
        std::strcpy(b.mod_file, "weapon.MOD");
        b.motions = {"weapon_swing.ANM"};
        b.materials = {};
        in.push_back(std::move(b));
    }
    const std::string text = ChrModel::serialize_text(in);
    auto parsed = ChrModel::parse(to_bytes(text));
    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->sections().size(), in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        EXPECT_STREQ(parsed->sections()[i].mod_file, in[i].mod_file);
        EXPECT_EQ(parsed->sections()[i].motions, in[i].motions);
        EXPECT_EQ(parsed->sections()[i].materials, in[i].materials);
    }
}

TEST(ChrModelSaveLoadFileTest, RoundTripThroughDisk) {
    auto tmp = std::filesystem::temp_directory_path() / "mxh_chr_roundtrip.chr";
    std::vector<ChrModelSection> in;
    ChrModelSection s{};
    std::strcpy(s.mod_file, "test.MOD");
    s.motions = {"test.ANM"};
    s.materials = {"test.MML"};
    in.push_back(std::move(s));

    ASSERT_TRUE(ChrModel::save_to_file(tmp, in));
    auto loaded = ChrModel::load(tmp);
    ASSERT_TRUE(loaded.has_value());
    ASSERT_EQ(loaded->sections().size(), 1u);
    EXPECT_STREQ(loaded->sections()[0].mod_file, "test.MOD");
    EXPECT_EQ(loaded->sections()[0].motions.size(), 1u);
    EXPECT_EQ(loaded->sections()[0].motions[0], "test.ANM");
    EXPECT_EQ(loaded->sections()[0].materials.size(), 1u);
    EXPECT_EQ(loaded->sections()[0].materials[0], "test.MML");

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

// ===========================================================================
// Real-world sample
// ===========================================================================

TEST(ChrModelRealSampleTest, LoadsTestExtract11160Chr) {
    if (!std::filesystem::exists(kRealChr)) {
        GTEST_SKIP() << "test-extract/11160.chr not present; skipping real sample";
    }
    auto m = ChrModel::load(kRealChr);
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->sections().size(), 1u);
    EXPECT_STREQ(m->sections()[0].mod_file, "11160.MOD");
    ASSERT_EQ(m->sections()[0].motions.size(), 1u);
    EXPECT_EQ(m->sections()[0].motions[0], "11160.ANM");
    EXPECT_TRUE(m->sections()[0].materials.empty());
}

}  // namespace mxh::compat::test
