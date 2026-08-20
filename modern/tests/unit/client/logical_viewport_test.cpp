#include <gtest/gtest.h>

#include "LogicalViewport.hpp"

namespace mxh::client {
namespace {

TEST(LogicalViewportTest, ExactEightHundredBySixHundredIsIdentity) {
    LogicalViewport viewport;
    viewport.update(800, 600);
    EXPECT_EQ(viewport.content_rect().x, 0);
    EXPECT_EQ(viewport.content_rect().y, 0);
    EXPECT_EQ(viewport.content_rect().width, 800);
    EXPECT_EQ(viewport.content_rect().height, 600);
    EXPECT_FLOAT_EQ(viewport.scale(), 1.0f);
    const auto point = viewport.to_logical(400, 300);
    ASSERT_TRUE(point.has_value());
    EXPECT_FLOAT_EQ(point->x, 400.0f);
    EXPECT_FLOAT_EQ(point->y, 300.0f);
}

TEST(LogicalViewportTest, WideWindowAddsHorizontalBlackBars) {
    LogicalViewport viewport;
    viewport.update(1920, 1080);
    EXPECT_EQ(viewport.content_rect().x, 240);
    EXPECT_EQ(viewport.content_rect().y, 0);
    EXPECT_EQ(viewport.content_rect().width, 1440);
    EXPECT_EQ(viewport.content_rect().height, 1080);
    EXPECT_FLOAT_EQ(viewport.scale(), 1.8f);
    EXPECT_FALSE(viewport.to_logical(239, 300).has_value());
    const auto point = viewport.to_logical(960, 540);
    ASSERT_TRUE(point.has_value());
    EXPECT_FLOAT_EQ(point->x, 400.0f);
    EXPECT_FLOAT_EQ(point->y, 300.0f);
}

TEST(LogicalViewportTest, TallWindowAddsVerticalBlackBars) {
    LogicalViewport viewport;
    viewport.update(800, 800);
    EXPECT_EQ(viewport.content_rect().x, 0);
    EXPECT_EQ(viewport.content_rect().y, 100);
    EXPECT_EQ(viewport.content_rect().width, 800);
    EXPECT_EQ(viewport.content_rect().height, 600);
    EXPECT_FALSE(viewport.to_logical(400, 99).has_value());
    const auto point = viewport.to_logical(799, 699);
    ASSERT_TRUE(point.has_value());
    EXPECT_FLOAT_EQ(point->x, 799.0f);
    EXPECT_FLOAT_EQ(point->y, 599.0f);
}

TEST(LogicalViewportTest, InvalidPhysicalSizeRejectsInput) {
    LogicalViewport viewport;
    viewport.update(0, 0);
    EXPECT_FALSE(viewport.to_logical(0, 0).has_value());
}

}  // namespace
}  // namespace mxh::client
