// ccombobox_test.cpp — Phase 6.14 / 0.13.48 coverage for cComboBox
// (combo box with dropdown). Tests the data model + state machine
// + selection logic. Render / cbWindowFunc dispatch are no-ops in
// the modern port; the engine-side bindings (WINDOWMGR mouse
// gating, cImage sprite draw) are stubbed to no-op.

#include "ccombobox.hpp"

#include "cWindow.hpp"

#include <gtest/gtest.h>

namespace {

// cPushupButton-like placeholder. cPushupButton is opaque in
// modern; we use a cWindow instance for the SetAbsXY / setParent
// plumbing. (Tests don't actually call SetPush / IsPushed since
// the modern cComboBox doesn't call them — those are the engine
// dispatcher's job.)
mxh::ui::cWindow* MakePushupPlaceholder() {
    auto* w = new mxh::ui::cWindow;
    w->Init(0, 0, 10, 10, nullptr, 100);
    return w;
}

}  // namespace

TEST(CComboBox, DefaultConstruction) {
    mxh::ui::cComboBox c;
    EXPECT_EQ(c.GetItemCount(), 0u);
    EXPECT_EQ(c.GetCurSelectedIdx(), -1);
    EXPECT_EQ(c.GetOverIdx(), -1);
    EXPECT_TRUE(c.GetComboText().empty());
    EXPECT_EQ(c.GetComboBtn(), nullptr);
}

TEST(CComboBox, InheritsCWindow) {
    // cListItem extends cWindow; cComboBox extends cListItem.
    // absX/absY/Init should work.
    mxh::ui::cComboBox c;
    c.Init(0, 0, 200, 30, nullptr, 1);
    EXPECT_EQ(c.width(), 200u);
    EXPECT_EQ(c.height(), 30u);
    EXPECT_EQ(c.id(), 1);
}

TEST(CComboBox, InitComboListStoresImages) {
    mxh::ui::cComboBox c;
    c.InitComboList(/*listWid*/150,
                    /*topImage*/reinterpret_cast<void*>(0x1),   /*topHei*/10,
                    /*middleImage*/reinterpret_cast<void*>(0x2), /*middleHei*/20,
                    /*downImage*/reinterpret_cast<void*>(0x3),   /*downHei*/10,
                    /*overImage*/reinterpret_cast<void*>(0x4));
    EXPECT_EQ(c.GetListWidth(), 150u);
    EXPECT_EQ(c.GetTopHeight(), 10u);
    EXPECT_EQ(c.GetMiddleHeight(), 20u);
    EXPECT_EQ(c.GetDownHeight(), 10u);
    EXPECT_EQ(c.GetTopImage(),   reinterpret_cast<void*>(0x1));
    EXPECT_EQ(c.GetMiddleImage(),reinterpret_cast<void*>(0x2));
    EXPECT_EQ(c.GetDownImage(),  reinterpret_cast<void*>(0x3));
    EXPECT_EQ(c.GetOverImage(),  reinterpret_cast<void*>(0x4));
}

TEST(CComboBox, AddItemAppendsToEnd) {
    mxh::ui::cComboBox c;
    c.SetMaxLine(10);
    c.AddItem({"item0", 0xFF000000, 0});
    c.AddItem({"item1", 0xFFFF0000, 1});
    EXPECT_EQ(c.GetItemCount(), 2u);
    EXPECT_EQ(c.Items().at(0).text, "item0");
    EXPECT_EQ(c.Items().at(1).text, "item1");
}

TEST(CComboBox, AddItemAtIndexInserts) {
    mxh::ui::cComboBox c;
    c.SetMaxLine(10);
    c.AddItem({"a"});
    c.AddItem({"c"});
    c.AddItem({"b"}, 1);
    EXPECT_EQ(c.GetItemCount(), 3u);
    EXPECT_EQ(c.Items().at(0).text, "a");
    EXPECT_EQ(c.Items().at(1).text, "b");
    EXPECT_EQ(c.Items().at(2).text, "c");
}

TEST(CComboBox, AddItemFifoEvictionAtMaxLine) {
    // 1:1 with legacy: at cap, head is dropped on AddItem.
    mxh::ui::cComboBox c;
    c.SetMaxLine(3);
    c.AddItem({"a"});
    c.AddItem({"b"});
    c.AddItem({"c"});
    c.AddItem({"d"});  // cap reached → drop "a"
    EXPECT_EQ(c.GetItemCount(), 3u);
    EXPECT_EQ(c.Items().at(0).text, "b");
    EXPECT_EQ(c.Items().at(1).text, "c");
    EXPECT_EQ(c.Items().at(2).text, "d");
}

