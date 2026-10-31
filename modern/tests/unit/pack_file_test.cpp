// Tests for mxh::compat::PackFile - .pak format.

#include "mxh/compat/pack_file.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <windows.h>  // for GetModuleFileNameW to find resources relative to exe
#include "../support/playdh_env.hpp"

using namespace mxh::compat;

namespace {

// Returns the canonical modern/data/PlayDH directory,
// locating the workspace by walking up from the test executable's path.
std::filesystem::path find_playdh_root() {
    if (const auto playdh_override = mxh::test::playdh_dir_override()) return *playdh_override;
    wchar_t buf[MAX_PATH];
    HMODULE h = GetModuleHandleW(nullptr);
    DWORD n = GetModuleFileNameW(h, buf, MAX_PATH);
    if (n == 0) return {};

    std::filesystem::path p(buf);
    // Walk up looking for the canonical modern resource tree.
    for (int i = 0; i < 6; ++i) {
        p = p.parent_path();
        if (p.empty()) break;
        auto candidate = p / L"modern" / L"data" / L"PlayDH";
        if (std::filesystem::exists(candidate)) return candidate;
    }
    return {};
}

const std::filesystem::path& playdh_root() {
    static const auto r = find_playdh_root();
    return r;
}

bool is_lfs_pointer_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    char buf[64] = {};
    in.read(buf, 63);
    const auto n = static_cast<std::size_t>(in.gcount());
    constexpr std::string_view kPrefix = "version https://git-lfs.github.com/spec/v1";
    return n >= kPrefix.size() &&
           std::string_view(buf, kPrefix.size()) == kPrefix;
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

// Real archives are read only on ROG from D:\MX. This case is not a
// required pass: a skip is not a pass. Synthetic coverage is
// PackFileHeader.SyntheticNpcPetTitanArchivesRoundTrip.
TEST(PackFile, ParsesAllRealPakFiles) {
    const char* files[] = {
        "Effect.pak", "Character.pak", "Map.pak", "monster.pak",
        "npc.pak", "Pet.pak", "Titan.pak"
    };
    const auto root = playdh_root();
    bool real_ready = !root.empty();
    if (real_ready) {
        for (auto f : files) {
            const auto path = root / f;
            std::error_code ec;
            if (!std::filesystem::is_regular_file(path, ec) || is_lfs_pointer_file(path)) {
                real_ready = false;
                break;
            }
        }
    }
    if (!real_ready) {
        for (auto f : files) {
            std::cout << "SKIP real " << f << " absent: ROG-only\n";
        }
        GTEST_SKIP() << "SKIP real pak archives absent: ROG-only";
    }
    int parsed_ok = 0;
    int total_entries = 0;
    for (auto f : files) {
        auto pack = PackFile::open(root / f);
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
