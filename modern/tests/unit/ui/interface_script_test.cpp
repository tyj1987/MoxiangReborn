// tests/unit/ui/interface_script_test.cpp
// Unit tests for mxh::ui::parse_interface_script — the legacy
// InterfaceScript .bin payload parser used to recover 1:1 dialog
// positions and child layouts from PlayDH/Image/InterfaceScript/*.bin.

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "mxh/ui/interface_script.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <filesystem>

using mxh::ui::cDialog;
using mxh::ui::InterfaceScript;
using mxh::ui::InterfaceNode;
using mxh::ui::apply_legacy_layout;
using mxh::ui::parse_interface_script;

namespace {

// A trimmed reproduction of the MAINDLG from PlayDH/Image/InterfaceScript/
// 15.bin (decrypted payload). Used as a fixed input for parser correctness.
constexpr std::string_view kMainDlgSnippet = R"(
$MAINDLG
{
    #ID        MI_MAINDLG
    #FUNC      MI_DlgFunc
    @#POINT    464 726 560 42
    #POINT     422 726 602 42
    #POINT_    198 558 602 42
    #CAPTIONRECT 0 0 14 42
    #BASICIMAGE ( 21 150 475 164 517 )
    #ACTIVE    1
    #MOVEABLE  1

    $BTN
    {
        #ID   MI_BTN_SIZE
        #POINT 560 0 42 42
        #BASICIMAGE ( 1 100 200 142 242 )
    }
    $STATIC
    {
        #ID   MI_STT_TEXT
        #POINT 0 0 100 42
    }
}
)";

}  // namespace

TEST(InterfaceScriptParser, ParsesSingleRootDialog) {
    auto out = parse_interface_script(kMainDlgSnippet);
    ASSERT_EQ(out.roots.size(), 1u);
    EXPECT_EQ(out.roots[0]->type, "MAINDLG");
}

TEST(InterfaceScriptParser, ParsesDialogProperties) {
    auto out = parse_interface_script(kMainDlgSnippet);
    auto& root = *out.roots[0];
    ASSERT_TRUE(root.point.has_value());
    EXPECT_EQ(root.point->x, 422);
    EXPECT_EQ(root.point->y, 726);
    EXPECT_EQ(root.point->w, 602);
    EXPECT_EQ(root.point->h, 42);
    ASSERT_TRUE(root.point_low.has_value());
    EXPECT_EQ(root.point_low->x, 198);
    EXPECT_EQ(root.point_low->y, 558);
    ASSERT_TRUE(root.caption_rect.has_value());
    EXPECT_EQ(root.caption_rect->x, 0);
    EXPECT_EQ(root.caption_rect->y, 0);
    EXPECT_EQ(root.caption_rect->w, 14);
    EXPECT_EQ(root.caption_rect->h, 42);
    ASSERT_TRUE(root.id.has_value());
    EXPECT_EQ(*root.id, "MI_MAINDLG");
    ASSERT_TRUE(root.func.has_value());
    EXPECT_EQ(*root.func, "MI_DlgFunc");
    EXPECT_EQ(root.basic_image_idx, 21);
    ASSERT_TRUE(root.basic_image_rect.has_value());
    EXPECT_EQ(root.basic_image_rect->left,   150);
    EXPECT_EQ(root.basic_image_rect->top,    475);
    EXPECT_EQ(root.basic_image_rect->right,  164);
    EXPECT_EQ(root.basic_image_rect->bottom, 517);
    EXPECT_TRUE(root.active);
    EXPECT_TRUE(root.movable);
}

TEST(InterfaceScriptParser, ParsesCommentLines) {
    // Comments start with '@' (legacy) — must be skipped, not interpreted
    // as properties. The snippet above has an inline @#POINT before the
    // real #POINT; verify the parser still picks up #POINT correctly.
    auto out = parse_interface_script(kMainDlgSnippet);
    auto& root = *out.roots[0];
    // First POINT in the file (at offset 464) is a comment — should NOT
    // be stored. We only have 1 POINT + 1 POINT_ stored.
    ASSERT_TRUE(root.point.has_value());
    EXPECT_EQ(root.point->x, 422);  // the real one, not the commented 464
}

TEST(InterfaceScriptParser, ParsesChildrenRecursively) {
    auto out = parse_interface_script(kMainDlgSnippet);
    auto& root = *out.roots[0];
    ASSERT_EQ(root.children.size(), 2u);
    EXPECT_EQ(root.children[0]->type, "BTN");
    EXPECT_EQ(root.children[1]->type, "STATIC");
    ASSERT_TRUE(root.children[0]->point.has_value());
    EXPECT_EQ(root.children[0]->point->x, 560);
    EXPECT_EQ(root.children[0]->point->w, 42);
    EXPECT_EQ(root.children[0]->basic_image_idx, 1);
    ASSERT_TRUE(root.children[0]->id.has_value());
    EXPECT_EQ(*root.children[0]->id, "MI_BTN_SIZE");
}

