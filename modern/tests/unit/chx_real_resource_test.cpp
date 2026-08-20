// Tests for mxh::compat::ChxModel real-resource behavior.
//
// NOTE: The .chx files in this game are NOT the binary mesh format the parser
// originally assumed (CHLX magic). They are TAB-SEPARATED TEXT metadata files
// that list the .mod model parts and the motion count for a character:
//
//   *MOD_FILE_NUM	5
//   *MOD_FILE_NAME	M_HAIR01.MOD
//   *MOD_FILE_NAME	M_BODY01.MOD
//   ...
//   *MOTION_NUM		466
//
// These tests document the actual format and make sure the parser does not
// crash or misreport on real files.

#include "mxh/compat/pack_file.hpp"
#include "mxh/compat/chx_model.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace mxh::compat;

namespace {

// Walks up to 8 levels from cwd looking for a sibling dir whose PlayDH/ subdir
// contains Character.pak. Returns the resolved Character.pak path (or empty
// if not found). Pattern mirrors findMapPack in hfl_height_field_test.cpp so
// the same scratch symlink layout resolves everywhere.
std::filesystem::path find_character_pak() {
    auto root = std::filesystem::current_path();
    for (int level = 0; level < 8; ++level) {
        for (const auto& first : std::filesystem::directory_iterator(root)) {
            if (!first.is_directory()) continue;
            const auto candidate = first.path() / "PlayDH" / "Character.pak";
            if (std::filesystem::exists(candidate)) return candidate;
        }
        if (!root.has_parent_path() || root.parent_path() == root) break;
        root = root.parent_path();
    }
    return {};
}

std::filesystem::path find_monster_list_bin() {
    auto root = std::filesystem::current_path();
    for (int level = 0; level < 8; ++level) {
        for (const auto& first : std::filesystem::directory_iterator(root)) {
            if (!first.is_directory()) continue;
            const auto candidate = first.path() / "PlayDH" / "Resource" / "MonsterList.bin";
            if (std::filesystem::exists(candidate)) return candidate;
        }
        if (!root.has_parent_path() || root.parent_path() == root) break;
        root = root.parent_path();
    }
    return {};
}

}  // namespace

TEST(ChxModelRealResource, ManChxIsTextMetadata) {
    // Read a real .chx from Character.pak.
    const auto kCharacterPak = find_character_pak();
    if (kCharacterPak.empty()) {
        GTEST_SKIP() << "Character.pak not found";
    }
    auto pack = PackFile::open(kCharacterPak);
    ASSERT_NE(pack, nullptr);

    auto bytes = pack->read("man.chx");
    ASSERT_FALSE(bytes.empty()) << "man.chx not found in pack";

// The file starts with the text magic "*MOD_FILE_NUM".
    // Read up to the first newline (handles both \n and \r\n line endings).
    const char* data = reinterpret_cast<const char*>(bytes.data());
    const auto end = std::find(data, data + bytes.size(), '\n');
    const auto len = static_cast<std::size_t>(end - data);
    std::string firstLine(data, len);
    // Strip the trailing \r if present (CRLF case).
    if (!firstLine.empty() && firstLine.back() == '\r') firstLine.pop_back();
    EXPECT_EQ(firstLine, "*MOD_FILE_NUM\t5") << "Unexpected text header";

    // ChxModel::parse is now a plain-text parser, so it should
    // accept the file and recover 5 mod files. (This used to
    // assert the opposite under the old binary-header skeleton.)
    auto m = ChxModel::parse(bytes);
    ASSERT_TRUE(m.has_value()) << "parser rejected a real .chx text file";
    EXPECT_EQ(m->mod_files.size(), 5u)
        << "man.chx is expected to have 5 *MOD_FILE_NAME entries";
}

TEST(ChxModelRealResource, MonsterBinSmoke) {
    // Sanity: the compat layer can read MonsterList.bin.
    const auto kMonsterBin = find_monster_list_bin();
    if (kMonsterBin.empty()) {
        GTEST_SKIP() << "MonsterList.bin not found";
    }
    auto result = read_mh_bin(kMonsterBin);
    ASSERT_TRUE(result.ok()) << "read_mh_bin failed with error "
                              << static_cast<int>(result.error);
    EXPECT_GT(result.value.data.size(), static_cast<size_t>(1000u))
        << "MonsterList.bin suspiciously small";
}

TEST(ChxModelRealResource, CharacterPakFileCount) {
    const auto kCharacterPak = find_character_pak();
    if (kCharacterPak.empty()) {
        GTEST_SKIP() << "Character.pak not found";
    }
    auto pack = PackFile::open(kCharacterPak);
    ASSERT_NE(pack, nullptr);
    EXPECT_GT(pack->file_count(), static_cast<std::uint32_t>(0))
        << "Character.pak appears empty";
    auto names = pack->list_names();
    EXPECT_GT(names.size(), static_cast<size_t>(0))
        << "Character.pak has no file entries";
}

TEST(ChxModelRealResource, ChxModelParseRejectsTextChx) {
    const auto kCharacterPak = find_character_pak();
    if (kCharacterPak.empty()) {
        GTEST_SKIP() << "Character.pak not found";
    }
    auto pack = PackFile::open(kCharacterPak);
    ASSERT_NE(pack, nullptr);
    auto bytes = pack->read("man.chx");
    if (bytes.empty()) {
        GTEST_SKIP() << "man.chx not found in pack";
    }

    // Verify it's text before we call parse
    bool startsWithStar = (!bytes.empty() && bytes[0] == '*');
    EXPECT_TRUE(startsWithStar) << "man.chx doesn't look like expected text format";

    // The new text parser should accept the file and recover 5
    // mod_file entries (man.chx is documented as 5-part character).
    auto m = ChxModel::parse(bytes);
    ASSERT_TRUE(m.has_value()) << "parser rejected a real .chx text file";
    EXPECT_EQ(m->mod_files.size(), 5u);
}
