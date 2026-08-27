#include "mxh/audio/sfx_player.hpp"
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

TEST(SfxPlayer, ResolvesNonStreamingWavAndRejectsBgm) {
    const auto root = findSoundRoot();
    if (root.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    mxh::audio::SfxPlayer player;
    std::string error;
    ASSERT_TRUE(player.initialize(root, &error)) << error;
    std::uint16_t wav = 0xffffu;
    for (const auto& entry : player.manifest().entries) {
        if (entry.available && !entry.streaming) { wav = entry.index; break; }
    }
    ASSERT_NE(wav, 0xffffu);
    EXPECT_TRUE(player.resolve(1667).empty()); // streaming BGM is not an SFX entry
    EXPECT_FALSE(player.resolve(wav).empty());
}

TEST(SfxPlayer, MissingSoundFailsClosed) {
    mxh::audio::SfxPlayer player;
    std::string error;
    EXPECT_FALSE(player.play(1, &error));
    EXPECT_FALSE(error.empty());
}

TEST(SfxPlayer, DistanceGainHonorsSoundListRange) {
    EXPECT_FLOAT_EQ(mxh::audio::SfxPlayer::distanceGain(0.0f, 10.0f, 100.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::SfxPlayer::distanceGain(100.0f, 10.0f, 100.0f), 0.0f);
    EXPECT_NEAR(mxh::audio::SfxPlayer::distanceGain(55.0f, 10.0f, 100.0f), 0.5f, 0.001f);
}

TEST(SfxPlayer, DistanceGainRejectsNonFiniteInputs) {
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto inf = std::numeric_limits<float>::infinity();
    EXPECT_FLOAT_EQ(mxh::audio::SfxPlayer::distanceGain(nan, 10.0f, 100.0f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::SfxPlayer::distanceGain(10.0f, inf, 100.0f), 0.0f);
}

TEST(SfxPlayer, SetVolumeFallsBackForNonFiniteRuntimeValues) {
    mxh::audio::SfxPlayer player;
    player.setVolume(0.25f);
    EXPECT_FLOAT_EQ(player.volume(), 0.25f);

    player.setVolume(std::numeric_limits<float>::quiet_NaN());
    EXPECT_FLOAT_EQ(player.volume(), 1.0f);
    player.setVolume(std::numeric_limits<float>::infinity());
    EXPECT_FLOAT_EQ(player.volume(), 1.0f);
    player.setVolume(-std::numeric_limits<float>::infinity());
    EXPECT_FLOAT_EQ(player.volume(), 1.0f);
    EXPECT_FLOAT_EQ(player.currentGain(), 1.0f);
}
