// mxh/ui/interface_script.hpp — modern parser for legacy InterfaceScript
// .bin files. The legacy client (cScriptManager.cpp) reads dialog layout
// (positions, sizes, image rects, child widgets) from binary files in
// PlayDH/Image/InterfaceScript/*.bin. The modern C++ port previously
// stubbed these positions; this parser recovers them 1:1.
//
// Format (after MHFile decryption, see mh_file_ex.hpp):
//   tokens are ASCII words separated by spaces/tabs, lines by \r\n.
//   #WORD [args]     property on the current widget (#POINT x y w h,
//                    #BASICIMAGE idx l t r b, #ID name, #FUNC name, ...)
//   $WORD            start a new widget child with type WORD (e.g. $BTN,
//                    $STATIC, $EDITBOX, $GUAGENAME, ...). Followed by a
//                    '{' on its own line.
//   {                open a child block (after a $TYPE)
//   }                close the current block
//   @                comment line (legacy skip)
//
// The top-level block is a dialog type token (e.g. $MAINDLG, $CHARGUAGEDLG,
// $INVENTORYDLG). We treat it as the root widget.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::ui {

// 1:1 with legacy cPoint { LONG left, top, right, bottom } used for
// BASICIMAGE source rects and IMAGESRCRECT.
struct ImageRect {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

// 1:1 with legacy cPoint { x, y, w, h } used for #POINT / window rect.
struct WindowRect {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t w = 0;
    std::int32_t h = 0;
};

// One parsed widget block: a dialog root or a child control.
struct InterfaceNode {
    std::string type;            // $TYPE token (e.g. "MAINDLG", "BTN")
    std::optional<std::string> id;        // #ID value (control name)
    std::optional<std::string> func;      // #FUNC value (callback name)
    std::optional<WindowRect> point;      // #POINT x y w h (window rect)
    std::optional<WindowRect> point_low;  // #POINT_ low-res variant
    std::optional<WindowRect> caption_rect;  // #CAPTIONRECT
    std::optional<ImageRect> image_src_rect;  // #IMAGESRCRECT
    std::int32_t basic_image_idx = -1;    // #BASICIMAGE idx
    std::optional<ImageRect> basic_image_rect;  // #BASICIMAGE l t r b
    std::int32_t over_image_idx = -1;     // #OVERIMAGE
    std::optional<ImageRect> over_image_rect;
    std::int32_t press_image_idx = -1;    // #PRESSIMAGE
    std::optional<ImageRect> press_image_rect;
    std::int32_t list_over_image_idx = -1;  // #LISTOVERIMAGE
    std::optional<ImageRect> list_over_image_rect;
    std::int32_t select_image_idx = -1;   // #SELECTIMAGE
    std::optional<ImageRect> select_image_rect;
    std::int32_t focus_image_idx = -1;    // #FOCUSIMAGE
    std::optional<ImageRect> focus_image_rect;
    std::int32_t tooltip_image_idx = -1;  // #TOOLTIPIMAGE
    std::optional<ImageRect> tooltip_image_rect;
    std::int32_t tooltip_msg_idx = -1;    // #TOOLTIPMSG (msg table index)
    std::int32_t text_msg_idx = -1;       // #TEXT msg table index
    std::int32_t btn_text_msg_idx = -1;   // #BTNTEXT msg index
    bool active = true;                   // #ACTIVE (default true)
    bool active_set = false;              // true if #ACTIVE was explicit
    bool movable = true;                  // #MOVEABLE (default true)
    bool movable_set = false;             // true if #MOVEABLE was explicit
    bool auto_close = false;              // #AUTOCLOSE
    bool auto_close_set = false;          // true if #AUTOCLOSE was explicit
    std::uint8_t alpha = 255;             // #ALPHA
    bool alpha_set = false;               // true if #ALPHA was explicit
    std::int32_t font_idx = 0;            // #FONTIDX
    bool font_idx_set = false;            // true if #FONTIDX was explicit
    std::vector<std::unique_ptr<InterfaceNode>> children;
};

// Parsed top-level result for one .bin file.
struct InterfaceScript {
    std::vector<std::unique_ptr<InterfaceNode>> roots;

    [[nodiscard]] bool empty() const noexcept { return roots.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return roots.size(); }
};

// Parse a decrypted MHFile payload (post-mh_file_ex decryption).
// Returns a flat list of root nodes; throws std::runtime_error on
// unrecoverable syntax errors. The parser is forgiving on unknown
// properties (legacy tolerated them) and skips comment lines.
InterfaceScript parse_interface_script(std::string_view payload);

}  // namespace mxh::ui
