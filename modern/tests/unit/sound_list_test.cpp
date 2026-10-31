#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/compat/sound_list.hpp"

#include <gtest/gtest.h>

#include <string>
#include "../support/rog_playdh.hpp"
#include "../support/synthetic_fixture.hpp"

namespace {

const std::string kHandWrittenSoundList =
    "3\r\n"
    "0 fx/tap.wav 0 0 2 90 1\r\n"
    "1 NULL.WAV 0 0 0 0 0\r\n"
    "2 bgm/theme.mp3 1 1 12 600 0.5\r\n";

}  // namespace

TEST(SoundList, ParsesLegacyFieldsAndNullSentinel) {
    const std::string payload =
        "3\r\n"
        "0 Sound\\BGM\\login.mp3 1 1 10.5 500 0.75\r\n"
        "1 Sound\\Effect\\swing.wav 0 0 1 80 1\r\n"
        "2 NULL.WAV 0 0 0 0 0\r\n";
    mxh::compat::SoundList list;
    std::string error;
    ASSERT_TRUE(mxh::compat::parse_sound_list_payload(
        {reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()}, list, &error)) << error;
    ASSERT_EQ(list.entries.size(), 3u);
    EXPECT_EQ(list.entries[0].file_name, "Sound\\BGM\\login.mp3");
    EXPECT_TRUE(list.entries[0].loop);
    EXPECT_TRUE(list.entries[0].streaming);
    EXPECT_FLOAT_EQ(list.entries[0].min_distance, 10.5f);
    EXPECT_FALSE(list.entries[2].available);
}

TEST(SoundList, RejectsNonSequentialIndex) {
    const std::string payload = "1 7 bad.wav 0 0 0 0 1";
    mxh::compat::SoundList list;
    EXPECT_FALSE(mxh::compat::parse_sound_list_payload(
        {reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()}, list));
}

TEST(SoundList, LoadsHandWrittenEncryptedBin) {
    mxh::test::SyntheticFixtureDir dir;
    ASSERT_TRUE(dir.created());
    const auto path = dir.write_bytes("SoundList.bin", "x");
    ASSERT_FALSE(path.empty());
    const auto written = mxh::compat::write_mh_bin(
        path, {reinterpret_cast<const std::uint8_t*>(kHandWrittenSoundList.data()),
               kHandWrittenSoundList.size()});
    ASSERT_EQ(written, mxh::compat::MhError::Ok) << path.string();
    mxh::compat::SoundList list;
    std::string error;
    ASSERT_TRUE(mxh::compat::load_sound_list(path, list, &error)) << error;
    ASSERT_EQ(list.entries.size(), 3u);
    EXPECT_EQ(list.entries[0].file_name, "fx/tap.wav");
    EXPECT_FALSE(list.entries[0].streaming);
    EXPECT_TRUE(list.entries[0].available);
    EXPECT_FALSE(list.entries[1].available);
    EXPECT_EQ(list.entries[2].file_name, "bgm/theme.mp3");
    EXPECT_TRUE(list.entries[2].streaming);
    EXPECT_EQ(list.entries[2].index, 2u);
}

TEST(SoundList, LoadsRealPlayDhSoundListWhenAvailable) {
    const auto root = mxh::test::require_rog_playdh_root();
    const auto path = root / "Sound" / "SoundList.bin";
    mxh::compat::SoundList list;
    std::string error;
    ASSERT_TRUE(mxh::compat::load_sound_list(path, list, &error)) << error;
    EXPECT_EQ(list.entries.size(), 1674u);
    EXPECT_EQ(list.entries.front().index, 0u);
    EXPECT_EQ(list.entries.back().index, 1673u);
}
