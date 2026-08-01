#include "guildplustimedialog.hpp"

#include "mxh/ui/cListDialog.hpp"
#include "mxh/ui/cStatic.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cGuildPlusTimeDialog;
using mxh::ui::cListDialog;
using mxh::ui::cStatic;
using mxh::ui::GuildPlusTimeEntry;
using mxh::ui::GuildPlusTimeKind;

namespace {

struct Controls {
    cStatic point;
    cListDialog list;

    void Attach(cGuildPlusTimeDialog& dialog) {
        dialog.SetControlsForTest(&point, &list);
    }
};

const char* StubChatFormat(std::int32_t messageId, void* /*userData*/) {
    switch (messageId) {
    case cGuildPlusTimeDialog::kSuryunMessageId:
        return "Suryun +%d for %u";
    case cGuildPlusTimeDialog::kMugongMessageId:
        return "Mugong +%d for %u";
    case cGuildPlusTimeDialog::kExpMessageId:
        return "Exp +%d for %u";
    case cGuildPlusTimeDialog::kDamageUpMessageId:
        return "DamageUp +%d for %u";
    default:
        return nullptr;
    }
}

struct DispatchCapture {
    int useGuildPointCalls = 0;
    std::int32_t lastForKind = -1;
    std::int32_t lastSlot = -1;
};

void CaptureUseGuildPoint(std::int32_t forKind, std::int32_t slot, void* userData) {
    auto* cap = static_cast<DispatchCapture*>(userData);
    ++cap->useGuildPointCalls;
    cap->lastForKind = forKind;
    cap->lastSlot = slot;
}

std::size_t StubCount(void* userData) {
    auto* v = static_cast<const GuildPlusTimeEntry*>(userData);
    (void)v;
    return 0;
}

GuildPlusTimeEntry SampleEntry(GuildPlusTimeKind k, std::int32_t addData,
                               std::uint32_t needPoint) {
    GuildPlusTimeEntry e{};
    e.kind = k;
    e.addData = addData;
    e.needPoint = needPoint;
    return e;
}

void InitListForHit(cListDialog& list) {
    list.InitList(16, 0, 0, 200, 100);
    list.SetLineHeight(16);
    list.AddItem("row0", 0xFF000000);
    list.AddItem("row1", 0xFF000000);
    list.SetCurSelectedRowIdx(1);
}

} // namespace

TEST(GuildPlusTimeDialogTest, InheritsDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<cDialog, cGuildPlusTimeDialog>);
    static_assert(!std::is_copy_constructible_v<cGuildPlusTimeDialog>);
    static_assert(!std::is_copy_assignable_v<cGuildPlusTimeDialog>);
    SUCCEED();
}

TEST(GuildPlusTimeDialogTest, ConstantsMatchPreprocessedLegacyWindowIds) {
    EXPECT_EQ(cGuildPlusTimeDialog::kCloseButtonId, 364);
    EXPECT_EQ(cGuildPlusTimeDialog::kPlustimeStartButtonId, 513);
    EXPECT_EQ(cGuildPlusTimeDialog::kPointStaticId, 516);
    EXPECT_EQ(cGuildPlusTimeDialog::kPlustimeListId, 517);
    EXPECT_EQ(cGuildPlusTimeDialog::kNoSelection, -1);
    EXPECT_EQ(cGuildPlusTimeDialog::kSuryunMessageId, 1377);
    EXPECT_EQ(cGuildPlusTimeDialog::kMugongMessageId, 1378);
    EXPECT_EQ(cGuildPlusTimeDialog::kExpMessageId, 1379);
    EXPECT_EQ(cGuildPlusTimeDialog::kDamageUpMessageId, 1380);
    EXPECT_EQ(cGuildPlusTimeDialog::kGuildPlusTimeForKind, 0);
    EXPECT_EQ(cGuildPlusTimeDialog::kDefaultTextColor, 0xFFFFFFFFu);
}

TEST(GuildPlusTimeDialogTest, ConstructorDefaultsAreCorrect) {
    cGuildPlusTimeDialog dialog;
    EXPECT_EQ(dialog.currentSelectedItem(), cGuildPlusTimeDialog::kNoSelection);
    EXPECT_EQ(dialog.pointStatic(), nullptr);
    EXPECT_EQ(dialog.plusItemList(), nullptr);
    EXPECT_FALSE(dialog.isActive());
}

TEST(GuildPlusTimeDialogTest, SetControlsForTestAssignsControls) {
    cGuildPlusTimeDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    EXPECT_EQ(dialog.pointStatic(), &controls.point);
    EXPECT_EQ(dialog.plusItemList(), &controls.list);
}