TEST(InterfaceScriptParser, ParsesBoolProperties) {
    auto out = parse_interface_script(kMainDlgSnippet);
    auto& root = *out.roots[0];
    EXPECT_TRUE(root.active);
    EXPECT_TRUE(root.movable);
    EXPECT_FALSE(root.auto_close);
    EXPECT_EQ(root.alpha, 255);
}

TEST(InterfaceScriptParser, HandlesEmptyPayload) {
    auto out = parse_interface_script("");
    EXPECT_TRUE(out.empty());
}

TEST(InterfaceScriptParser, HandlesOnlyCommentLines) {
    constexpr std::string_view only_comments = "@\n@\n@\n";
    auto out = parse_interface_script(only_comments);
    EXPECT_TRUE(out.empty());
}

TEST(InterfaceScriptParser, ParsesNestedThreeLevels) {
    constexpr std::string_view nested = R"(
$OUTER
{
    #POINT 0 0 100 100
    $INNER1
    {
        #POINT 10 10 80 80
        $INNER2
        {
            #POINT 20 20 40 40
        }
    }
}
)";
    auto out = parse_interface_script(nested);
    ASSERT_EQ(out.roots.size(), 1u);
    auto& outer = *out.roots[0];
    EXPECT_EQ(outer.type, "OUTER");
    ASSERT_EQ(outer.children.size(), 1u);
    auto& inner1 = *outer.children[0];
    EXPECT_EQ(inner1.type, "INNER1");
    ASSERT_EQ(inner1.children.size(), 1u);
    auto& inner2 = *inner1.children[0];
    EXPECT_EQ(inner2.type, "INNER2");
    ASSERT_TRUE(inner2.point.has_value());
    EXPECT_EQ(inner2.point->x, 20);
}

TEST(InterfaceScriptParser, ParsesMessageIndexProperties) {
    constexpr std::string_view msgs = R"(
$DLG
{
    #TEXT 1234
    #TOOLTIPMSG 5678
    #BTNTEXT 4321
}
)";
    auto out = parse_interface_script(msgs);
    ASSERT_EQ(out.roots.size(), 1u);
    auto& root = *out.roots[0];
    EXPECT_EQ(root.text_msg_idx, 1234);
    EXPECT_EQ(root.tooltip_msg_idx, 5678);
    EXPECT_EQ(root.btn_text_msg_idx, 4321);
}

TEST(InterfaceScriptParser, IgnoresUnknownProperties) {
    constexpr std::string_view unk = R"(
$DLG
{
    #POINT 1 2 3 4
    #SOMETHING_NEW 99 88 77
    #ACTIVE 1
}
)";
    auto out = parse_interface_script(unk);
    ASSERT_EQ(out.roots.size(), 1u);
    auto& root = *out.roots[0];
    ASSERT_TRUE(root.point.has_value());
    EXPECT_EQ(root.point->x, 1);
    EXPECT_TRUE(root.active);
}

TEST(InterfaceScriptParser, ParsesAlphaAndAutoclose) {
    constexpr std::string_view props = R"(
$DLG
{
    #POINT 0 0 10 10
    #ALPHA 200
    #AUTOCLOSE 1
}
)";
    auto out = parse_interface_script(props);
    auto& root = *out.roots[0];
    EXPECT_EQ(root.alpha, 200);
    EXPECT_TRUE(root.auto_close);
}

TEST(InterfaceScriptParser, FindNodeByIdReturnsRoot) {
    auto out = parse_interface_script(kMainDlgSnippet);
    const auto* n = mxh::ui::find_node_by_id(out, "MI_MAINDLG");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->type, "MAINDLG");
    ASSERT_TRUE(n->point.has_value());
    EXPECT_EQ(n->point->x, 422);
}

TEST(InterfaceScriptParser, FindNodeByIdReturnsChild) {
    auto out = parse_interface_script(kMainDlgSnippet);
    const auto* n = mxh::ui::find_node_by_id(out, "MI_BTN_SIZE");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->type, "BTN");
    ASSERT_TRUE(n->point.has_value());
    EXPECT_EQ(n->point->x, 560);
}

TEST(InterfaceScriptParser, FindNodeByIdReturnsNullForMissing) {
    auto out = parse_interface_script(kMainDlgSnippet);
    EXPECT_EQ(mxh::ui::find_node_by_id(out, "NOT_PRESENT"), nullptr);
}

TEST(InterfaceScriptParser, FindRootByTypeReturnsRoot) {
    auto out = parse_interface_script(kMainDlgSnippet);
    const auto* n = mxh::ui::find_root_by_type(out, "MAINDLG");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->type, "MAINDLG");
}

