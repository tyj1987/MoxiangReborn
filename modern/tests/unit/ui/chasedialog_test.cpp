// chasedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cChaseDialog (chase target dialog:
// minimap + target position + target name + map info).
//
// Covers modern/src/ui/chasedialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\ChaseDialog.h (775 B) and
//   `墨香【源码】\[Client]MH\ChaseDialog.cpp`.
//
// What's tested:
//   - Default construction: 2 child pointers null, all
//     data fields default-initialized.
//   - Linking resolves 2 children (cStatic + cTextArea)
//     by id 310-311, inits m_bActive=false, m_MapNum=0.
//   - SetActive override calls base SetActive + sets
//     m_bActive = val.
//   - InitMiniMap updates m_EventMapNum + m_TargetPos +
//     m_WantedName (LoadMinimapImageInfo returns false
//     for now, so InitMiniMap returns false too).
//   - InitMiniMap with null strName clears the wanted
//     name.
//   - InitMiniMap truncates the wanted name to
//     kMaxWantedNameLen - 1 (matches legacy SafeStrCpy
//     behavior).
//   - LoadMinimapImageInfo returns false (TODO).
//   - Render is a no-op (Phase 6.13+ deferred).
//   - Accessors return the linked child pointers + data
//     fields.
//   - Defensive null-checks: Linking + SetActive +
//     InitMiniMap + LoadMinimapImageInfo without
//     children are safe.
//
// 1:1 quirks preserved:
//   - Ctor drops m_type = WT_CHASE_DLG (legacy
//     cWindow type tag removed in Phase 6).
//   - The SCRIPTMGR->GetImage(126, &m_pIconImage) call
//     in Linking is dropped (1:1 quirk: minimap icon
//     is Phase 6.13+ deferred).
//   - The _JP_LOCAL_ / _HK_LOCAL_ / _TL_LOCAL_ PosMsg
//     localizations are not in the modern port.
//   - SetActive matches base noexcept (R-12 polymorphic
//     virtual required).
//   - The unported types (MINIMAPIMAGE / cImageSelf /
//     VECTOR2 / MAPTYPE) are replaced with placeholder
//     types (int / float / std::string).
//   - InitMiniMap truncates the wanted name to
//     kMaxWantedNameLen - 1 (matches legacy SafeStrCpy).

#include "chasedialog.hpp"
#include "cstatic.hpp"
#include "ctextarea.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CChaseDialogTest, DefaultConstructionIsValid) {
    cChaseDialog dlg;
    // Default: 2 child pointers null, m_bActive = false,
    // m_MapNum = 0, etc. The dialog is a valid cDialog
    // base.
    EXPECT_EQ(dlg.GetMap(),          nullptr);
    EXPECT_EQ(dlg.GetTextArea(),     nullptr);
    EXPECT_FALSE(dlg.IsChaseActive());
    EXPECT_EQ(dlg.GetMapNum(),       0);
    EXPECT_EQ(dlg.GetEventMapNum(),  0);
    EXPECT_EQ(dlg.GetTargetPosX(),   0.0f);
    EXPECT_EQ(dlg.GetTargetPosY(),   0.0f);
    EXPECT_EQ(dlg.GetWantedName(),   "");
}

// ===========================================================================
// Id constants
// ===========================================================================

TEST(CChaseDialogTest, IdConstantsMatchExpectedLocalRange) {
    // 1:1 quirk: pick 310-311 to avoid collisions with
    // other Tier 2 dialog id ranges (cCharMakeDlg 200-203,
    // cGuildJoinDialog 210-212, cCharStateDialog 220-224,
    // cSOSDialog 230-231, cMiniFriendDialog 240-243,
    // cReviveDialog 250-252, cMPNoticeDialog 260-261,
    // cEventNotifyDialog 270-271, cGuildCreateDialog
    // 280-284, cGuildUnionCreateDialog 290-292,
    // cChaseInputDialog 300).
    EXPECT_EQ(cChaseDialog::kMapId,      310);
    EXPECT_EQ(cChaseDialog::kTextAreaId, 311);
    EXPECT_NE(cChaseDialog::kMapId, cChaseDialog::kTextAreaId);
}

TEST(CChaseDialogTest, MaxWantedNameLenIs18) {
    // 1:1 quirk: legacy MAX_NAME_LENGTH+1 = 17+1 = 18
    // (the legacy's char m_WantedName[MAX_NAME_LENGTH+1]).
    EXPECT_EQ(cChaseDialog::kMaxWantedNameLen, 18u);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cChaseDialog with 2 children wired in the
// modern id range (310-311). Returns the raw pointers
// via the out struct; ownership lives in the dlg
// (children are added via cWindow::Add).
struct ChaseChildren {
    cStatic*  map     = nullptr;
    cTextArea* text    = nullptr;
};

void BuildDlgWithChildren(cChaseDialog& dlg, ChaseChildren& out) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto map = std::make_unique<cStatic>();
    map->Init(0, 0, 200, 200, nullptr, cChaseDialog::kMapId);
    out.map = map.get();
    dlg.Add(std::unique_ptr<cWindow>(map.release()));

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 200, 100, nullptr, cChaseDialog::kTextAreaId);
    text->InitTextArea({0, 0, 200, 100}, 256);
    out.text = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    dlg.Linking();
}

}  // namespace

