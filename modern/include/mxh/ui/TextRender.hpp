#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace mxh::ui {

enum class TextRenderAlign : std::uint8_t {
    Left,
    Center,
    Right,
};

struct TextRenderRequest {
    static constexpr std::size_t NoCaret = static_cast<std::size_t>(-1);

    std::string_view text;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t left_inset = 0;
    std::int32_t right_inset = 0;
    std::uint32_t color = 0xFFFFFFFFu;
    std::uint16_t font_index = 0;
    TextRenderAlign align = TextRenderAlign::Left;
    bool multiline = false;
    std::size_t caret_byte = NoCaret;
};

using TextRenderAdapterFn = bool (*)(void* context,
                                     const TextRenderRequest& request);

void bindTextRenderer(TextRenderAdapterFn draw_fn, void* context) noexcept;
bool renderText(const TextRenderRequest& request);

} // namespace mxh::ui
