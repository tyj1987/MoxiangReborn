#include "TextRender.hpp"

namespace mxh::ui {
namespace {

struct TextRenderAdapter {
    TextRenderAdapterFn draw = nullptr;
    void* context = nullptr;
};

TextRenderAdapter& adapter() noexcept {
    static TextRenderAdapter value;
    return value;
}

} // namespace

void bindTextRenderer(TextRenderAdapterFn draw_fn, void* context) noexcept {
    adapter().draw = draw_fn;
    adapter().context = context;
}

bool renderText(const TextRenderRequest& request) {
    if (!adapter().draw) return false;
    if (request.text.empty() &&
        request.caret_byte == TextRenderRequest::NoCaret) {
        return false;
    }
    return adapter().draw(adapter().context, request);
}

} // namespace mxh::ui
