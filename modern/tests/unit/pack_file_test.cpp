// Tests for mxh::compat::PackFile - .pak format.

#include "mxh/compat/pack_file.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include <windows.h>  // for GetModuleFileNameW to find resources relative to exe

using namespace mxh::compat;

namespace {

// Returns the absolute path to the "墨香【源码配套资源】\PlayDH" directory,
// locating the workspace by walking up from the test executable's path.
std::filesystem::path find_playdh_root() {
    wchar_t buf[MAX_PATH];
    HMODULE h = GetModuleHandleW(nullptr);
    DWORD n = GetModuleFileNameW(h, buf, MAX_PATH);
    if (n == 0) return {};

    std::filesystem::path p(buf);
    // Walk up looking for "墨香【源码配套资源】" sibling dir.
    for (int i = 0; i < 6; ++i) {
        p = p.parent_path();
        if (p.empty()) break;
        auto candidate = p / L"墨香【源码配套资源】" / L"PlayDH";
        if (std::filesystem::exists(candidate)) return candidate;
    }
    return {};
}

const std::filesystem::path& playdh_root() {
    static const auto r = find_playdh_root();
    return r;
}

}  // namespace

TEST(PackFile, FindCaseInsensitive) {
    if (playdh_root().empty()) {
        GTEST_SKIP() << "PlayDH root not found";
    }
    auto path = playdh_root() / "Effect.pak";
    auto pack = PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    EXPECT_NE(pack->find("TITAN_PORTAL_EFF.MOD"), nullptr);
    EXPECT_NE(pack->find("titan_portal_eff.mod"), nullptr);
    EXPECT_EQ(pack->find("missing.mod"), nullptr);
}

TEST(PackFile, RejectsNonPakBlob) {
    std::vector<std::uint8_t> garbage = {0x01, 0x02, 0x03};
    EXPECT_FALSE(PackFile::is_pak(garbage));
}

TEST(PackFile, ParsesRealEffectPak) {
    if (playdh_root().empty()) {
        GTEST_SKIP() << "PlayDH root not found";
    }
    auto path = playdh_root() / "Effect.pak";
    auto pack = PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    EXPECT_EQ(pack->file_count(), 1671u);
    EXPECT_GT(pack->parsed_count(), 1000u);
}

TEST(PackFile, ParsesAllRealPakFiles) {
    if (playdh_root().empty()) {
        GTEST_SKIP() << "PlayDH root not found";
    }
    const char* files[] = {
        "Effect.pak", "Character.pak", "Map.pak", "monster.pak",
        "npc.pak", "Pet.pak", "Titan.pak"
    };
    int parsed_ok = 0;
    int total_entries = 0;
    for (auto f : files) {
        auto p = playdh_root() / f;
        if (!std::filesystem::exists(p)) continue;
        auto pack = PackFile::open(p);
        ASSERT_NE(pack, nullptr) << "Failed to open " << f;
        EXPECT_GT(pack->parsed_count(), 0u) << f << " had 0 entries parsed";
        parsed_ok++;
        total_entries += static_cast<int>(pack->parsed_count());
    }
    EXPECT_EQ(parsed_ok, 7) << "Should have parsed all 7 real .pak files";
    EXPECT_GT(total_entries, 10000) << "Expected many entries total";
}

TEST(PackFile, ReadRealEntryData) {
    if (playdh_root().empty()) {
        GTEST_SKIP() << "PlayDH root not found";
    }
    auto path = playdh_root() / "Effect.pak";
    auto pack = PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    auto data = pack->read("titan_portal_eff.mod");
    EXPECT_GT(data.size(), 100u) << "titan_portal_eff.mod should be a real file";
}