TEST(GuildPlusTimeDialogTest, SetGuildPointTextFormatsThousands) {
    cGuildPlusTimeDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetGuildPointText(0);
    EXPECT_EQ(controls.point.GetStaticText(), "0");
    dialog.SetGuildPointText(15);
    EXPECT_EQ(controls.point.GetStaticText(), "15");
    dialog.SetGuildPointText(1234);
    EXPECT_EQ(controls.point.GetStaticText(), "1,234");
    dialog.SetGuildPointText(1234567);
    EXPECT_EQ(controls.point.GetStaticText(), "1,234,567");
}

TEST(GuildPlusTimeDialogTest, SetGuildPointTextToleratesNullPoint) {
    cGuildPlusTimeDialog dialog;
    EXPECT_NO_FATAL_FAILURE(dialog.SetGuildPointText(42));
}

TEST(GuildPlusTimeDialogTest, LoadPlustimeListFormatsAllFourKinds) {
    cGuildPlusTimeDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetCallbacks(&StubChatFormat, &CaptureUseGuildPoint, &StubCount,
                        nullptr, nullptr);

    const GuildPlusTimeEntry entries[] = {
        SampleEntry(GuildPlusTimeKind::Suryun, 5, 10),
        SampleEntry(GuildPlusTimeKind::Mugong, 3, 20),
        SampleEntry(GuildPlusTimeKind::Exp, 7, 30),
        SampleEntry(GuildPlusTimeKind::DamageUp, 9, 40),
    };
    dialog.SetPlustimeEntries(entries, sizeof(entries) / sizeof(entries[0]));
    dialog.LoadPlustimeList();

    const auto rowCount = controls.list.RowCount();
    ASSERT_EQ(rowCount, 4u);
    EXPECT_EQ(controls.list.GetRow(0).first, "Suryun +5 for 10");
    EXPECT_EQ(controls.list.GetRow(1).first, "Mugong +3 for 20");
    EXPECT_EQ(controls.list.GetRow(2).first, "Exp +7 for 30");
    EXPECT_EQ(controls.list.GetRow(3).first, "DamageUp +9 for 40");
}

TEST(GuildPlusTimeDialogTest, LoadPlustimeListSkipsUnknownKind) {
    cGuildPlusTimeDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetCallbacks(&StubChatFormat, &CaptureUseGuildPoint, &StubCount,
                        nullptr, nullptr);

    const GuildPlusTimeEntry entries[] = {
        SampleEntry(GuildPlusTimeKind::Unknown, 0, 0),
        SampleEntry(GuildPlusTimeKind::Suryun, 1, 2),
    };
    dialog.SetPlustimeEntries(entries, sizeof(entries) / sizeof(entries[0]));
    dialog.LoadPlustimeList();

    const auto rowCount = controls.list.RowCount();
    ASSERT_EQ(rowCount, 2u);
    EXPECT_EQ(controls.list.GetRow(0).first, "");
    EXPECT_EQ(controls.list.GetRow(1).first, "Suryun +1 for 2");
}

TEST(GuildPlusTimeDialogTest, LoadPlustimeListToleratesNullList) {
    cGuildPlusTimeDialog dialog;
    dialog.SetCallbacks(&StubChatFormat, &CaptureUseGuildPoint, &StubCount,
                        nullptr, nullptr);
    EXPECT_NO_FATAL_FAILURE(dialog.LoadPlustimeList());
}

TEST(GuildPlusTimeDialogTest, LinkingResolvesControlsAndResetsState) {
    cGuildPlusTimeDialog dialog;
    dialog.SetPlustimeEntries(nullptr, 0);
    dialog.SetCallbacks(&StubChatFormat, &CaptureUseGuildPoint, &StubCount,
                        nullptr, nullptr);

    auto addStatic = [&](std::int32_t id) {
        auto child = std::make_unique<cStatic>();
        child->Init(0, 0, 10, 10, nullptr, id);
        cStatic* raw = child.get();
        dialog.Add(std::move(child));
        return raw;
    };
    auto addList = [&](std::int32_t id) {
        auto child = std::make_unique<cListDialog>();
        child->Init(0, 0, 100, 100, nullptr, id);
        cListDialog* raw = child.get();
        dialog.Add(std::move(child));
        return raw;
    };

    cStatic* point = addStatic(cGuildPlusTimeDialog::kPointStaticId);
    cListDialog* list = addList(cGuildPlusTimeDialog::kPlustimeListId);

    dialog.Linking();

    EXPECT_EQ(dialog.pointStatic(), point);
    EXPECT_EQ(dialog.plusItemList(), list);
    EXPECT_TRUE(list->IsShowSelect());
    EXPECT_EQ(dialog.currentSelectedItem(), cGuildPlusTimeDialog::kNoSelection);
    EXPECT_EQ(point->GetStaticText(), "0");
}