TEST(InterfaceScriptParser, FindRootByTypeReturnsNullForMissing) {
    auto out = parse_interface_script(kMainDlgSnippet);
    EXPECT_EQ(mxh::ui::find_root_by_type(out, "NOT_A_TYPE"), nullptr);
}

TEST(InterfaceScriptParser, WiringAppliesPositionsFromParsed) {
    // Demonstrates the 1:1 wiring path: parse a legacy .bin payload,
    // pull #POINT for each node, and verify the values match the
    // legacy hand-coded positions.
    auto out = parse_interface_script(kMainDlgSnippet);
    ASSERT_FALSE(out.empty());

    // Root MAINDLG -> (422, 726, 602, 42) — match exactly.
    const auto* maindlg = out.roots[0].get();
    ASSERT_TRUE(maindlg->point.has_value());
    EXPECT_EQ(maindlg->point->x, 422);
    EXPECT_EQ(maindlg->point->y, 726);
    EXPECT_EQ(maindlg->point->w, 602);
    EXPECT_EQ(maindlg->point->h, 42);

    // BTN child -> (560, 0, 42, 42).
    ASSERT_EQ(maindlg->children.size(), 2u);
    const auto& btn = *maindlg->children[0];
    ASSERT_TRUE(btn.point.has_value());
    EXPECT_EQ(btn.point->x, 560);
    EXPECT_EQ(btn.point->y, 0);
    EXPECT_EQ(btn.point->w, 42);
    EXPECT_EQ(btn.point->h, 42);
}

TEST(InterfaceScriptParser, ApplyLegacyLayoutAppliesMainDlgPos) {
    // Demonstrates that apply_legacy_layout feeds the parsed #POINT into
    // a real cDialog instance — positions/sizes match the legacy values
    // exactly.
    auto out = parse_interface_script(kMainDlgSnippet);
    ASSERT_FALSE(out.empty());
    int dummyImage = 1;
    cDialog dlg;
    ASSERT_TRUE(apply_legacy_layout(dlg, *out.roots[0], &dummyImage));
    EXPECT_EQ(dlg.absX(), 422);
    EXPECT_EQ(dlg.absY(), 726);
    EXPECT_EQ(dlg.width(), 602u);
    EXPECT_EQ(dlg.height(), 42u);
    EXPECT_TRUE(dlg.hasCaption());
    EXPECT_EQ(dlg.captionLeft(), 0);
    EXPECT_EQ(dlg.captionTop(), 0);
    EXPECT_EQ(dlg.captionRight(), 14);
    EXPECT_EQ(dlg.captionBottom(), 42);
}

TEST(InterfaceScriptParser, ApplyLegacyLayoutReturnsFalseWithoutPoint) {
    // A node without #POINT cannot be applied.
    constexpr std::string_view no_point = R"(
$DLG
{
    #ACTIVE 1
}
)";
    auto out = parse_interface_script(no_point);
    ASSERT_FALSE(out.empty());
    cDialog dlg;
    EXPECT_FALSE(apply_legacy_layout(dlg, *out.roots[0], nullptr));
}

TEST(InterfaceScriptParser, ApplyLegacyLayoutOnRealMainDlgBinFile) {
    // End-to-end 1:1 wiring: load the real legacy MAINDLG .bin from
    // PlayDH, parse it, and apply to a cDialog. The dialog's
    // position/size must match the legacy hand-coded values (422, 726,
    // 602, 42 — same as our kMainDlgSnippet golden).
    namespace fs = std::filesystem;
    const char* candidates[] = {
        "modern/data/PlayDH",
        "C:/moxiang/modern/data/PlayDH",
        "C:/moxiang/墨香【源码配套资源】/PlayDH",
    };
    fs::path playdh;
    for (const auto* c : candidates) {
        std::error_code ec;
        if (fs::exists(fs::path(c) / "Image" / "InterfaceScript" / "15.bin", ec)) {
            playdh = c;
            break;
        }
    }
    if (playdh.empty()) {
        GTEST_SKIP() << "PlayDH not available; skipping real-bin wiring test.";
    }
    auto read = mxh::compat::read_mh_bin(
        playdh / "Image" / "InterfaceScript" / "15.bin");
    ASSERT_TRUE(read.ok());
    auto parsed = mxh::ui::parse_interface_script(
        std::string_view(reinterpret_cast<const char*>(read.value.data.data()),
                         read.value.data.size()));
    ASSERT_FALSE(parsed.roots.empty());
    int dummy = 0;
    cDialog dlg;
    ASSERT_TRUE(apply_legacy_layout(dlg, *parsed.roots[0], &dummy));
    EXPECT_EQ(dlg.absX(), 422);
    EXPECT_EQ(dlg.absY(), 726);
    EXPECT_EQ(dlg.width(), 602u);
    EXPECT_EQ(dlg.height(), 42u);
}
