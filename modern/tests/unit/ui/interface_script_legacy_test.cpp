// tests/unit/ui/interface_script_legacy_test.cpp
// End-to-end 1:1 alignment test: read every legacy InterfaceScript/*.bin
// under PlayDH, parse with modern parse_interface_script + mh_file_ex,
// and verify the parsed root layout matches the goldens stored in
// modern/tests/fixtures/interface_script/*.json. Any drift between
// modern parser output and the legacy-decoded golden is a fail.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/ui/interface_script.hpp"

#include "../../../third_party/nlohmann/json.hpp"

namespace fs = std::filesystem;

namespace {

// Locate PlayDH. In normal local runs this is a junction at
// modern/data/PlayDH. For the harness we accept several fallbacks.
fs::path locate_playdh() {
    fs::path candidates[] = {
        "modern/data/PlayDH",
        "../data/PlayDH",
        "../../data/PlayDH",
        "../../../data/PlayDH",
        "C:/moxiang/modern/data/PlayDH",
        "C:/moxiang/墨香【源码配套资源】/PlayDH",
    };
    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c / "Image" / "InterfaceScript", ec)) return c;
    }
    return {};
}

fs::path locate_golden_dir() {
    fs::path candidates[] = {
        "modern/tests/fixtures/interface_script",
        "../tests/fixtures/interface_script",
        "../../tests/fixtures/interface_script",
        "C:/moxiang/modern/tests/fixtures/interface_script",
    };
    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c, ec)) return c;
    }
    return {};
}

void render_rect(std::ostringstream& os, const char* key,
                 const std::optional<mxh::ui::WindowRect>& r) {
    if (!r.has_value()) return;
    os << ",\"" << key << "\":[" << r->x << "," << r->y << ","
       << r->w << "," << r->h << "]";
}
void render_rect2(std::ostringstream& os, const char* key,
                  const std::optional<mxh::ui::ImageRect>& r) {
    if (!r.has_value()) return;
    os << ",\"" << key << "\":[" << r->left << "," << r->top << ","
       << r->right << "," << r->bottom << "]";
}

void render_node(std::ostringstream& os, const mxh::ui::InterfaceNode* n) {
    os << "{\"type\":\"" << n->type << "\"";
    if (n->id)     os << ",\"id\":\"" << *n->id << "\"";
    if (n->func)   os << ",\"func\":\"" << *n->func << "\"";
    render_rect(os, "point", n->point);
    render_rect(os, "point_low", n->point_low);
    render_rect(os, "caption_rect", n->caption_rect);
    render_rect2(os, "image_src_rect", n->image_src_rect);
    if (n->basic_image_idx >= 0)
        os << ",\"basic_image_idx\":" << n->basic_image_idx;
    if (n->basic_image_rect)
        render_rect2(os, "basic_image_rect", n->basic_image_rect);
    if (n->over_image_idx >= 0)
        os << ",\"overimage_idx\":" << n->over_image_idx;
    if (n->over_image_rect)
        render_rect2(os, "overimage_rect", n->over_image_rect);
    if (n->press_image_idx >= 0)
        os << ",\"pressimage_idx\":" << n->press_image_idx;
    if (n->press_image_rect)
        render_rect2(os, "pressimage_rect", n->press_image_rect);
    if (n->list_over_image_idx >= 0)
        os << ",\"listoverimage_idx\":" << n->list_over_image_idx;
    if (n->list_over_image_rect)
        render_rect2(os, "listoverimage_rect", n->list_over_image_rect);
    if (n->select_image_idx >= 0)
        os << ",\"selectimage_idx\":" << n->select_image_idx;
    if (n->select_image_rect)
        render_rect2(os, "selectimage_rect", n->select_image_rect);
    if (n->focus_image_idx >= 0)
        os << ",\"focusimage_idx\":" << n->focus_image_idx;
    if (n->focus_image_rect)
        render_rect2(os, "focusimage_rect", n->focus_image_rect);
    if (n->tooltip_image_idx >= 0)
        os << ",\"tooltipimage_idx\":" << n->tooltip_image_idx;
    if (n->tooltip_image_rect)
        render_rect2(os, "tooltipimage_rect", n->tooltip_image_rect);
    if (n->tooltip_msg_idx >= 0)
        os << ",\"tooltipmsg_msg_idx\":" << n->tooltip_msg_idx;
    if (n->text_msg_idx >= 0)
        os << ",\"text_msg_idx\":" << n->text_msg_idx;
    if (n->btn_text_msg_idx >= 0)
        os << ",\"btntext_msg_idx\":" << n->btn_text_msg_idx;
    // active/movable default to true — emit only if #ACTIVE/#MOVEABLE
    // was explicit in the source, matching the legacy golden.
    if (n->active_set)   os << ",\"active\":" << (n->active ? "true" : "false");
    if (n->movable_set)  os << ",\"movable\":" << (n->movable ? "true" : "false");
    if (n->auto_close_set) os << ",\"auto_close\":" << (n->auto_close ? "true" : "false");
    if (n->alpha_set) os << ",\"alpha\":" << static_cast<int>(n->alpha);
    if (n->font_idx_set) os << ",\"font_idx\":" << n->font_idx;
    if (!n->children.empty()) {
        os << ",\"children\":[";
        bool first = true;
        for (const auto& c : n->children) {
            if (!first) os << ",";
            first = false;
            render_node(os, c.get());
        }
        os << "]";
    }
    os << "}";
}

