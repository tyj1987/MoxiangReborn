#include "mxh/audio/bgm_player.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace {
std::filesystem::path findSoundRoot() {
    for (const auto& first : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        if (!first.is_directory()) continue;
        const auto direct = first.path() / "PlayDH" / "Sound";
        if (std::filesystem::exists(direct / "SoundList.bin")) return direct;
    }
    return {};
}
}

TEST(BgmPlayer, ResolvesOriginalLoginMusic) {
    const auto root = findSoundRoot();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    mxh::audio::BgmPlayer player;
    std::string error;
    ASSERT_TRUE(player.initialize(root, &error)) << error;
    EXPECT_EQ(player.manifest().entries.size(), 1674u);
    const auto login = player.resolve(1667);
    ASSERT_FALSE(login.empty());
    EXPECT_EQ(login.filename(), "bg_login.mp3");
}

TEST(BgmPlayer, RejectsEffectAndNullSlotsAsBgm) {
    const auto root = findSoundRoot();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    mxh::audio::BgmPlayer player;
    ASSERT_TRUE(player.initialize(root));
    EXPECT_TRUE(player.resolve(0).empty());
    EXPECT_TRUE(player.resolve(1).empty());
}
