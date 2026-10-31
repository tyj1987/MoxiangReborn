#include "mxh/client/dpi_scale.hpp"

#include <gtest/gtest.h>

namespace {

constexpr int kDpi96 = 96;
constexpr int kDpi125 = 120;
constexpr int kDpi150 = 144;
constexpr int kDpi200 = 192;

}  // namespace

TEST(DpiScale, RenderClientStaysRequestedAtCommonScales) {
    for (const int dpi : {kDpi96, kDpi125, kDpi150, kDpi200, 0, -1}) {
        const auto size = mxh::client::render_client_size(800, 600, dpi);
        EXPECT_EQ(size.width, 800) << dpi;
        EXPECT_EQ(size.height, 600) << dpi;
        const auto post = mxh::client::render_client_size(1024, 768, dpi);
        EXPECT_EQ(post.width, 1024) << dpi;
        EXPECT_EQ(post.height, 768) << dpi;
    }
}

TEST(DpiScale, OuterFrameScalesAndClientDoesNot) {
    // Synthetic 96-DPI frame: 8 px left/right, 30 px top, 8 px bottom.
    const auto at96 = mxh::client::outer_window_size(800, 600, 8, 30, 8, 8, kDpi96);
    EXPECT_EQ(at96.width, 816);
    EXPECT_EQ(at96.height, 638);

    const auto at125 = mxh::client::outer_window_size(800, 600, 8, 30, 8, 8, kDpi125);
    EXPECT_EQ(at125.width, 800 + 10 + 10);
    EXPECT_EQ(at125.height, 600 + mxh::client::scale_length_for_dpi(30, kDpi125) + 10);

    const auto at150 = mxh::client::outer_window_size(800, 600, 8, 30, 8, 8, kDpi150);
    EXPECT_EQ(at150.width, 800 + 12 + 12);
    EXPECT_EQ(at150.height, 600 + 45 + 12);
    EXPECT_EQ(mxh::client::scale_length_for_dpi(30, kDpi150), 45);

    const auto at200 = mxh::client::outer_window_size(1024, 768, 8, 30, 8, 8, kDpi200);
    EXPECT_EQ(at200.width, 1024 + 16 + 16);
    EXPECT_EQ(at200.height, 768 + 60 + 16);
}

TEST(DpiScale, MouseInsideClientIsNotDpiScaled) {
    for (const int dpi : {kDpi96, kDpi125, kDpi150, kDpi200}) {
        const auto client = mxh::client::render_client_size(800, 600, dpi);
        const auto center = mxh::client::mouse_to_game_client(
            400, 300, client.width, client.height);
        ASSERT_TRUE(center.has_value()) << dpi;
        EXPECT_EQ(center->x, 400) << dpi;
        EXPECT_EQ(center->y, 300) << dpi;
        const auto corner = mxh::client::mouse_to_game_client(
            799, 599, client.width, client.height);
        ASSERT_TRUE(corner.has_value());
        EXPECT_EQ(corner->x, 799);
        EXPECT_EQ(corner->y, 599);
    }
}

TEST(DpiScale, MouseOutsideClientIsRejected) {
    EXPECT_FALSE(mxh::client::mouse_to_game_client(-1, 0, 800, 600).has_value());
    EXPECT_FALSE(mxh::client::mouse_to_game_client(0, -1, 800, 600).has_value());
    EXPECT_FALSE(mxh::client::mouse_to_game_client(800, 300, 800, 600).has_value());
    EXPECT_FALSE(mxh::client::mouse_to_game_client(400, 600, 800, 600).has_value());
    EXPECT_FALSE(mxh::client::mouse_to_game_client(0, 0, 0, 600).has_value());
    EXPECT_FALSE(mxh::client::mouse_to_game_client(0, 0, 800, 0).has_value());
}

TEST(DpiScale, NonPositiveDpiUsesBaseline) {
    EXPECT_EQ(mxh::client::normalize_dpi(0), mxh::client::kDpiBaseline);
    EXPECT_EQ(mxh::client::normalize_dpi(-144), mxh::client::kDpiBaseline);
    EXPECT_EQ(mxh::client::scale_length_for_dpi(8, 0), 8);
    EXPECT_EQ(mxh::client::outer_window_size(800, 600, 8, 30, 8, 8, 0).width, 816);
}
