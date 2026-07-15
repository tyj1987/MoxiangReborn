// chx_model_test.cpp - .chx character manifest parser tests (Phase 10.22 / 12.1)
//
// Covers modern/include/mxh/compat/chx_model.hpp + src/chx_model.cpp.
//
// The legacy 4Dyuchi .chx format is plain TEXT (tab-separated). It
// is NOT the binary "CHLX" header format the original skeleton
// stub assumed. Real format (verified against
// test-extract/Character.pak:man.chx):
//
//   *MOD_FILE_NUM    <N>
//   *MOD_FILE_NAME   <path_1>
//   *MOD_FILE_NAME   <path_2>
//   ...
//   *MOTION_NUM      <M>
//   <motion_path_1>
//   ...

#include "mxh/compat/chx_model.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat::test {

namespace {

std::string make_chx_text(
    const std::vector<std::string>& mod_files,
    const std::vector<std::string>& motions) {
    std::string out;
    out += "*MOD_FILE_NUM\t" + std::to_string(mod_files.size()) + "\n";
    for (const auto& mf : mod_files) {
        out += "*MOD_FILE_NAME\t" + mf + "\n";
    }
    out += "*MOTION_NUM\t" + std::to_string(motions.size()) + "\n";
    for (const auto& mv : motions) {
        out += mv + "\n";
    }
    return out;
}

std::vector<std::uint8_t> to_bytes(std::string_view s) {
    return std::vector<std::uint8_t>(s.begin(), s.end());
}

}  // namespace

// ===========================================================================
// Empty / invalid input
// ===========================================================================

TEST(ChxModelParseTest, RejectsEmptyBuffer) {
    EXPECT_FALSE(ChxModel::parse(std::span<const std::uint8_t>{}).has_value());
}

TEST(ChxModelParseTest, RejectsWhitespaceOnly) {
    auto bytes = to_bytes("   \n\t\n  \n");
    EXPECT_FALSE(ChxModel::parse(bytes).has_value());
}

TEST(ChxModelParseTest, RejectsModFileNumOnlyNoNames) {
    // *MOD_FILE_NUM 5 but no following *MOD_FILE_NAME lines.
    // mod_files stays empty → nullopt.
    auto bytes = to_bytes("*MOD_FILE_NUM\t5\n");
    EXPECT_FALSE(ChxModel::parse(bytes).has_value());
}

TEST(ChxModelLoadTest, MissingFileReturnsNullopt) {
    auto m = ChxModel::load("D:/_does_not_exist_/nope.chx");
    EXPECT_FALSE(m.has_value());
}

TEST(ChxModelLoadTest, EmptyFileReturnsNullopt) {
    auto tmp = std::filesystem::temp_directory_path() / "mxh_chx_empty.chx";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    }
    auto m = ChxModel::load(tmp);
    EXPECT_FALSE(m.has_value());
    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

// ===========================================================================
// Simple cases
// ===========================================================================

TEST(ChxModelParseTest, SingleModFileNoMotions) {
    auto text = make_chx_text({"M_BODY01.MOD"}, {});
    auto m = ChxModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->mod_files.size(), 1u);
    EXPECT_EQ(m->mod_files[0], "M_BODY01.MOD");
    EXPECT_TRUE(m->motions.empty());
}

TEST(ChxModelParseTest, MultiModFileNoMotions) {
    auto text = make_chx_text(
        {"M_HAIR01.MOD", "M_BODY01.MOD", "M_PANTS01.MOD", "M_BOOTS01.MOD", "M_HAND01.MOD"},
        {});
    auto m = ChxModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->mod_files.size(), 5u);
    EXPECT_EQ(m->mod_files[0], "M_HAIR01.MOD");
    EXPECT_EQ(m->mod_files[4], "M_HAND01.MOD");
    EXPECT_TRUE(m->motions.empty());
}

