#include "legacy_window_event.hpp"
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace mxh::ui::test {

TEST(LegacyWindowEventTest, ValuesMatchLegacyCWindowDef) {
    EXPECT_EQ(legacy_window_event::kNull, 0u);
    EXPECT_EQ(legacy_window_event::kCloseWindow, 1u);
    EXPECT_EQ(legacy_window_event::kTopWindow, 2u);
    EXPECT_EQ(legacy_window_event::kChangeText, 4u);
    EXPECT_EQ(legacy_window_event::kReturn, 8u);
    EXPECT_EQ(legacy_window_event::kPushUp, 16u);
    EXPECT_EQ(legacy_window_event::kPushDown, 32u);
    EXPECT_EQ(legacy_window_event::kButtonClick, 64u);
    EXPECT_EQ(legacy_window_event::kSpinButtonUp, 128u);
    EXPECT_EQ(legacy_window_event::kSpinButtonDown, 256u);
    EXPECT_EQ(legacy_window_event::kRightButtonClick, 512u);
    EXPECT_EQ(legacy_window_event::kLeftButtonClick, 1024u);
    EXPECT_EQ(legacy_window_event::kComboBoxSelect, 2048u);
    EXPECT_EQ(legacy_window_event::kRowClick, 4096u);
    EXPECT_EQ(legacy_window_event::kCellSelect, 8192u);
    EXPECT_EQ(legacy_window_event::kChecked, 16384u);
    EXPECT_EQ(legacy_window_event::kNotChecked, 32768u);
    EXPECT_EQ(legacy_window_event::kLeftButtonDoubleClick, 65536u);
    EXPECT_EQ(legacy_window_event::kRightButtonDoubleClick, 131072u);
    EXPECT_EQ(legacy_window_event::kDestroy, 262144u);
    EXPECT_EQ(legacy_window_event::kSetFocusOn, 524288u);
    EXPECT_EQ(legacy_window_event::kMouseOver, 1048576u);
    EXPECT_EQ(legacy_window_event::kActiveWindow, 2097152u);
    EXPECT_EQ(legacy_window_event::kRowDoubleClick, 4194304u);
}

TEST(LegacyWindowEventTest, NonNullValuesAreDistinctPowersOfTwo) {
    constexpr std::array<std::uint32_t, 23> values = {
        legacy_window_event::kCloseWindow, legacy_window_event::kTopWindow,
        legacy_window_event::kChangeText, legacy_window_event::kReturn,
        legacy_window_event::kPushUp, legacy_window_event::kPushDown,
        legacy_window_event::kButtonClick, legacy_window_event::kSpinButtonUp,
        legacy_window_event::kSpinButtonDown, legacy_window_event::kRightButtonClick,
        legacy_window_event::kLeftButtonClick, legacy_window_event::kComboBoxSelect,
        legacy_window_event::kRowClick, legacy_window_event::kCellSelect,
        legacy_window_event::kChecked, legacy_window_event::kNotChecked,
        legacy_window_event::kLeftButtonDoubleClick,
        legacy_window_event::kRightButtonDoubleClick, legacy_window_event::kDestroy,
        legacy_window_event::kSetFocusOn, legacy_window_event::kMouseOver,
        legacy_window_event::kActiveWindow, legacy_window_event::kRowDoubleClick,
    };
    for (std::size_t leftIndex = 0; leftIndex < values.size(); ++leftIndex) {
        const auto eventValue = values[leftIndex];
        EXPECT_EQ(eventValue & (eventValue - 1u), 0u);
        for (std::size_t rightIndex = leftIndex + 1; rightIndex < values.size(); ++rightIndex) {
            EXPECT_NE(eventValue, values[rightIndex]);
        }
    }
}

}  // namespace mxh::ui::test
