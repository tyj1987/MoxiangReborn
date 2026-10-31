#include "mxh/audio/bgm_player.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <string>
#include "../../support/rog_playdh.hpp"
#include "../../support/synthetic_fixture.hpp"

namespace {

const std::string kHandWrittenSoundList =
    "3\r\n"
    "0 fx/tap.wav 0 0 2 90 1\r\n"
    "1 NULL.WAV 0 0 0 0 0\r\n"
    "2 bgm/theme.mp3 1 1 12 600 0.5\r\n";

class HandWrittenSoundRoot {
public:
    HandWrittenSoundRoot() {
        const auto list = dir_.write_bytes("SoundList.bin", "x");
        const auto encoded = mxh::compat::write_mh_bin(
            list, {reinterpret_cast<const std::uint8_t*>(kHandWrittenSoundList.data()),
                   kHandWrittenSoundList.size()});
        ok_ = dir_.created() && !list.empty() && encoded == mxh::compat::MhError::Ok &&
              !dir_.write_bytes("fx/tap.wav", "RIFF").empty() &&
              !dir_.write_bytes("bgm/theme.mp3", "ID3").empty();
    }

    bool ok() const { return ok_; }
    const std::filesystem::path& path() const { return dir_.path(); }

private:
    mxh::test::SyntheticFixtureDir dir_;
    bool ok_ = false;
};

}  // namespace

TEST(BgmPlayer, ResolvesOriginalLoginMusic) {
    const auto root = mxh::test::require_rog_playdh_root() / "Sound";
    mxh::audio::BgmPlayer player;
    std::string error;
    ASSERT_TRUE(player.initialize(root, &error)) << error;
    EXPECT_EQ(player.manifest().entries.size(), 1674u);
    const auto login = player.resolve(1667);
    ASSERT_FALSE(login.empty());
    EXPECT_EQ(login.filename(), "bg_login.mp3");
}

TEST(BgmPlayer, RejectsEffectAndNullSlotsAsBgm) {
    HandWrittenSoundRoot sound;
    ASSERT_TRUE(sound.ok());
    mxh::audio::BgmPlayer player;
    ASSERT_TRUE(player.initialize(sound.path()));
    EXPECT_TRUE(player.resolve(0).empty());
    EXPECT_TRUE(player.resolve(1).empty());
    const auto theme = player.resolve(2);
    ASSERT_FALSE(theme.empty());
    EXPECT_EQ(theme.filename(), "theme.mp3");
}

// Playback lifecycle verification: confirm play()/stop() drives the player
// state. Windows uses the production Media Foundation/XAudio2 backend;
// non-Windows builds intentionally report that native playback is unavailable.
TEST(BgmPlayer, PlaybackLoopReportsCurrentId) {
    const auto root = mxh::test::require_rog_playdh_root() / "Sound";
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
    EXPECT_FALSE(played);
    EXPECT_EQ(player.currentSoundId(), 0xffffu);
#endif
}

TEST(BgmPlayer, PlaybackReplacesCurrentBgm) {
    const auto root = mxh::test::require_rog_playdh_root() / "Sound";
    mxh::audio::BgmPlayer player;
    std::string error;
    ASSERT_TRUE(player.initialize(root, &error)) << error;

#ifdef _WIN32
    ASSERT_TRUE(player.play(1667, &error)) << error;
    EXPECT_EQ(player.currentSoundId(), 1667u);
    ASSERT_TRUE(player.play(1663, &error)) << error;
    EXPECT_EQ(player.currentSoundId(), 1663u);
    player.stop();
    EXPECT_EQ(player.currentSoundId(), 0xffffu);
#else
    EXPECT_FALSE(player.play(1667, &error));
    EXPECT_FALSE(player.play(1663, &error));
#endif
}

TEST(BgmPlayer, VolumeClampIsApplied) {
    HandWrittenSoundRoot sound;
    ASSERT_TRUE(sound.ok());
    mxh::audio::BgmPlayer player;
    ASSERT_TRUE(player.initialize(sound.path()));
    player.setVolume(2.5f);
    player.setVolume(-0.5f);
    SUCCEED();
}

TEST(BgmPlayer, NonFiniteVolumeIsSafe) {
    mxh::audio::BgmPlayer player;
    player.setVolume(std::numeric_limits<float>::quiet_NaN());
    player.setVolume(std::numeric_limits<float>::infinity());
    player.setVolume(-std::numeric_limits<float>::infinity());
    SUCCEED();
}
