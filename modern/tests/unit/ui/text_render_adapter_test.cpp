#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "cEditBox.hpp"
#include "cStatic.hpp"
#include "TextRender.hpp"

namespace {

struct CapturedText {
    std::string text;
    mxh::ui::TextRenderRequest request;
};

std::vector<CapturedText> g_calls;

bool captureText(void*, const mxh::ui::TextRenderRequest& request) {
    auto copy = request;
    g_calls.push_back({std::string(request.text), copy});
    return true;
}

class TextRenderAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_calls.clear();
        mxh::ui::bindTextRenderer(&captureText, nullptr);
    }

    void TearDown() override {
        mxh::ui::bindTextRenderer(nullptr, nullptr);
    }
};

} // namespace

TEST_F(TextRenderAdapterTest, StaticForwardsLegacyGeometryAndShadow) {
    mxh::ui::cStatic label;
    label.Init(100, 50, 80, 20, nullptr, 1);
    label.SetStaticText("Moxian");
    label.SetTextXY(4, 3);
    label.SetFontIdx(7);
    label.SetAlign(mxh::ui::cStatic::Align::Right);
    label.SetFGColor(0xFF112233u);
    label.SetShadow(true);
    label.SetShadowTextXY(2, 1);
    label.SetShadowColor(0xFF010203u);
    label.Render();

    ASSERT_EQ(g_calls.size(), 2u);
    EXPECT_EQ(g_calls[0].text, "Moxian");
    EXPECT_EQ(g_calls[0].request.x, 102);
    EXPECT_EQ(g_calls[0].request.y, 54);
    EXPECT_EQ(g_calls[0].request.color, 0xFF010203u);
    EXPECT_EQ(g_calls[1].request.x, 100);
    EXPECT_EQ(g_calls[1].request.y, 53);
    EXPECT_EQ(g_calls[1].request.width, 80);
    EXPECT_EQ(g_calls[1].request.left_inset, 4);
    EXPECT_EQ(g_calls[1].request.right_inset, 4);
    EXPECT_EQ(g_calls[1].request.font_index, 7u);
    EXPECT_EQ(g_calls[1].request.align, mxh::ui::TextRenderAlign::Right);
    EXPECT_EQ(g_calls[1].request.color, 0xFF112233u);
}

TEST_F(TextRenderAdapterTest, EditBoxForwardsMaskedTextCaretAndStyle) {
    mxh::ui::cEditBox edit;
    edit.Init(10, 20, 100, 24, nullptr, nullptr, 1);
    edit.InitEditbox(100, 17);
    edit.SetEditText("Hero");
    edit.SetSecret(true);
    edit.SetTextOffset(5, 6, 2);
    edit.SetActiveTextColor(0xFF123456u);
    edit.SetFontIdx(3);
    edit.ActionEvent(20, 25, mxh::ui::cWindow::MouseFlagLButton);
    edit.SetCaretPos(2);
    edit.Render();

    ASSERT_EQ(g_calls.size(), 1u);
    EXPECT_EQ(g_calls[0].text, "****");
    EXPECT_EQ(g_calls[0].request.x, 10);
    EXPECT_EQ(g_calls[0].request.y, 22);
    EXPECT_EQ(g_calls[0].request.width, 100);
    EXPECT_EQ(g_calls[0].request.left_inset, 5);
    EXPECT_EQ(g_calls[0].request.right_inset, 6);
    EXPECT_EQ(g_calls[0].request.color, 0xFF123456u);
    EXPECT_EQ(g_calls[0].request.font_index, 3u);
    EXPECT_EQ(g_calls[0].request.caret_byte, 2u);

    g_calls.clear();
    edit.SetEditText("");
    edit.Render();
    ASSERT_EQ(g_calls.size(), 1u);
    EXPECT_TRUE(g_calls[0].text.empty());
    EXPECT_EQ(g_calls[0].request.caret_byte, 0u);
}
