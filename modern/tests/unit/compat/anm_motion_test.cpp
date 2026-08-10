#include <gtest/gtest.h>

#include "mxh/compat/anm_motion.hpp"
#include "mxh/compat/pack_file.hpp"

#include <filesystem>
#include <cmath>

namespace {
std::filesystem::path findCharacterPack() {
    auto root = std::filesystem::current_path();
    for (int level = 0; level < 8; ++level) {
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (!entry.is_directory()) continue;
            const auto path = entry.path() / "PlayDH" / "Character.pak";
            if (std::filesystem::is_regular_file(path)) return path;
        }
        if (!root.has_parent_path() || root.parent_path() == root) break;
        root = root.parent_path();
    }
    return {};
}
}

TEST(AnmMotion, ParsesOriginalMaleIdleMotion) {
    const auto path = findCharacterPack();
    if (path.empty()) GTEST_SKIP() << "PlayDH Character.pak unavailable";
    const auto pack = mxh::compat::PackFile::open(path);
    ASSERT_NE(pack, nullptr);
    const auto bytes = pack->read("m001.anm");
    ASSERT_EQ(bytes.size(), 152880u);
    std::string error;
    const auto motion = mxh::compat::AnmMotion::parse(bytes, &error);
    ASSERT_TRUE(motion.has_value()) << error;
    EXPECT_EQ(motion->ticks_per_frame, 160u);
    EXPECT_EQ(motion->first_frame, 0u);
    EXPECT_EQ(motion->last_frame, 59u);
    EXPECT_EQ(motion->frame_speed, 30u);
    EXPECT_EQ(motion->objects.size(), 116u);
    ASSERT_NE(motion->find("Bip01"), nullptr);
    EXPECT_FALSE(motion->find("Bip01")->rotations.empty());
}

TEST(AnmMotion, InterpolatesPositionScaleAndQuaternion) {
    mxh::compat::AnmMotionObject object;
    object.positions = {{0, 0, {0, 2, 4}}, {160, 10, {10, 12, 14}}};
    object.scales = {{0, 0, {1, 1, 1}}, {160, 10, {3, 5, 7}}};
    object.rotations = {{0, 0, {0, 0, 0, 1}},
                        {160, 10, {0, 0, 1, 0}}};
    EXPECT_EQ(object.samplePosition(5, {}), (std::array<float, 3>{5, 7, 9}));
    EXPECT_EQ(object.sampleScale(5, {}), (std::array<float, 3>{2, 3, 4}));
    const auto rotation = object.sampleRotation(5, {});
    EXPECT_NEAR(rotation[2], std::sqrt(0.5f), 1e-5f);
    EXPECT_NEAR(rotation[3], std::sqrt(0.5f), 1e-5f);
    EXPECT_EQ(object.samplePosition(-1, {}), object.positions.front().value);
    EXPECT_EQ(object.samplePosition(20, {}), object.positions.back().value);
}
