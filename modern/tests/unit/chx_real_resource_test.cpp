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

const std::filesystem::path kCharacterPak = LR"(D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码配套资源】\PlayDH\Character.pak)";
const std::filesystem::path kMonsterBin   = LR"(D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码配套资源】\PlayDH\Resource\MonsterList.bin)";

}  // namespace

TEST(ChxModelRealResource, ManChxIsTextMetadata) {
    // Read a real .chx from Character.pak.
    if (!std::filesystem::exists(kCharacterPak)) {
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

    // ChxModel::parse currently rejects it (because its loose magic check
    // requires a uint32 version in [1,10]). That's the documented limitation.
    auto m = ChxModel::parse(bytes);
    EXPECT_FALSE(m.header.vertex_count > 0 || m.header.index_count > 0)
        << "Parser accidentally accepted a text file as binary";
}

TEST(ChxModelRealResource, MonsterBinSmoke) {
    // Sanity: the compat layer can read MonsterList.bin.
    if (!std::filesystem::exists(kMonsterBin)) {
        GTEST_SKIP() << "MonsterList.bin not found";
    }
    auto result = read_mh_bin(kMonsterBin);
    ASSERT_TRUE(result.ok()) << "read_mh_bin failed with error "
                              << static_cast<int>(result.error);
    EXPECT_GT(result.value.data.size(), static_cast<size_t>(1000u))
        << "MonsterList.bin suspiciously small";
}

TEST(ChxModelRealResource, CharacterPakFileCount) {
    if (!std::filesystem::exists(kCharacterPak)) {
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
    if (!std::filesystem::exists(kCharacterPak)) {
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

    auto m = ChxModel::parse(bytes);
    // Text header should NOT be accepted as binary
    EXPECT_EQ(m.header.vertex_count, static_cast<std::uint32_t>(0));
    EXPECT_EQ(m.header.index_count, static_cast<std::uint32_t>(0));
}