TEST(CComboBox, AddItemAtIndexBeyondSizeIsNoOp) {
    // 1:1 with legacy: idx > size → silently dropped.
    mxh::ui::cComboBox c;
    c.SetMaxLine(10);
    c.AddItem({"a"});
    c.AddItem({"b"}, 99);
    EXPECT_EQ(c.GetItemCount(), 1u);
    EXPECT_EQ(c.Items().at(0).text, "a");
}

TEST(CComboBox, RemoveAllClearsItems) {
    mxh::ui::cComboBox c;
    c.SetMaxLine(10);
    c.AddItem({"a"});
    c.AddItem({"b"});
    c.AddItem({"c"});
    c.RemoveAll();
    EXPECT_EQ(c.GetItemCount(), 0u);
}

TEST(CComboBox, RemoveItemAtIndex) {
    mxh::ui::cComboBox c;
    c.SetMaxLine(10);
    c.AddItem({"a"});
    c.AddItem({"b"});
    c.AddItem({"c"});
    c.RemoveItem(1);
    EXPECT_EQ(c.GetItemCount(), 2u);
    EXPECT_EQ(c.Items().at(0).text, "a");
    EXPECT_EQ(c.Items().at(1).text, "c");
}

TEST(CComboBox, SetMaxLineCapRoundTrip) {
    mxh::ui::cComboBox c;
    EXPECT_EQ(c.GetMaxLine(), 0u);  // 0 = unlimited
    c.SetMaxLine(50);
    EXPECT_EQ(c.GetMaxLine(), 50u);
}

TEST(CComboBox, SetCurSelectedIdxRoundTrip) {
    mxh::ui::cComboBox c;
    EXPECT_EQ(c.GetCurSelectedIdx(), -1);
    c.SetCurSelectedIdx(3);
    EXPECT_EQ(c.GetCurSelectedIdx(), 3);
    c.SetCurSelectedIdx(-1);
    EXPECT_EQ(c.GetCurSelectedIdx(), -1);
}

TEST(CComboBox, SetOverIdxRoundTrip) {
    mxh::ui::cComboBox c;
    EXPECT_EQ(c.GetOverIdx(), -1);
    c.SetOverIdx(2);
    EXPECT_EQ(c.GetOverIdx(), 2);
}

TEST(CComboBox, SelectComboTextCopiesItemText) {
    mxh::ui::cComboBox c;
    c.SetMaxLine(10);
    c.AddItem({"apple",  0xFFFF0000, 0});
    c.AddItem({"banana", 0xFFFFFF00, 0});
    c.SelectComboText(1);
    EXPECT_EQ(c.GetComboText(), "banana");
    c.SelectComboText(0);
    EXPECT_EQ(c.GetComboText(), "apple");
}

TEST(CComboBox, SelectComboTextOutOfRangeIsNoOp) {
    mxh::ui::cComboBox c;
    c.SetMaxLine(10);
    c.AddItem({"a"});
    c.SelectComboText(99);
    EXPECT_TRUE(c.GetComboText().empty());
}

TEST(CComboBox, SetComboTextColorRoundTrip) {
    mxh::ui::cComboBox c;
    c.SetComboTextColor(0xFFAABBCC);
    // No public getter for color; verify no crash. (1:1 with
    // legacy: SetComboTextColor has no GetComboTextColor.)
    SUCCEED();
}

TEST(CComboBox, SetOverImageScaleRoundTrip) {
    mxh::ui::cComboBox c;
    c.SetOverImageScale(2.0f, 3.0f);
    SUCCEED();  // No public getter; verify no crash.
}

TEST(CComboBox, SetMarginRoundTrip) {
    mxh::ui::cComboBox c;
    c.SetMargin(1, 2, 3, 4);
    auto r = c.GetTextClippingRect();
    EXPECT_EQ(r.left, 1);
    EXPECT_EQ(r.top, 2);
    EXPECT_EQ(r.right, 3);
    EXPECT_EQ(r.bottom, 4);
}

TEST(CComboBox, AddLinkComboBtn) {
    // 1:1 with legacy. Add() links the cPushupButton (modern
    // cWindow placeholder) and cascades SetAbsXY + setParent.
    mxh::ui::cComboBox c;
    c.Init(0, 0, 100, 30, nullptr, 1);
    auto* btn = MakePushupPlaceholder();
    c.Add(btn);
    EXPECT_EQ(c.GetComboBtn(), btn);
    delete btn;
}

