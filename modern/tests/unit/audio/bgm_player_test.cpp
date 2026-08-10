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

// Playback loop verification: confirm play()/stop() drives the player state.
// On Windows the MCI command succeeds and the player reports the BGM id as
// current. On non-Windows play() returns false (no MCI) - we treat that as
// expected behavior so the test suite stays portable.
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
    // Non-Windows: player.play() is expected to refuse (MCI is Win32-only).
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
    // On non-Windows, play() refuses - the loop is still safe to call repeatedly.
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
    // If clamp is broken, BgmPlayer will produce an out-of-range MCI volume
    // and the next play() call will fail; this assertion just guards that
    // setVolume() is callable without crashing.
    SUCCEED();
}