TEST(GuildPlusTimeDialogTest, HandleMouseActionCapturesSelection) {
    cGuildPlusTimeDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    InitListForHit(controls.list);
    EXPECT_EQ(dialog.currentSelectedItem(), cGuildPlusTimeDialog::kNoSelection);
    dialog.HandleMouseAction(0, 0, cGuildPlusTimeDialog::kActionBtnClick);
    EXPECT_EQ(dialog.currentSelectedItem(), 1);
}

TEST(GuildPlusTimeDialogTest, HandleMouseActionIgnoresMisses) {
    cGuildPlusTimeDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    InitListForHit(controls.list);
    dialog.HandleMouseAction(9999, 9999, cGuildPlusTimeDialog::kActionBtnClick);
    EXPECT_EQ(dialog.currentSelectedItem(), cGuildPlusTimeDialog::kNoSelection);
    dialog.HandleMouseAction(0, 0, 0);
    EXPECT_EQ(dialog.currentSelectedItem(), cGuildPlusTimeDialog::kNoSelection);
}

TEST(GuildPlusTimeDialogTest, HandleMouseActionToleratesNullList) {
    cGuildPlusTimeDialog dialog;
    EXPECT_NO_FATAL_FAILURE(dialog.HandleMouseAction(0, 0,
        cGuildPlusTimeDialog::kActionBtnClick));
}

TEST(GuildPlusTimeDialogTest, OnActionEventStartButtonDispatchesUseGuildPoint) {
    cGuildPlusTimeDialog dialog;
    dialog.SetPlustimeEntries(nullptr, 0);
    DispatchCapture cap;
    dialog.SetCallbacks(&StubChatFormat, &CaptureUseGuildPoint, &StubCount,
                        nullptr, &cap);

    dialog.OnActionEvent(cGuildPlusTimeDialog::kPlustimeStartButtonId, nullptr,
                         cGuildPlusTimeDialog::kActionBtnClick);
    EXPECT_EQ(cap.useGuildPointCalls, 0);

    Controls controls;
    controls.Attach(dialog);
    InitListForHit(controls.list);
    controls.list.SetCurSelectedRowIdx(1);
    dialog.HandleMouseAction(0, 0, cGuildPlusTimeDialog::kActionBtnClick);
    EXPECT_EQ(dialog.currentSelectedItem(), 1);

    dialog.OnActionEvent(cGuildPlusTimeDialog::kPlustimeStartButtonId, nullptr,
                         cGuildPlusTimeDialog::kActionBtnClick);
    EXPECT_EQ(cap.useGuildPointCalls, 1);
    EXPECT_EQ(cap.lastForKind, cGuildPlusTimeDialog::kGuildPlusTimeForKind);
    EXPECT_EQ(cap.lastSlot, 2); // legacy uses selected + 1
}

TEST(GuildPlusTimeDialogTest, OnActionEventCloseButtonDeactivatesDialog) {
    cGuildPlusTimeDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.SetActive(true);
    EXPECT_TRUE(dialog.isActive());
    dialog.OnActionEvent(cGuildPlusTimeDialog::kCloseButtonId, nullptr,
                         cGuildPlusTimeDialog::kActionBtnClick);
    EXPECT_FALSE(dialog.isActive());
}

TEST(GuildPlusTimeDialogTest, OnActionEventIgnoresNonClickFlags) {
    cGuildPlusTimeDialog dialog;
    DispatchCapture cap;
    dialog.SetCallbacks(&StubChatFormat, &CaptureUseGuildPoint, &StubCount,
                        nullptr, &cap);
    dialog.SetActive(true);
    dialog.OnActionEvent(cGuildPlusTimeDialog::kPlustimeStartButtonId, nullptr,
                         0);
    EXPECT_EQ(cap.useGuildPointCalls, 0);
    dialog.OnActionEvent(cGuildPlusTimeDialog::kCloseButtonId, nullptr,
                         0x100);
    EXPECT_TRUE(dialog.isActive());
}

TEST(GuildPlusTimeDialogTest, OnActionEventIgnoresUnknownId) {
    cGuildPlusTimeDialog dialog;
    DispatchCapture cap;
    dialog.SetCallbacks(&StubChatFormat, &CaptureUseGuildPoint, &StubCount,
                        nullptr, &cap);
    dialog.OnActionEvent(99999, nullptr,
                         cGuildPlusTimeDialog::kActionBtnClick);
    EXPECT_EQ(cap.useGuildPointCalls, 0);
}

TEST(GuildPlusTimeDialogTest, SetActivePropagates) {
    cGuildPlusTimeDialog dialog;
    dialog.SetActive(true);
    EXPECT_TRUE(dialog.isActive());
    dialog.SetActive(false);
    EXPECT_FALSE(dialog.isActive());
}
