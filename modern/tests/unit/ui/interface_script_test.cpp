// tests/unit/ui/interface_script_test.cpp
// Unit tests for mxh::ui::parse_interface_script — the legacy
// InterfaceScript .bin payload parser used to recover 1:1 dialog
// positions and child layouts from PlayDH/Image/InterfaceScript/*.bin.

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "mxh/ui/interface_script.hpp"

using mxh::ui::InterfaceScript;
using mxh::ui::InterfaceNode;
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
