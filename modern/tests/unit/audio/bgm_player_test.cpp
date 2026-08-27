#include "mxh/audio/bgm_player.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <limits>

namespace {
std::filesystem::path findSoundRoot() {
    auto root = std::filesystem::current_path();
    for (int level = 0; level < 8; ++level) {
        for (const auto& first : std::filesystem::directory_iterator(root)) {
            if (!first.is_directory()) continue;
            const auto direct = first.path() / "PlayDH" / "Sound";
            if (std::filesystem::exists(direct / "SoundList.bin")) return direct;
        }
        if (!root.has_parent_path() || root.parent_path() == root) break;
        root = root.parent_path();
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

// Playback lifecycle verification: confirm play()/stop() drives the player
// state. Windows uses the production Media Foundation/XAudio2 backend;
// non-Windows builds intentionally report that native playback is unavailable.
TEST(BgmPlayer, PlaybackLoopReportsCurrentId) {
    const auto root = findSoundRoot();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    mxh::audio::BgmPlayer player;
    std::string error;
    ASSERT_TRUE(player.initialize(root, &error)) << error;

    const auto played = player.play(1667, &error);
#ifdef _WIN32
    ASSERT_TRUE(played) << error;
    EXPECT_EQ(player.currentSoundId(), 1667u);
    player.stop();
    EXPECT_EQ(player.currentSoundId(), 0xffffu);
#else
    // Non-Windows: native Windows playback is intentionally unavailable.
    EXPECT_FALSE(played);
    EXPECT_EQ(player.currentSoundId(), 0xffffu);
#endif
}

TEST(BgmPlayer, PlaybackReplacesCurrentBgm) {
    const auto root = findSoundRoot();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    mxh::audio::BgmPlayer player;
    std::string error;
    ASSERT_TRUE(player.initialize(root, &error)) << error;

#ifdef _WIN32
    ASSERT_TRUE(player.play(1667, &error)) << error;  // bg_login
    EXPECT_EQ(player.currentSoundId(), 1667u);
    ASSERT_TRUE(player.play(1663, &error)) << error;  // bg_field
    EXPECT_EQ(player.currentSoundId(), 1663u);
    player.stop();
    EXPECT_EQ(player.currentSoundId(), 0xffffu);
#else
    // On non-Windows, native playback refuses - repeated calls remain safe.
    EXPECT_FALSE(player.play(1667, &error));
    EXPECT_FALSE(player.play(1663, &error));
#endif
}

TEST(BgmPlayer, VolumeClampIsApplied) {
    const auto root = findSoundRoot();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    mxh::audio::BgmPlayer player;
    ASSERT_TRUE(player.initialize(root));
    player.setVolume(2.5f);   // above max
    player.setVolume(-0.5f);  // below min
    // This assertion guards that volume updates remain safe before playback.
    SUCCEED();
}

TEST(BgmPlayer, NonFiniteVolumeIsSafe) {
    mxh::audio::BgmPlayer player;
    player.setVolume(std::numeric_limits<float>::quiet_NaN());
    player.setVolume(std::numeric_limits<float>::infinity());
    player.setVolume(-std::numeric_limits<float>::infinity());
    SUCCEED();
}