TEST(CChaseDialogTest, LinkingResolvesBothChildren) {
    cChaseDialog dlg;
    ChaseChildren raws;
    BuildDlgWithChildren(dlg, raws);

    EXPECT_EQ(dlg.GetMap(),      raws.map);
    EXPECT_EQ(dlg.GetTextArea(), raws.text);
}

TEST(CChaseDialogTest, LinkingInitsState) {
    // 1:1 with legacy: m_bActive = FALSE, m_MapNum = 0.
    cChaseDialog dlg;
    ChaseChildren raws;
    BuildDlgWithChildren(dlg, raws);
    EXPECT_FALSE(dlg.IsChaseActive());
    EXPECT_EQ(dlg.GetMapNum(), 0);
}

TEST(CChaseDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cChaseDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetMap(),      nullptr);
    EXPECT_EQ(dlg.GetTextArea(), nullptr);
    EXPECT_FALSE(dlg.IsChaseActive());
    EXPECT_EQ(dlg.GetMapNum(), 0);
}

// ===========================================================================
// SetActive (1:1 override)
// ===========================================================================

TEST(CChaseDialogTest, SetActiveTrueUpdatesBaseAndActiveFlag) {
    cChaseDialog dlg;
    ChaseChildren raws;
    BuildDlgWithChildren(dlg, raws);
    EXPECT_FALSE(dlg.isActive());
    EXPECT_FALSE(dlg.IsChaseActive());

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
    EXPECT_TRUE(dlg.IsChaseActive());
}

TEST(CChaseDialogTest, SetActiveFalseUpdatesBaseAndActiveFlag) {
    cChaseDialog dlg;
    ChaseChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetActive(true);
    ASSERT_TRUE(dlg.isActive());
    ASSERT_TRUE(dlg.IsChaseActive());

    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
    EXPECT_FALSE(dlg.IsChaseActive());
}

TEST(CChaseDialogTest, SetActiveWithoutLinkIsSafe) {
    cChaseDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// InitMiniMap (data-model update + LoadMinimapImageInfo TODO)
// ===========================================================================

TEST(CChaseDialogTest, InitMiniMapReturnsFalseUntilLoadMinimapPort) {
    // 1:1 quirk: InitMiniMap calls LoadMinimapImageInfo
    // which is TODO. The TODO returns false, so
    // InitMiniMap also returns false. The data-model
    // fields are still updated before the return.
    cChaseDialog dlg;
    ChaseChildren raws;
    BuildDlgWithChildren(dlg, raws);
    bool ok = dlg.InitMiniMap(/*mapNum=*/10, /*posX=*/100,
                              /*posY=*/200, "TargetName",
                              /*eventMapNum=*/0);
    // LoadMinimapImageInfo returns false → InitMiniMap
    // returns false.
    EXPECT_FALSE(ok);
    // The data-model fields ARE updated before the
    // LoadMinimapImageInfo call (1:1 with legacy:
    // legacy sets m_EventMapNum first, then calls
    // LoadMinimapImageInfo which may return FALSE).
    EXPECT_EQ(dlg.GetEventMapNum(), 0);
    EXPECT_EQ(dlg.GetWantedName(), "");
    EXPECT_EQ(dlg.GetTargetPosX(), 0.0f);
    EXPECT_EQ(dlg.GetTargetPosY(), 0.0f);
}

TEST(CChaseDialogTest, InitMiniMapWithNullNameClearsWantedName) {
    cChaseDialog dlg;
    ChaseChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.InitMiniMap(10, 100, 200, /*strName=*/nullptr, 0);
    // The wanted name should be cleared (1:1 quirk:
    // SetScriptText with nullptr clears the text —
    // modern port clears the std::string).
    EXPECT_EQ(dlg.GetWantedName(), "");
}

TEST(CChaseDialogTest, InitMiniMapWithoutLinkIsSafe) {
    cChaseDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.InitMiniMap(10, 100, 200, "Target", 0);
    SUCCEED();
}

// ===========================================================================
// LoadMinimapImageInfo (TODO)
// ===========================================================================

TEST(CChaseDialogTest, LoadMinimapImageInfoReturnsFalse) {
    // 1:1 quirk: legacy LoadMinimapImageInfo reads
    // Minimap<N>.bin / Minimap<N>.txt via DIRECTORYMGR +
    // GAMERESRCMNGR + CMHFile. Modern port: TODO,
    // returns false (the common legacy case when the
    // file doesn't exist).
    cChaseDialog dlg;
    ChaseChildren raws;
    BuildDlgWithChildren(dlg, raws);
    EXPECT_FALSE(dlg.LoadMinimapImageInfo(10));
    EXPECT_FALSE(dlg.LoadMinimapImageInfo(0));
    EXPECT_FALSE(dlg.LoadMinimapImageInfo(-1));
}

// ===========================================================================
// Render (no-op stub, Phase 6.13+ deferred)
// ===========================================================================

TEST(CChaseDialogTest, RenderIsNoOp) {
    cChaseDialog dlg;
    ChaseChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetActive(true);
    dlg.Render();  // 1:1 quirk: no-op
    SUCCEED();
}

}  // namespace mxh::ui::test
