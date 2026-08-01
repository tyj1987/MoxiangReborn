#include "mnjoindialog.hpp"

#include "mxh/ui/cStatic.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cMNJoinDialog;
using mxh::ui::cStatic;

TEST(MNJoinDialogTest, InheritsDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<cDialog, cMNJoinDialog>);
    static_assert(!std::is_copy_constructible_v<cMNJoinDialog>);
    static_assert(!std::is_copy_assignable_v<cMNJoinDialog>);
    static_assert(std::has_virtual_destructor_v<cMNJoinDialog>);
    SUCCEED();
}

TEST(MNJoinDialogTest, ConstructorKeepsDialogDefaults) {
    cMNJoinDialog dialog;
    EXPECT_FALSE(dialog.isActive());
    EXPECT_TRUE(dialog.isEnabled());
    EXPECT_TRUE(dialog.isVisible());
    EXPECT_EQ(dialog.componentCount(), 0u);
    EXPECT_FALSE(dialog.closeRequested());
}

TEST(MNJoinDialogTest, LinkingIsEmptyAndPreservesGeometry) {
    cMNJoinDialog dialog;
    dialog.Init(10, 20, 100, 80, nullptr, 77);
    dialog.Linking();
    EXPECT_EQ(dialog.absX(), 10);
    EXPECT_EQ(dialog.absY(), 20);
    EXPECT_EQ(dialog.width(), 100);
    EXPECT_EQ(dialog.height(), 80);
    EXPECT_EQ(dialog.id(), 77);
}

TEST(MNJoinDialogTest, LinkingPreservesActiveState) {
    cMNJoinDialog dialog;
    dialog.SetActive(true);
    dialog.Linking();
    EXPECT_TRUE(dialog.isActive());
}

TEST(MNJoinDialogTest, LinkingPreservesDisableState) {
    cMNJoinDialog dialog;
    dialog.SetDisable(true);
    dialog.Linking();
    EXPECT_FALSE(dialog.isEnabled());
}

TEST(MNJoinDialogTest, LinkingPreservesOwnedChildren) {
    cMNJoinDialog dialog;
    auto child = std::make_unique<cStatic>();
    child->Init(0, 0, 20, 10, nullptr, 91);
    cStatic* raw = child.get();
    dialog.Add(std::move(child));
    dialog.Linking();
    EXPECT_EQ(dialog.componentCount(), 1u);
    EXPECT_EQ(dialog.findWindowById(91), raw);
}

TEST(MNJoinDialogTest, ActionEventIsEmptyForZeroInput) {
    cMNJoinDialog dialog;
    dialog.OnActionEvent(0, nullptr, 0);
    EXPECT_FALSE(dialog.closeRequested());
    EXPECT_EQ(dialog.componentCount(), 0u);
}

TEST(MNJoinDialogTest, ActionEventIgnoresAllLegacyArguments) {
    cMNJoinDialog dialog;
    int payload = 42;
    dialog.OnActionEvent(-123, &payload, 0xFFFFFFFFu);
    EXPECT_EQ(payload, 42);
    EXPECT_FALSE(dialog.closeRequested());
}

TEST(MNJoinDialogTest, ActionEventPreservesActiveAndDisableState) {
    cMNJoinDialog dialog;
    dialog.SetActive(true);
    dialog.SetDisable(true);
    dialog.OnActionEvent(999, nullptr, 4);
    EXPECT_TRUE(dialog.isActive());
    EXPECT_FALSE(dialog.isEnabled());
}

TEST(MNJoinDialogTest, ActionEventPreservesGeometryAndIdentity) {
    cMNJoinDialog dialog;
    dialog.Init(5, 6, 70, 80, nullptr, 101);
    dialog.OnActionEvent(1, nullptr, 64);
    EXPECT_EQ(dialog.absX(), 5);
    EXPECT_EQ(dialog.absY(), 6);
    EXPECT_EQ(dialog.width(), 70);
    EXPECT_EQ(dialog.height(), 80);
    EXPECT_EQ(dialog.id(), 101);
}

TEST(MNJoinDialogTest, RepeatedLegacyCallsRemainIdempotent) {
    cMNJoinDialog dialog;
    dialog.Init(1, 2, 3, 4, nullptr, 5);
    for (int index = 0; index < 10; ++index) {
        dialog.Linking();
        dialog.OnActionEvent(index, &dialog, static_cast<std::uint32_t>(index));
    }
    EXPECT_EQ(dialog.absX(), 1);
    EXPECT_EQ(dialog.absY(), 2);
    EXPECT_EQ(dialog.id(), 5);
    EXPECT_FALSE(dialog.closeRequested());
}

TEST(MNJoinDialogTest, DestructionThroughDialogBaseIsSafe) {
    std::unique_ptr<cDialog> dialog = std::make_unique<cMNJoinDialog>();
    dialog->SetActive(true);
    EXPECT_TRUE(dialog->isActive());
}
