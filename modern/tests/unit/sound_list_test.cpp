#include "mxh/compat/sound_list.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <string>

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

TEST(SoundList, LoadsRealPlayDhSoundListWhenAvailable) {
    std::filesystem::path path;
    for (const auto& first : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        if (!first.is_directory()) continue;
        const auto direct = first.path() / "PlayDH" / "Sound" / "SoundList.bin";
        if (std::filesystem::exists(direct)) { path = direct; break; }
        const auto nested = first.path() / "Sound" / "SoundList.bin";
        if (first.path().filename() == "PlayDH" && std::filesystem::exists(nested)) {
            path = nested; break;
        }
    }
    if (path.empty()) GTEST_SKIP() << "PlayDH fixture is not installed";
    mxh::compat::SoundList list;
    std::string error;
    ASSERT_TRUE(mxh::compat::load_sound_list(path, list, &error)) << error;
    EXPECT_EQ(list.entries.size(), 1674u);
    EXPECT_EQ(list.entries.front().index, 0u);
    EXPECT_EQ(list.entries.back().index, 1673u);
}
