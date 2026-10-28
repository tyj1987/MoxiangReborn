#include <gtest/gtest.h>

#include "mxh/client/CharacterPreviewController.hpp"

namespace mxh::client {

TEST(CharacterPreviewController, RightDragRotatesAndReleaseStops) {
    CharacterPreviewController controller;
    EXPECT_FALSE(controller.onMouseMove(50, 50));
    EXPECT_TRUE(controller.onMouseButton(true, true, 100, 80));
    EXPECT_TRUE(controller.rotating());
    EXPECT_TRUE(controller.onMouseMove(140, 95));
    EXPECT_FLOAT_EQ(controller.yaw(), 0.5f);
    EXPECT_TRUE(controller.onMouseButton(true, false, 140, 95));
    EXPECT_FALSE(controller.rotating());
    EXPECT_FALSE(controller.onMouseMove(180, 95));
    EXPECT_FLOAT_EQ(controller.yaw(), 0.5f);
}

TEST(CharacterPreviewController, LeftButtonIsReservedForUi) {
    CharacterPreviewController controller;
    EXPECT_FALSE(controller.onMouseButton(false, true, 10, 10));
    EXPECT_FALSE(controller.rotating());
}

TEST(CharacterPreviewController, WheelZoomIsClampedAndResettable) {
    CharacterPreviewController controller;
    EXPECT_TRUE(controller.onMouseWheel(120));
    EXPECT_FLOAT_EQ(controller.distance(), 4.35f);
    for (int i = 0; i < 100; ++i) controller.onMouseWheel(120);
    EXPECT_FLOAT_EQ(controller.distance(), CharacterPreviewController::kMinDistance);
    for (int i = 0; i < 100; ++i) controller.onMouseWheel(-120);
    EXPECT_FLOAT_EQ(controller.distance(), CharacterPreviewController::kMaxDistance);
    controller.reset();
    EXPECT_FLOAT_EQ(controller.yaw(), CharacterPreviewController::kDefaultYaw);
    EXPECT_FLOAT_EQ(controller.distance(), CharacterPreviewController::kDefaultDistance);
}

} // namespace mxh::client