std::string render(const mxh::ui::InterfaceScript& s) {
    std::ostringstream os;
    os << "[";
    bool first_root = true;
    for (const auto& r : s.roots) {
        if (!first_root) os << ",";
        first_root = false;
        render_node(os, r.get());
    }
    os << "]";
    return os.str();
}

std::string normalize_for_compare(const std::string& s) {
    // Strip whitespace so JSON layout differences don't matter.
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
        out.push_back(c);
    }
    return out;
}

// Compare two normalized JSON-ish strings structurally (nlohmann/json
// handles nested object equality regardless of key order).
::testing::AssertionResult JsonEqual(const std::string& a,
                                     const std::string& b,
                                     std::string* diff_out) {
    try {
        auto ja = nlohmann::json::parse(a);
        auto jb = nlohmann::json::parse(b);
        if (ja == jb) return ::testing::AssertionSuccess();
        // Compute diff path
        std::string path = "root";
        std::function<bool(const nlohmann::json&, const nlohmann::json&,
                            std::string)>
            walk;
        walk = [&](const nlohmann::json& x, const nlohmann::json& y,
                    std::string p) -> bool {
            if (x.type() != y.type()) {
                *diff_out = p + ": type mismatch (" +
                            std::to_string(static_cast<int>(x.type())) + " vs " +
                            std::to_string(static_cast<int>(y.type())) + ")";
                return false;
            }
            if (x.is_object()) {
                for (auto it = x.begin(); it != x.end(); ++it) {
                    if (!y.contains(it.key())) {
                        *diff_out = p + "/" + it.key() + ": missing in golden";
                        return false;
                    }
                    if (!walk(it.value(), y.at(it.key()), p + "/" + it.key()))
                        return false;
                }
                for (auto it = y.begin(); it != y.end(); ++it) {
                    if (!x.contains(it.key())) {
                        *diff_out = p + "/" + it.key() + ": missing in modern";
                        return false;
                    }
                }
                return true;
            }
            if (x.is_array()) {
                if (x.size() != y.size()) {
                    *diff_out = p + ": array size " +
                                std::to_string(x.size()) + " vs " +
                                std::to_string(y.size());
                    return false;
                }
                for (size_t i = 0; i < x.size(); ++i) {
                    if (!walk(x[i], y[i], p + "[" + std::to_string(i) + "]"))
                        return false;
                }
                return true;
            }
            if (x != y) {
                *diff_out = p + ": " + x.dump() + " vs " + y.dump();
                return false;
            }
            return true;
        };
        walk(ja, jb, path);
        return ::testing::AssertionFailure();
    } catch (const std::exception& e) {
        *diff_out = std::string("parse error: ") + e.what();
        return ::testing::AssertionFailure();
    }
}

