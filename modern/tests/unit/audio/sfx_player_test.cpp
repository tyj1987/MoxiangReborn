#include "mxh/audio/sfx_player.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <string>
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

TEST(SfxPlayer, ResolvesNonStreamingWavAndRejectsBgm) {
    HandWrittenSoundRoot sound;
    ASSERT_TRUE(sound.ok());
    mxh::audio::SfxPlayer player;
    std::string error;
    ASSERT_TRUE(player.initialize(sound.path(), &error)) << error;
    EXPECT_TRUE(player.resolve(2).empty());
    const auto wav = player.resolve(0);
    ASSERT_FALSE(wav.empty());
    EXPECT_EQ(wav.filename(), "tap.wav");
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
