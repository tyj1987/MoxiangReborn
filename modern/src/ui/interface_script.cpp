// mxh/ui/interface_script.cpp — implementation of the InterfaceScript
// .bin parser. See interface_script.hpp for the format description.

#include "mxh/ui/interface_script.hpp"

#include <cctype>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace mxh::ui {

namespace {

// Lex tokens out of the payload, skipping whitespace + comments + braces.
// Returns the next meaningful token starting at `pos`. On EOF, returns
// false. Tokens are:
//   - "$WORD"  widget type opener (consumed up to but not including whitespace)
//   - "#WORD"  property name
//   - "{"      block open (consumed)
//   - "}"      block close (consumed)
//   - plain ASCII word used as property value
// Numeric and quoted-string arguments are read separately via parse_args().
bool next_token(std::string_view payload, std::size_t& pos,
                std::string& out) {
    while (pos < payload.size()) {
        char c = payload[pos];
        if (c == '\r') { ++pos; continue; }
        if (c == '\n') { ++pos; continue; }
        if (c == ' ' || c == '\t') { ++pos; continue; }
        if (c == '{' || c == '}' || c == '(' || c == ')') {
            out.assign(1, c);
            ++pos;
            return true;
        }
        if (c == '@') {
            // legacy comment line — skip to end of line
            while (pos < payload.size() && payload[pos] != '\n') ++pos;
            continue;
        }
        // Read a word: alphanumeric or $ or # or _.
        std::size_t start = pos;
        while (pos < payload.size()) {
            char d = payload[pos];
            if (std::isalnum(static_cast<unsigned char>(d)) ||
                d == '$' || d == '#' || d == '_' || d == '.') {
                ++pos;
            } else break;
        }
        if (pos > start) {
            out.assign(payload.substr(start, pos - start));
            return true;
        }
        // Unknown byte — skip to avoid infinite loop on legacy oddities.
        ++pos;
    }
    return false;
}

// Skip to the next line boundary.
void skip_to_eol(std::string_view payload, std::size_t& pos) {
    while (pos < payload.size() && payload[pos] != '\n') ++pos;
}

// Read 4 ints (x y w h) — used by #POINT and #CAPTIONRECT.
bool parse_4ints(std::string_view payload, std::size_t& pos,
                 std::int32_t* out) {
    for (int i = 0; i < 4; ++i) {
        std::string tok;
        if (!next_token(payload, pos, tok)) return false;
        // Mirror atoi(): silently 0 for non-numeric tokens.
        if (tok.empty()) return false;
        char* end = nullptr;
        long v = std::strtol(tok.c_str(), &end, 10);
        out[i] = (end == tok.c_str()) ? 0 : static_cast<std::int32_t>(v);
    }
    return true;
}

// Read 1 int — used by #TEXT, #TOOLTIPMSG, #BTNTEXT (msg indexes).
bool parse_int(std::string_view payload, std::size_t& pos, std::int32_t& out) {
    std::string tok;
    if (!next_token(payload, pos, tok)) return false;
    // Legacy MHFile::GetInt() uses atoi(), which silently returns 0 for
    // non-numeric tokens (e.g. literal "O" instead of "0" appears in
    // some legacy InterfaceScript .bin files — see 30.bin). Mirror that.
    if (tok.empty()) return false;
    char* end = nullptr;
    long v = std::strtol(tok.c_str(), &end, 10);
    if (end == tok.c_str()) {
        out = 0;
    } else {
        out = static_cast<std::int32_t>(v);
    }
    return true;
}

// Read 5 ints — used by #BASICIMAGE: idx l t r b.
bool parse_5ints(std::string_view payload, std::size_t& pos,
                 std::int32_t* out) {
    for (int i = 0; i < 5; ++i) {
        std::string tok;
        if (!next_token(payload, pos, tok)) return false;
        try { out[i] = std::stoi(tok); } catch (...) { return false; }
    }
    return true;
}

// Read 1 int — used by #BASICIMAGE/#OVERIMAGE/etc without rect (just idx).
// Legacy emits either:
//   "#BASICIMAGE  idx"        (idx only, no rect — uncommon)
//   "#BASICIMAGE ( idx l t r b )"  (parenthesized 5-tuple, common)
bool parse_image_idx(std::string_view payload, std::size_t& pos,
                     std::int32_t& idx, std::optional<ImageRect>& rect) {
    std::string tok;
    std::size_t saved = pos;
    if (next_token(payload, pos, tok) && tok == "(") {
        // ( idx l t r b ) form
        std::int32_t v[5];
        bool ok = true;
        for (int i = 0; i < 5; ++i) {
            if (!next_token(payload, pos, tok)) { ok = false; break; }
            try { v[i] = std::stoi(tok); } catch (...) { ok = false; break; }
        }
        if (ok) {
            std::string close;
            std::size_t saved2 = pos;
            if (next_token(payload, pos, close) && close == ")") {
                idx = v[0];
                rect = ImageRect{v[1], v[2], v[3], v[4]};
                return true;
            }
            pos = saved2;
        }
        // Couldn't parse the parenthesized form — restore and try bare int.
        pos = saved;
    } else {
        pos = saved;
    }
    if (!parse_int(payload, pos, idx)) return false;
    return true;
}

// Apply a single property to a node, reading any required args from the
// stream. Returns false on unrecoverable parse error.
bool apply_property(InterfaceNode& node, std::string_view prop,
                    std::string_view payload, std::size_t& pos) {
    if (prop == "POINT") {
        WindowRect r;
        if (!parse_4ints(payload, pos, &r.x)) return false;
        node.point = r;
        return true;
    }
    if (prop == "POINT_") {
        WindowRect r;
        if (!parse_4ints(payload, pos, &r.x)) return false;
        node.point_low = r;
        return true;
    }
    if (prop == "CAPTIONRECT") {
        WindowRect r;
        if (!parse_4ints(payload, pos, &r.x)) return false;
        node.caption_rect = r;
        return true;
    }
    if (prop == "BASICIMAGE") {
        std::int32_t idx;
        std::optional<ImageRect> rect;
        if (!parse_image_idx(payload, pos, idx, rect)) return false;
        node.basic_image_idx = idx;
        node.basic_image_rect = rect;
        return true;
    }
    if (prop == "OVERIMAGE" || prop == "PRESSIMAGE" ||
        prop == "LISTOVERIMAGE" || prop == "SELECTIMAGE" ||
        prop == "FOCUSIMAGE" || prop == "TOOLTIPIMAGE") {
        std::int32_t idx;
        std::optional<ImageRect> rect;
        if (!parse_image_idx(payload, pos, idx, rect)) return false;
        if (prop == "OVERIMAGE") {
            node.over_image_idx = idx;
            node.over_image_rect = rect;
        } else if (prop == "PRESSIMAGE") {
            node.press_image_idx = idx;
            node.press_image_rect = rect;
        } else if (prop == "LISTOVERIMAGE") {
            node.list_over_image_idx = idx;
            node.list_over_image_rect = rect;
        } else if (prop == "SELECTIMAGE") {
            node.select_image_idx = idx;
            node.select_image_rect = rect;
        } else if (prop == "FOCUSIMAGE") {
            node.focus_image_idx = idx;
            node.focus_image_rect = rect;
        } else {
            node.tooltip_image_idx = idx;
            node.tooltip_image_rect = rect;
        }
        return true;
    }
    if (prop == "IMAGESRCRECT") {
        std::int32_t v[4];
        if (!parse_4ints(payload, pos, v)) return false;
        node.image_src_rect = ImageRect{v[0], v[1], v[2], v[3]};
        return true;
    }
    if (prop == "TOOLTIPMSG" || prop == "TEXT" || prop == "BTNTEXT") {
        std::int32_t idx;
        if (!parse_int(payload, pos, idx)) return false;
        if (prop == "TOOLTIPMSG") node.tooltip_msg_idx = idx;
        else if (prop == "TEXT") node.text_msg_idx = idx;
        else node.btn_text_msg_idx = idx;
        return true;
    }
    if (prop == "ACTIVE") {
        std::int32_t v;
        if (!parse_int(payload, pos, v)) return false;
        node.active = (v != 0);
        node.active_set = true;
        return true;
    }
    if (prop == "MOVEABLE") {
        std::int32_t v;
        if (!parse_int(payload, pos, v)) return false;
        node.movable = (v != 0);
        node.movable_set = true;
        return true;
    }
    if (prop == "AUTOCLOSE") {
        std::int32_t v;
        if (!parse_int(payload, pos, v)) return false;
        node.auto_close = (v != 0);
        node.auto_close_set = true;
        return true;
    }
    if (prop == "ALPHA") {
        std::int32_t v;
        if (!parse_int(payload, pos, v)) return false;
        node.alpha = static_cast<std::uint8_t>(v & 0xff);
        node.alpha_set = true;
        return true;
    }
    if (prop == "FONTIDX") {
        std::int32_t v;
        if (!parse_int(payload, pos, v)) return false;
        node.font_idx = v;
        node.font_idx_set = true;
        return true;
    }
    if (prop == "ID") {
        std::string tok;
        if (!next_token(payload, pos, tok)) return false;
        node.id = tok;
        return true;
    }
    if (prop == "FUNC") {
        std::string tok;
        if (!next_token(payload, pos, tok)) return false;
        node.func = tok;
        return true;
    }
    // Properties below are recognized for 1:1 fidelity to the legacy
    // format (so the parser doesn't accidentally skip the *next*
    // property line). The values themselves are not surfaced in the
    // modern InterfaceNode yet — only the parser skip semantics matter.
    // Each consumes a fixed number of int arguments.
    struct IntArgs {
        std::int32_t count;
    };
    static const std::unordered_map<std::string, IntArgs> kIntProps = {
        {"FGCOLOR",        {3}},
        {"TEXTCOLOR",      {6}},
        {"SHADOWCOLOR",    {3}},
        {"EDITSIZE",       {2}},
        {"SPINSIZE",       {2}},
        {"TEXTRECT",       {4}},
        {"TEXTSHADOWRECT", {4}},
        {"TEXTXY",         {2}},
        {"CAPTIONRECT",    {4}},
        {"LIMITBYTES",     {1}},
        {"TEXTALIGN",      {1}},
        {"MAXLINE",        {1}},
        {"LISTMAXLINE",    {1}},
        {"LINEHEIGHT",     {1}},
        {"BTNTEXTANI",     {2}},
        {"COORD",          {2}},
        {"ITEMTOOLTIP",    {1}},
        {"TOOLTIPCOL",     {3}},
        {"SHADOWTEXT",     {2}},
    };
    auto it = kIntProps.find(std::string(prop));
    if (it != kIntProps.end()) {
        for (std::int32_t k = 0; k < it->second.count; ++k) {
            std::int32_t junk;
            if (!parse_int(payload, pos, junk)) return false;
        }
        return true;
    }
    // Single-bool properties.
    static const std::unordered_set<std::string> kBoolProps = {
        "SHADOW", "AUTOSCROLL", "PASSIVE", "READONLY", "SECRET",
    };
    if (kBoolProps.count(std::string(prop))) {
        std::int32_t junk;
        return parse_int(payload, pos, junk);
    }
    // Unknown property — skip to end of line.
    skip_to_eol(payload, pos);
    return true;
}

// Recursively parse one node starting at `pos`. Children of type
// $FOO { ... } are parsed and appended.
std::unique_ptr<InterfaceNode> parse_node(std::string_view payload,
                                          std::size_t& pos,
                                          std::string type_token) {
    auto node = std::make_unique<InterfaceNode>();
    node->type = std::move(type_token);

    // Expect a "{" next to open the block. (Legacy grammar: type token
    // is immediately followed by { on the same line.)
    std::string tok;
    if (!next_token(payload, pos, tok)) return node;
    if (tok != "{") {
        // No block — single-token entry. Bail out.
        return node;
    }

    while (true) {
        if (!next_token(payload, pos, tok)) break;
        if (tok == "}") return node;

        if (!tok.empty() && tok[0] == '$') {
            // Child widget of type tok.substr(1).
            std::string child_type = tok.substr(1);
            auto child = parse_node(payload, pos, std::move(child_type));
            if (child) node->children.push_back(std::move(child));
            continue;
        }

        if (!tok.empty() && tok[0] == '#') {
            std::string prop = tok.substr(1);
            if (!apply_property(*node, prop, payload, pos)) break;
            continue;
        }

        // Stray token — skip to end of line and continue.
        skip_to_eol(payload, pos);
    }
    return node;
}

}  // namespace

InterfaceScript parse_interface_script(std::string_view payload) {
    InterfaceScript out;
    std::size_t pos = 0;
    std::string tok;
    while (next_token(payload, pos, tok)) {
        if (!tok.empty() && tok[0] == '$') {
            std::string root_type = tok.substr(1);
            auto root = parse_node(payload, pos, std::move(root_type));
            if (root) out.roots.push_back(std::move(root));
        } else {
            // Stray non-$ token at top level — skip line.
            skip_to_eol(payload, pos);
        }
    }
    return out;
}

const InterfaceNode* find_node_by_id(const InterfaceScript& script,
                                     std::string_view id) {
    for (const auto& root : script.roots) {
        if (root->id && *root->id == id) return root.get();
        for (const auto& child : root->children) {
            if (child->id && *child->id == id) return child.get();
            for (const auto& gc : child->children) {
                if (gc->id && *gc->id == id) return gc.get();
            }
        }
    }
    return nullptr;
}

const InterfaceNode* find_root_by_type(const InterfaceScript& script,
                                       std::string_view type) {
    for (const auto& root : script.roots) {
        if (root->type == type) return root.get();
    }
    return nullptr;
}

}  // namespace mxh::ui