std::vector<std::string> collect_bin_names() {
    fs::path playdh;
    fs::path candidates[] = {
        "modern/data/PlayDH",
        "C:/moxiang/modern/data/PlayDH",
        "C:/moxiang/墨香【源码配套资源】/PlayDH",
    };
    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c / "Image" / "InterfaceScript", ec)) { playdh = c; break; }
    }
    std::vector<std::string> names;
    if (!playdh.empty()) {
        for (auto& e : fs::directory_iterator(playdh / "Image" / "InterfaceScript")) {
            if (e.path().extension() == ".bin")
                names.push_back(e.path().filename().string());
        }
        std::sort(names.begin(), names.end());
    }
    return names;
}

}  // namespace

// Single bin file at a time — easier to triage than one big assertion.
class InterfaceScriptLegacy : public ::testing::TestWithParam<std::string> {};

TEST_P(InterfaceScriptLegacy, MatchesGolden) {
    fs::path playdh = locate_playdh();
    fs::path golden_dir = locate_golden_dir();
    ASSERT_FALSE(playdh.empty()) << "Cannot locate PlayDH/Image/InterfaceScript";
    ASSERT_FALSE(golden_dir.empty()) << "Cannot locate fixture directory";

    const std::string fname = GetParam();
    fs::path bin = playdh / "Image" / "InterfaceScript" / fname;
    fs::path golden = golden_dir / (fname.substr(0, fname.size() - 4) + ".json");

    auto read = mxh::compat::read_mh_bin(bin);
    ASSERT_TRUE(read.ok()) << "Failed to read " << bin.string();

    auto parsed = mxh::ui::parse_interface_script(
        std::string_view(reinterpret_cast<const char*>(read.value.data.data()),
                         read.value.data.size()));

    std::ifstream gf(golden);
    ASSERT_TRUE(gf) << "Missing golden for " << fname;

    std::stringstream golden_ss;
    golden_ss << gf.rdbuf();
    std::string golden_raw = golden_ss.str();

    auto roots_pos = golden_raw.find("\"roots\"");
    ASSERT_NE(roots_pos, std::string::npos);
    auto bracket_pos = golden_raw.find('[', roots_pos);
    ASSERT_NE(bracket_pos, std::string::npos);
    // Walk past the matching ']' (ignoring nested [...]).
    int depth = 0;
    size_t end_bracket = bracket_pos;
    for (size_t k = bracket_pos; k < golden_raw.size(); ++k) {
        if (golden_raw[k] == '[') depth++;
        else if (golden_raw[k] == ']') { depth--; if (depth == 0) { end_bracket = k; break; } }
    }
    auto roots_str = golden_raw.substr(bracket_pos, end_bracket - bracket_pos + 1);

    std::string actual = render(parsed);
    std::string diff;
    auto norm_actual = normalize_for_compare(actual);
    auto norm_golden = normalize_for_compare(roots_str);
    EXPECT_TRUE(JsonEqual(norm_actual, norm_golden, &diff))
        << "Layout drift in " << fname << ": " << diff
        << "\n  actual: " << norm_actual
        << "\n  golden: " << norm_golden;
}

INSTANTIATE_TEST_SUITE_P(
    AllBins, InterfaceScriptLegacy,
    ::testing::ValuesIn(collect_bin_names()),
    [](const ::testing::TestParamInfo<std::string>& info) {
        std::string n = info.param;
        for (auto& c : n) {
            if (c == '.' || c == '-') c = '_';
        }
        return n;
    });
