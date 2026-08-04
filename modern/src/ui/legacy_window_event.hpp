#pragma once

#include <cstdint>

namespace mxh::ui::legacy_window_event {

inline constexpr std::uint32_t kNull = 0u;
inline constexpr std::uint32_t kCloseWindow = 1u;
inline constexpr std::uint32_t kTopWindow = 2u;
inline constexpr std::uint32_t kChangeText = 4u;
inline constexpr std::uint32_t kReturn = 8u;
inline constexpr std::uint32_t kPushUp = 16u;
inline constexpr std::uint32_t kPushDown = 32u;
inline constexpr std::uint32_t kButtonClick = 64u;
inline constexpr std::uint32_t kSpinButtonUp = 128u;
inline constexpr std::uint32_t kSpinButtonDown = 256u;
inline constexpr std::uint32_t kRightButtonClick = 512u;
inline constexpr std::uint32_t kLeftButtonClick = 1024u;
inline constexpr std::uint32_t kComboBoxSelect = 2048u;
inline constexpr std::uint32_t kRowClick = 4096u;
inline constexpr std::uint32_t kCellSelect = 8192u;
inline constexpr std::uint32_t kChecked = 16384u;
inline constexpr std::uint32_t kNotChecked = 32768u;
inline constexpr std::uint32_t kLeftButtonDoubleClick = 65536u;
inline constexpr std::uint32_t kRightButtonDoubleClick = 131072u;
inline constexpr std::uint32_t kDestroy = 262144u;
inline constexpr std::uint32_t kSetFocusOn = 524288u;
inline constexpr std::uint32_t kMouseOver = 1048576u;
inline constexpr std::uint32_t kActiveWindow = 2097152u;
inline constexpr std::uint32_t kRowDoubleClick = 4194304u;

}  // namespace mxh::ui::legacy_window_event