TEST(CComboBox, AddNullBtnIsSafe) {
    mxh::ui::cComboBox c;
    c.Add(nullptr);
    EXPECT_EQ(c.GetComboBtn(), nullptr);
}

TEST(CComboBox, RenderIsNoop) {
    mxh::ui::cComboBox c;
    c.Init(0, 0, 100, 30, nullptr, 1);
    c.Render();
    c.Render();  // idempotent
    EXPECT_EQ(c.width(), 100u);
}

TEST(CComboBox, PtIdxInComboListNoHitReturnsNoHit) {
    // 1:1 with legacy: returns GetItemCount()+1 on no-hit.
    // With 0 items, that returns 1.
    mxh::ui::cComboBox c;
    c.Init(0, 0, 100, 30, nullptr, 1);
    c.InitComboList(100, nullptr, 10, nullptr, 20, nullptr, 10, nullptr);
    EXPECT_EQ(c.PtIdxInComboList(50, 50), 1u);  // no items → no hit
}

TEST(CComboBox, PtIdxInComboListWithItems) {
    // 1:1 with legacy: returns the row index if the click is
    // inside the list rect. The legacy uses m_absPos + m_height
    // + (i+1) * m_middleHeight for the y-check. With absX=0
    // absY=0, listWid=100, m_middleHeight=20:
    //   row 0: y in [30, 50)  (height=30, then 1*20=50)
    //   row 1: y in [50, 70)
    mxh::ui::cComboBox c;
    c.Init(0, 0, 100, 30, nullptr, 1);
    c.InitComboList(100, nullptr, 10, nullptr, 20, nullptr, 10, nullptr);
    c.SetMaxLine(10);
    c.AddItem({"row0"});
    c.AddItem({"row1"});
    c.AddItem({"row2"});
    EXPECT_EQ(c.PtIdxInComboList(50, 35), 0u);  // row 0
    EXPECT_EQ(c.PtIdxInComboList(50, 55), 1u);  // row 1
    EXPECT_EQ(c.PtIdxInComboList(50, 75), 2u);  // row 2
    EXPECT_EQ(c.PtIdxInComboList(50, 100), 4u); // no hit
}

TEST(CComboBox, ListMouseCheckLeftDownSetsCurSelectedIdx) {
    // 1:1 with legacy. leftDown=true sets m_nCurSelectedIdx to
    // the row under (mouseX, mouseY) and copies the item text
    // to m_comboText.
    mxh::ui::cComboBox c;
    c.Init(0, 0, 100, 30, nullptr, 1);
    c.InitComboList(100, nullptr, 10, nullptr, 20, nullptr, 10, nullptr);
    c.SetMaxLine(10);
    c.AddItem({"first"});
    c.AddItem({"second"});
    c.ListMouseCheck(50, 55, /*leftDown*/true);
    EXPECT_EQ(c.GetCurSelectedIdx(), 1);
    EXPECT_EQ(c.GetComboText(), "second");
}

TEST(CComboBox, ListMouseCheckHoverOnlySetsOverIdx) {
    // 1:1 with legacy. leftDown=false only updates the over
    // index, not the selected index.
    mxh::ui::cComboBox c;
    c.Init(0, 0, 100, 30, nullptr, 1);
    c.InitComboList(100, nullptr, 10, nullptr, 20, nullptr, 10, nullptr);
    c.SetMaxLine(10);
    c.AddItem({"first"});
    c.AddItem({"second"});
    c.SetCurSelectedIdx(-1);
    c.ListMouseCheck(50, 35, /*leftDown*/false);
    EXPECT_EQ(c.GetCurSelectedIdx(), -1);
    EXPECT_EQ(c.GetOverIdx(), 0);
}

TEST(CComboBox, ListMouseCheckNoHitResetsSelection) {
    // 1:1 with legacy. Click outside the list rect → m_nCurSelectedIdx = -1.
    mxh::ui::cComboBox c;
    c.Init(0, 0, 100, 30, nullptr, 1);
    c.InitComboList(100, nullptr, 10, nullptr, 20, nullptr, 10, nullptr);
    c.SetMaxLine(10);
    c.AddItem({"first"});
    c.SetCurSelectedIdx(0);
    c.ListMouseCheck(50, 9999, /*leftDown*/true);
    EXPECT_EQ(c.GetCurSelectedIdx(), -1);
}