TEST(ChxModelParseTest, ModFilesWithMotions) {
    auto text = make_chx_text(
        {"A.MOD", "B.MOD"},
        {"idle.ANM", "walk.ANM", "run.ANM"});
    auto m = ChxModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->mod_files.size(), 2u);
    EXPECT_EQ(m->mod_files[0], "A.MOD");
    EXPECT_EQ(m->mod_files[1], "B.MOD");
    ASSERT_EQ(m->motions.size(), 3u);
    EXPECT_EQ(m->motions[0], "idle.ANM");
    EXPECT_EQ(m->motions[1], "walk.ANM");
    EXPECT_EQ(m->motions[2], "run.ANM");
}

// ===========================================================================
// Tolerance
// ===========================================================================

TEST(ChxModelParseTest, SkipsBlankAndCommentLines) {
    auto text = std::string("\n") +
        "// pre-comment\n" +
        "*MOD_FILE_NUM\t1\n" +
        "   \n" +
        "// another comment\n" +
        "*MOD_FILE_NAME x.MOD\n" +
        "\t*MOTION_NUM\t0\n";
    auto m = ChxModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->mod_files.size(), 1u);
    EXPECT_EQ(m->mod_files[0], "x.MOD");
    EXPECT_TRUE(m->motions.empty());
}

TEST(ChxModelParseTest, NegativeCountsClampedToZero) {
    auto text = std::string(
        "*MOD_FILE_NUM\t-1\n"
        "*MOD_FILE_NAME\tstray.MOD\n"  // count was 0, this is dropped
        "*MOTION_NUM\t-3\n"
        "stray.ANM\n");                // count was 0, this is dropped
    auto m = ChxModel::parse(to_bytes(text));
    EXPECT_FALSE(m.has_value()) << "no mod files should land → nullopt";
}

TEST(ChxModelParseTest, UnknownPidTokenIgnored) {
    // *FOO_BAR is unknown, line is dropped, parsing continues.
    auto text = std::string(
        "*FOO_BAR 1\n"
        "*MOD_FILE_NUM 1\n"
        "*MOD_FILE_NAME x.MOD\n"
        "*MOTION_NUM 0\n");
    auto m = ChxModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->mod_files.size(), 1u);
    EXPECT_EQ(m->mod_files[0], "x.MOD");
}

TEST(ChxModelParseTest, ModFileNameWithoutCountHeaderTreatedAsOne) {
    // No leading *MOD_FILE_NUM but a *MOD_FILE_NAME appears. We
    // treat this as a count of 1 and accept the file (defensive
    // — some hand-edited resources skip the count header).
    auto text = std::string(
        "*MOD_FILE_NAME lonesome.MOD\n"
        "*MOTION_NUM 0\n");
    auto m = ChxModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->mod_files.size(), 1u);
    EXPECT_EQ(m->mod_files[0], "lonesome.MOD");
}

// ===========================================================================
// Round-trip
// ===========================================================================

TEST(ChxModelSerializeTest, EmptyProducesJustHeaders) {
    // serialize_text with empty mod_files still writes the
    // *MOD_FILE_NUM 0 line so the file is parseable.
    auto text = ChxModel::serialize_text({}, {});
    EXPECT_EQ(text, "*MOD_FILE_NUM\t0\n*MOTION_NUM\t0\n");
}

TEST(ChxModelRoundTripTest, SynthesizedManChx) {
    std::vector<std::string> mods = {
        "M_HAIR01.MOD", "M_BODY01.MOD", "M_PANTS01.MOD",
        "M_BOOTS01.MOD", "M_HAND01.MOD"
    };
    std::vector<std::string> motions = {"man_idle.ANM", "man_walk.ANM"};
    const std::string text = ChxModel::serialize_text(mods, motions);
    auto m = ChxModel::parse(to_bytes(text));
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->mod_files, mods);
    EXPECT_EQ(m->motions, motions);
}

TEST(ChxModelSaveLoadFileTest, RoundTripThroughDisk) {
    auto tmp = std::filesystem::temp_directory_path() / "mxh_chx_roundtrip.chx";
    std::vector<std::string> mods = {"A.MOD", "B.MOD"};
    std::vector<std::string> motions = {"x.ANM"};

    ASSERT_TRUE(ChxModel::save_to_file(tmp, mods, motions));
    auto loaded = ChxModel::load(tmp);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->mod_files, mods);
    EXPECT_EQ(loaded->motions, motions);

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

}  // namespace mxh::compat::test
