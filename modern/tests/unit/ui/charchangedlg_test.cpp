//
// Unit tests for mxh::ui::cCharChangeDlg (Phase C Batch 2.32 port).
//
// Locks down the 1:1 surface of the character-change dialog (used by
// the kind-change item: hair/face/sex/height/width picker).
//
// Coverage:
//   * Constants (kHairTypeMax=4, kFaceTypeMax=4, kChatGenderMale=1180,
//     kChatGenderFemale=1181, kChatHairFormat=1182, kChatFaceFormat=1183,
//     kItemTable* enums, kSexButtonCount=2, WindowID enums)
//   * Inherits cDialog; non-copyable
//   * Default-constructed state: zeroed info, null members
//   * Linking() resolves children via cDialog::findWindowById
//   * Linking() resets state to zero
//   * SetControlsForTest sets the 8 control pointers
//   * SetCallbacks captures the 8 function pointers + userdata
//   * SetShapeChange (no-op under shape-change mode for ChangeSexType)
//   * SetCharacterInfo formats gender/hair/face from chat callback,
//     applies height/width via SetCurRate, and invokes the
//     SetHeroCharChangeInfo + TriggerCharacterPartChange callbacks
//   * ChangeSexType toggles gender 0<->1 (skipped under shape-change)
//   * ChangeHairType wraps at min/max (0..4) — no shape-change gate (1:1)
//   * ChangeFaceType wraps at min/max (0..4) — no shape-change gate (1:1)
//   * Reset(true) re-publishes current info via SetHeroCharChangeInfo
//   * Reset(false) restores backup + triggers part change
//   * CharacterChangeSyn invokes send callback with itemPos + info,
//     then SetActive(false)
//   * SetActive(true) with shape-change=FALSE enables sex buttons
//     and height/width guages
//   * SetActive(true) under shape-change=TRUE disables sex buttons
//     and height/width guages
//   * SetActive(false) calls SetItemTableDisabled(FALSE, ...) for
//     all 4 tables (inverted)
//   * Process() reads curRate from height/width, computes new
//     bH/bW, and calls SetHeroScale when changed
//   * Process() is a no-op when bShapeChange=true
//   * Process() is a no-op when height/width is null
//   * cGuageBar port: rate accessors round-trip through InitValue
//     and SetCurRate mirrors into cGuagen::SetValue
//

#include "mxh/ui/charchangedlg.hpp"
#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cGuageBar.hpp"
#include "mxh/ui/cStatic.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

using mxh::ui::cCharChangeDlg;
using mxh::ui::CharacterChangeInfo;
using mxh::ui::cButton;
using mxh::ui::cGuageBar;
using mxh::ui::cStatic;

namespace {

// Combined capture struct (single userData passed to all callbacks).
struct CallbackCapture {
    // SetHeroCharChangeInfo
    int setInfoCalls = 0;
    CharacterChangeInfo lastSetInfo{};
    // TriggerCharacterPartChange
    int partChangeCalls = 0;
    // SetHeroScale
    int scaleCalls = 0;
    float lastScaleX = 0.0f;
    float lastScaleY = 0.0f;
    float lastScaleZ = 0.0f;
    // SendCharacterChange
    int sendCalls = 0;
    std::uint32_t lastSendPos = 0;
    CharacterChangeInfo lastSendInfo{};
    // SetItemTableDisabled
    int tableCalls = 0;
    std::int32_t lastTableId = -1;
    bool lastDisabled = false;
    // EndObjectState
    int endStateCalls = 0;
};

struct Controls {
    cStatic name;
    cStatic sex;
    cStatic hair;
    cStatic face;
    cButton sexBtn0;
    cButton sexBtn1;
    cGuageBar height;
    cGuageBar width;

    void Attach(cCharChangeDlg& dlg) {
        dlg.SetControlsForTest(&name, &sex, &hair, &face,
                               &sexBtn0, &sexBtn1,
                               &height, &width);
    }
};

const char* StubChatFormat(std::int32_t messageId, void* /*userData*/) {
    switch (messageId) {
    case cCharChangeDlg::kChatGenderMale:   return "Male";
    case cCharChangeDlg::kChatGenderFemale: return "Female";
    case cCharChangeDlg::kChatHairFormat:   return "Hair#%d";
    case cCharChangeDlg::kChatFaceFormat:   return "Face#%d";
    default:                                return nullptr;
    }
}

const char* StubHeroName() {
    return "Hero";
}

void CaptureSetItemTableDisabled(bool disabled, std::int32_t tableId,
                                 void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return;
    ++cap->tableCalls;
    cap->lastDisabled = disabled;
    cap->lastTableId = tableId;
}

void CaptureEndObjectState(void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return;
    ++cap->endStateCalls;
}

void CaptureSetHeroCharChangeInfo(const CharacterChangeInfo& info,
                                  void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return;
    ++cap->setInfoCalls;
    cap->lastSetInfo = info;
}

void CaptureTriggerPartChange(void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return;
    ++cap->partChangeCalls;
}

void CaptureSetHeroScale(float x, float y, float z, void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return;
    ++cap->scaleCalls;
    cap->lastScaleX = x;
    cap->lastScaleY = y;
    cap->lastScaleZ = z;
}

void CaptureSendCharacterChange(std::uint32_t pos,
                                const CharacterChangeInfo& info,
                                void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return;
    ++cap->sendCalls;
    cap->lastSendPos = pos;
    cap->lastSendInfo = info;
}

void InstallCallbacks(cCharChangeDlg& d, CallbackCapture& cap) {
    d.SetCallbacks(StubChatFormat,
                   CaptureSetItemTableDisabled,
                   CaptureEndObjectState,
                   StubHeroName,
                   CaptureSetHeroCharChangeInfo,
                   CaptureTriggerPartChange,
                   CaptureSetHeroScale,
                   CaptureSendCharacterChange,
                   &cap);
}

}  // namespace

// --------------------------------------------------------------------------
// Smoke / static checks
// --------------------------------------------------------------------------

TEST(CharChangeDlgTest, InheritsDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<mxh::ui::cDialog, cCharChangeDlg>);
    static_assert(!std::is_copy_constructible_v<cCharChangeDlg>);
    static_assert(!std::is_copy_assignable_v<cCharChangeDlg>);
    SUCCEED();
}

TEST(CharChangeDlgTest, ConstantsMatchPreprocessedLegacyValues) {
    EXPECT_EQ(cCharChangeDlg::kHairTypeMin, 0);
    EXPECT_EQ(cCharChangeDlg::kHairTypeMax, 4);
    EXPECT_EQ(cCharChangeDlg::kFaceTypeMin, 0);
    EXPECT_EQ(cCharChangeDlg::kFaceTypeMax, 4);
    EXPECT_EQ(cCharChangeDlg::kHeightRateMultiplier, 5);
    EXPECT_EQ(cCharChangeDlg::kChatGenderMale, 1180);
    EXPECT_EQ(cCharChangeDlg::kChatGenderFemale, 1181);
    EXPECT_EQ(cCharChangeDlg::kChatHairFormat, 1182);
    EXPECT_EQ(cCharChangeDlg::kChatFaceFormat, 1183);
    EXPECT_EQ(cCharChangeDlg::kItemTableInventory, 0);
    EXPECT_EQ(cCharChangeDlg::kItemTablePyoguk, 1);
    EXPECT_EQ(cCharChangeDlg::kItemTableMunpaWarehouse, 2);
    EXPECT_EQ(cCharChangeDlg::kItemTableShop, 3);
    EXPECT_EQ(cCharChangeDlg::kHairTypeCount, 5u);
    EXPECT_EQ(cCharChangeDlg::kFaceTypeCount, 5u);
    EXPECT_EQ(cCharChangeDlg::kSexButtonCount, 2u);
    EXPECT_EQ(cCharChangeDlg::kChgDlgId, 1436);
    EXPECT_EQ(cCharChangeDlg::kNameId, 1437);
    EXPECT_EQ(cCharChangeDlg::kSexId, 1440);
    EXPECT_EQ(cCharChangeDlg::kHairId, 1441);
    EXPECT_EQ(cCharChangeDlg::kFaceId, 1442);
    EXPECT_EQ(cCharChangeDlg::kSexButton0Id, 1443);
    EXPECT_EQ(cCharChangeDlg::kSexButton1Id, 1444);
    EXPECT_EQ(cCharChangeDlg::kHeightId, 1449);
    EXPECT_EQ(cCharChangeDlg::kWidthId, 1450);
}

TEST(CharChangeDlgTest, CharacterChangeInfoDefaultsAreZeroed) {
    CharacterChangeInfo info{};
    EXPECT_EQ(info.gender, 0u);
    EXPECT_EQ(info.hairType, 0u);
    EXPECT_EQ(info.faceType, 0u);
    EXPECT_FLOAT_EQ(info.height, 1.0f);
    EXPECT_FLOAT_EQ(info.width, 1.0f);
}

TEST(CharChangeDlgTest, DefaultConstructorLeavesAllControlsNull) {
    cCharChangeDlg d;
    EXPECT_EQ(d.nameStatic(), nullptr);
    EXPECT_EQ(d.sexStatic(), nullptr);
    EXPECT_EQ(d.hairStatic(), nullptr);
    EXPECT_EQ(d.faceStatic(), nullptr);
    EXPECT_EQ(d.sexButton(0), nullptr);
    EXPECT_EQ(d.sexButton(1), nullptr);
    EXPECT_EQ(d.heightGuage(), nullptr);
    EXPECT_EQ(d.widthGuage(), nullptr);
    EXPECT_EQ(d.itemPos(), 0u);
    EXPECT_FALSE(d.shapeChange());
    CharacterChangeInfo info = d.info();
    EXPECT_EQ(info.gender, 0u);
    EXPECT_EQ(info.hairType, 0u);
    EXPECT_EQ(info.faceType, 0u);
    EXPECT_FLOAT_EQ(info.height, 1.0f);
    EXPECT_FLOAT_EQ(info.width, 1.0f);
}

TEST(CharChangeDlgTest, SexButtonOutOfRangeReturnsNull) {
    cCharChangeDlg d;
    EXPECT_EQ(d.sexButton(2), nullptr);
    EXPECT_EQ(d.sexButton(99), nullptr);
}

TEST(CharChangeDlgTest, BackupInfoMatchesInfoDefault) {
    cCharChangeDlg d;
    CharacterChangeInfo backup = d.backupInfo();
    CharacterChangeInfo info = d.info();
    EXPECT_EQ(backup.gender, info.gender);
    EXPECT_EQ(backup.hairType, info.hairType);
    EXPECT_EQ(backup.faceType, info.faceType);
    EXPECT_FLOAT_EQ(backup.height, info.height);
    EXPECT_FLOAT_EQ(backup.width, info.width);
}

// --------------------------------------------------------------------------
// Linking
// --------------------------------------------------------------------------

TEST(CharChangeDlgTest, LinkingResetsAllState) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    d.SetItemInfo(123);
    d.SetShapeChange(true);

    d.Linking();

    EXPECT_EQ(d.nameStatic(), nullptr);
    EXPECT_EQ(d.sexStatic(), nullptr);
    EXPECT_EQ(d.hairStatic(), nullptr);
    EXPECT_EQ(d.faceStatic(), nullptr);
    EXPECT_EQ(d.sexButton(0), nullptr);
    EXPECT_EQ(d.sexButton(1), nullptr);
    EXPECT_EQ(d.heightGuage(), nullptr);
    EXPECT_EQ(d.widthGuage(), nullptr);
    EXPECT_EQ(d.itemPos(), 0u);
    EXPECT_FALSE(d.shapeChange());
}

TEST(CharChangeDlgTest, LinkingFromChildLookupUsesFindWindowById) {
    cCharChangeDlg d;
    auto name  = std::make_unique<cStatic>();
    auto sex   = std::make_unique<cStatic>();
    auto hair  = std::make_unique<cStatic>();
    auto face  = std::make_unique<cStatic>();
    auto btn0  = std::make_unique<cButton>();
    auto btn1  = std::make_unique<cButton>();
    auto hgt   = std::make_unique<cGuageBar>();
    auto wid   = std::make_unique<cGuageBar>();

    name->Init(0, 0, 0, 0, nullptr, cCharChangeDlg::kNameId);
    sex->Init(0, 0, 0, 0, nullptr, cCharChangeDlg::kSexId);
    hair->Init(0, 0, 0, 0, nullptr, cCharChangeDlg::kHairId);
    face->Init(0, 0, 0, 0, nullptr, cCharChangeDlg::kFaceId);
    btn0->Init(0, 0, 0, 0, nullptr, nullptr, nullptr, [](std::int32_t, void*){}, nullptr,
               cCharChangeDlg::kSexButton0Id);
    btn1->Init(0, 0, 0, 0, nullptr, nullptr, nullptr, [](std::int32_t, void*){}, nullptr,
               cCharChangeDlg::kSexButton1Id);
    // cGuageBar (cGuagen subclass) inherits cWindow::Init, so we set
    // its id via Init. InitGuageBar() sets the axis interval.
    hgt->Init(0, 0, 0, 0, nullptr, cCharChangeDlg::kHeightId);
    hgt->InitGuageBar(100, false);
    wid->Init(0, 0, 0, 0, nullptr, cCharChangeDlg::kWidthId);
    wid->InitGuageBar(100, false);

    cStatic* nameRaw = name.get();
    cStatic* sexRaw  = sex.get();
    cStatic* hairRaw = hair.get();
    cStatic* faceRaw = face.get();
    cButton* btn0Raw = btn0.get();
    cButton* btn1Raw = btn1.get();
    cGuageBar* hgtRaw = hgt.get();
    cGuageBar* widRaw = wid.get();

    d.Add(std::move(name));
    d.Add(std::move(sex));
    d.Add(std::move(hair));
    d.Add(std::move(face));
    d.Add(std::move(btn0));
    d.Add(std::move(btn1));
    d.Add(std::move(hgt));
    d.Add(std::move(wid));

    d.Linking();

    EXPECT_EQ(d.nameStatic(), nameRaw);
    EXPECT_EQ(d.sexStatic(),  sexRaw);
    EXPECT_EQ(d.hairStatic(), hairRaw);
    EXPECT_EQ(d.faceStatic(), faceRaw);
    EXPECT_EQ(d.sexButton(0), btn0Raw);
    EXPECT_EQ(d.sexButton(1), btn1Raw);
    EXPECT_EQ(d.heightGuage(), hgtRaw);
    EXPECT_EQ(d.widthGuage(),  widRaw);
}

// --------------------------------------------------------------------------
// SetControlsForTest / SetCallbacks / accessors
// --------------------------------------------------------------------------

TEST(CharChangeDlgTest, SetControlsForTestAssignsAllPointers) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    EXPECT_EQ(d.nameStatic(),    &c.name);
    EXPECT_EQ(d.sexStatic(),     &c.sex);
    EXPECT_EQ(d.hairStatic(),    &c.hair);
    EXPECT_EQ(d.faceStatic(),    &c.face);
    EXPECT_EQ(d.sexButton(0),   &c.sexBtn0);
    EXPECT_EQ(d.sexButton(1),   &c.sexBtn1);
    EXPECT_EQ(d.heightGuage(),  &c.height);
    EXPECT_EQ(d.widthGuage(),   &c.width);
}

TEST(CharChangeDlgTest, SetShapeChangeAndSetItemInfoAccessors) {
    cCharChangeDlg d;
    d.SetItemInfo(42);
    EXPECT_EQ(d.itemPos(), 42u);
    d.SetShapeChange(true);
    EXPECT_TRUE(d.shapeChange());
    d.SetShapeChange(false);
    EXPECT_FALSE(d.shapeChange());
}

// --------------------------------------------------------------------------
// SetCharacterInfo
// --------------------------------------------------------------------------

TEST(CharChangeDlgTest, SetCharacterInfoFormatsFromChatCallback) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);

    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo in{};
    in.gender = 1;
    in.hairType = 2;
    in.faceType = 3;
    in.height = 1.0f;
    in.width = 1.0f;

    d.SetCharacterInfo(in);

    EXPECT_EQ(c.sex.GetStaticText(),    "Female");
    EXPECT_EQ(c.hair.GetStaticText(),   "Hair#3");
    EXPECT_EQ(c.face.GetStaticText(),   "Face#4");
    EXPECT_EQ(c.name.GetStaticText(),   "Hero");
    EXPECT_EQ(cap.setInfoCalls, 1);
    EXPECT_EQ(cap.partChangeCalls, 1);
    EXPECT_EQ(cap.lastSetInfo.gender, 1u);
    EXPECT_EQ(cap.lastSetInfo.hairType, 2u);
    EXPECT_EQ(cap.lastSetInfo.faceType, 3u);
    EXPECT_EQ(d.info().gender, 1u);
    EXPECT_EQ(d.info().hairType, 2u);
    EXPECT_EQ(d.info().faceType, 3u);
    EXPECT_EQ(d.backupInfo().gender, 1u);
    EXPECT_EQ(d.backupInfo().hairType, 2u);
    EXPECT_EQ(d.backupInfo().faceType, 3u);
}

TEST(CharChangeDlgTest, SetCharacterInfoMaleGenderFormatsMale) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo in{};
    in.gender = 0;
    in.hairType = 0;
    in.faceType = 0;
    d.SetCharacterInfo(in);

    EXPECT_EQ(c.sex.GetStaticText(), "Male");
    EXPECT_EQ(c.hair.GetStaticText(), "Hair#1");
    EXPECT_EQ(c.face.GetStaticText(), "Face#1");
}

TEST(CharChangeDlgTest, SetCharacterInfoWithNullChatCallbackLeavesEmptyText) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    d.SetCallbacks(nullptr,
                   CaptureSetItemTableDisabled,
                   CaptureEndObjectState,
                   StubHeroName,
                   CaptureSetHeroCharChangeInfo,
                   CaptureTriggerPartChange,
                   CaptureSetHeroScale,
                   CaptureSendCharacterChange,
                   nullptr);

    CharacterChangeInfo in{};
    in.gender = 0;
    in.hairType = 0;
    in.faceType = 0;
    d.SetCharacterInfo(in);

    EXPECT_EQ(c.sex.GetStaticText(),  "");
    EXPECT_EQ(c.hair.GetStaticText(), "");
    EXPECT_EQ(c.face.GetStaticText(), "");
    EXPECT_EQ(c.name.GetStaticText(), "Hero");
}

TEST(CharChangeDlgTest, SetCharacterInfoWithNullHeroNameClearsName) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    d.SetCallbacks(StubChatFormat,
                   CaptureSetItemTableDisabled,
                   CaptureEndObjectState,
                   nullptr,
                   CaptureSetHeroCharChangeInfo,
                   CaptureTriggerPartChange,
                   CaptureSetHeroScale,
                   CaptureSendCharacterChange,
                   nullptr);

    CharacterChangeInfo in{};
    d.SetCharacterInfo(in);
    EXPECT_EQ(c.name.GetStaticText(), "");
}

TEST(CharChangeDlgTest, SetCharacterInfoAppliesHeightAndWidthRate) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    c.height.InitGuageBar(100, false);
    c.width.InitGuageBar(100, false);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo in{};
    in.height = 0.9f;
    in.width = 1.1f;
    d.SetCharacterInfo(in);

    // (0.9 - 0.9) * 5 = 0.0
    // (1.1 - 0.9) * 5 = 1.0
    EXPECT_FLOAT_EQ(c.height.GetCurRate(), 0.0f);
    EXPECT_FLOAT_EQ(c.width.GetCurRate(), 1.0f);
}

TEST(CharChangeDlgTest, SetCharacterInfoWithNullHeightWidthSkipsGuageWrite) {
    cCharChangeDlg d;
    Controls c;
    c.sex.Init(0, 0, 0, 0, nullptr, 0);
    c.hair.Init(0, 0, 0, 0, nullptr, 0);
    c.face.Init(0, 0, 0, 0, nullptr, 0);
    d.SetControlsForTest(&c.name, &c.sex, &c.hair, &c.face,
                        &c.sexBtn0, &c.sexBtn1, nullptr, nullptr);
    CallbackCapture cap;
    InstallCallbacks(d, cap);
    CharacterChangeInfo in{};
    d.SetCharacterInfo(in);
    EXPECT_EQ(c.sex.GetStaticText(), "Male");
}

// --------------------------------------------------------------------------
// ChangeSexType / ChangeHairType / ChangeFaceType
// --------------------------------------------------------------------------

TEST(CharChangeDlgTest, ChangeSexTypeTogglesGender) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo in{};
    d.SetCharacterInfo(in);
    EXPECT_EQ(d.info().gender, 0u);

    d.ChangeSexType(false);
    EXPECT_EQ(d.info().gender, 1u);
    EXPECT_EQ(c.sex.GetStaticText(), "Female");

    d.ChangeSexType(false);
    EXPECT_EQ(d.info().gender, 0u);
    EXPECT_EQ(c.sex.GetStaticText(), "Male");
}

TEST(CharChangeDlgTest, ChangeSexTypeIsNoOpUnderShapeChange) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);
    CharacterChangeInfo in{};
    d.SetCharacterInfo(in);
    d.SetShapeChange(true);
    d.ChangeSexType(false);
    EXPECT_EQ(d.info().gender, 0u);
}

TEST(CharChangeDlgTest, ChangeHairTypeWrapsAtMinMax) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo in{};
    in.hairType = 0;
    d.SetCharacterInfo(in);

    d.ChangeHairType(false);
    EXPECT_EQ(d.info().hairType, 1u);
    d.ChangeHairType(false);
    EXPECT_EQ(d.info().hairType, 2u);
    d.ChangeHairType(false);
    EXPECT_EQ(d.info().hairType, 3u);
    d.ChangeHairType(false);
    EXPECT_EQ(d.info().hairType, 4u);
    d.ChangeHairType(false);
    EXPECT_EQ(d.info().hairType, 0u);

    d.ChangeHairType(true);
    EXPECT_EQ(d.info().hairType, 4u);
    d.ChangeHairType(true);
    EXPECT_EQ(d.info().hairType, 3u);
    d.ChangeHairType(true);
    EXPECT_EQ(d.info().hairType, 2u);
    d.ChangeHairType(true);
    EXPECT_EQ(d.info().hairType, 1u);
    d.ChangeHairType(true);
    EXPECT_EQ(d.info().hairType, 0u);
}

TEST(CharChangeDlgTest, ChangeHairTypeIsNotGatedByShapeChange) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);
    CharacterChangeInfo in{};
    in.hairType = 2;
    d.SetCharacterInfo(in);
    d.SetShapeChange(true);
    // 1:1 with legacy: ChangeHairType does NOT check m_bShapeChange.
    d.ChangeHairType(false);
    EXPECT_EQ(d.info().hairType, 3u);
}

TEST(CharChangeDlgTest, ChangeFaceTypeWrapsAtMinMax) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo in{};
    in.faceType = 0;
    d.SetCharacterInfo(in);

    d.ChangeFaceType(false);
    EXPECT_EQ(d.info().faceType, 1u);
    d.ChangeFaceType(false);
    EXPECT_EQ(d.info().faceType, 2u);
    d.ChangeFaceType(false);
    EXPECT_EQ(d.info().faceType, 3u);
    d.ChangeFaceType(false);
    EXPECT_EQ(d.info().faceType, 4u);
    d.ChangeFaceType(false);
    EXPECT_EQ(d.info().faceType, 0u);

    d.ChangeFaceType(true);
    EXPECT_EQ(d.info().faceType, 4u);
    d.ChangeFaceType(true);
    EXPECT_EQ(d.info().faceType, 3u);
}

TEST(CharChangeDlgTest, ChangeFaceTypeIsNotGatedByShapeChange) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);
    CharacterChangeInfo in{};
    in.faceType = 1;
    d.SetCharacterInfo(in);
    d.SetShapeChange(true);
    // 1:1 with legacy: ChangeFaceType does NOT check m_bShapeChange.
    d.ChangeFaceType(false);
    EXPECT_EQ(d.info().faceType, 2u);
}

// --------------------------------------------------------------------------
// Reset
// --------------------------------------------------------------------------

TEST(CharChangeDlgTest, ResetSaveTrueRePublishesCurrentInfo) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo in{};
    in.gender = 1;
    in.hairType = 2;
    in.faceType = 3;
    d.SetCharacterInfo(in);
    const int callsAfterSet = cap.setInfoCalls;

    d.SetItemInfo(99);
    d.SetShapeChange(true);
    d.Reset(true);

    EXPECT_EQ(cap.setInfoCalls, callsAfterSet + 1);
    EXPECT_EQ(cap.lastSetInfo.gender, 1u);
    EXPECT_EQ(cap.lastSetInfo.hairType, 2u);
    EXPECT_EQ(cap.lastSetInfo.faceType, 3u);
    EXPECT_EQ(d.itemPos(), 0u);
    EXPECT_FALSE(d.shapeChange());
    CharacterChangeInfo info = d.info();
    EXPECT_EQ(info.gender, 0u);
    EXPECT_EQ(info.hairType, 0u);
    EXPECT_EQ(info.faceType, 0u);
}

TEST(CharChangeDlgTest, ResetSaveFalseRestoresBackupAndTriggersPartChange) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo backup{};
    backup.gender = 0;
    backup.hairType = 1;
    backup.faceType = 2;
    d.SetCharacterInfo(backup);

    d.ChangeSexType(false);
    d.ChangeHairType(false);
    const int callsAfterMutate = cap.setInfoCalls;
    const int partChangeAfterMutate = cap.partChangeCalls;

    d.Reset(false);

    EXPECT_EQ(cap.setInfoCalls, callsAfterMutate + 1);
    EXPECT_EQ(cap.lastSetInfo.gender, 0u);
    EXPECT_EQ(cap.lastSetInfo.hairType, 1u);
    EXPECT_EQ(cap.lastSetInfo.faceType, 2u);
    EXPECT_EQ(cap.partChangeCalls, partChangeAfterMutate + 1);
    EXPECT_EQ(d.itemPos(), 0u);
    EXPECT_FALSE(d.shapeChange());
}

// --------------------------------------------------------------------------
// CharacterChangeSyn
// --------------------------------------------------------------------------

TEST(CharChangeDlgTest, CharacterChangeSynInvokesSendCallback) {
    cCharChangeDlg d;
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    d.SetItemInfo(77);
    CharacterChangeInfo in{};
    in.gender = 1;
    in.hairType = 2;
    d.SetCharacterInfo(in);

    d.CharacterChangeSyn();

    EXPECT_EQ(cap.sendCalls, 1);
    EXPECT_EQ(cap.lastSendPos, 77u);
    EXPECT_EQ(cap.lastSendInfo.gender, 1u);
    EXPECT_EQ(cap.lastSendInfo.hairType, 2u);
    EXPECT_FALSE(d.isActive());
}

TEST(CharChangeDlgTest, CharacterChangeSynWithNullSendCallbackDeactivates) {
    cCharChangeDlg d;
    d.SetCallbacks(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    d.CharacterChangeSyn();
    EXPECT_FALSE(d.isActive());
}

// --------------------------------------------------------------------------
// SetActive path
// --------------------------------------------------------------------------

TEST(CharChangeDlgTest, SetActiveTrueEnablesShapeControls) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    d.SetActive(true);
    EXPECT_TRUE(c.sexBtn0.isActive());
    EXPECT_TRUE(c.sexBtn1.isActive());
    EXPECT_TRUE(c.height.isActive());
    EXPECT_TRUE(c.width.isActive());
}

TEST(CharChangeDlgTest, SetActiveTrueUnderShapeChangeDisablesControls) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    d.SetShapeChange(true);
    d.SetActive(true);
    EXPECT_FALSE(c.sexBtn0.isActive());
    EXPECT_FALSE(c.sexBtn1.isActive());
    EXPECT_FALSE(c.height.isActive());
    EXPECT_FALSE(c.width.isActive());
}

TEST(CharChangeDlgTest, SetActiveFalseCallsItemTableDisabledForAllTables) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    d.SetActive(false);
    EXPECT_EQ(cap.tableCalls, 4);
    EXPECT_EQ(cap.lastTableId, cCharChangeDlg::kItemTableShop);
    // 1:1 with legacy: SetActive(false) calls SetDisableDialog(FALSE, ...)
    // so disabled=false.
    EXPECT_FALSE(cap.lastDisabled);
}

TEST(CharChangeDlgTest, SetActiveFalseSkipsCallbacksWhenUnset) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    d.SetActive(false);
    SUCCEED();
}

TEST(CharChangeDlgTest, SetActiveTrueAlsoCallsBaseDialogSetActive) {
    cCharChangeDlg d;
    d.SetActive(true);
    EXPECT_TRUE(d.isActive());
    d.SetActive(false);
    EXPECT_FALSE(d.isActive());
}

// --------------------------------------------------------------------------
// Process
// --------------------------------------------------------------------------

TEST(CharChangeDlgTest, ProcessAppliesHeightAndWidthRate) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    c.height.InitGuageBar(100, false);
    c.width.InitGuageBar(100, false);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo in{};
    in.height = 0.9f;
    in.width = 0.9f;
    d.SetCharacterInfo(in);

    // Now mutate the guage rates (after SetCharacterInfo which sets
    // them from the info height/width).
    c.height.SetCurRate(0.5f);
    c.width.SetCurRate(0.5f);

    d.Process();
    EXPECT_EQ(cap.scaleCalls, 1);
    EXPECT_FLOAT_EQ(cap.lastScaleX, 0.9f + 0.5f * 0.2f);  // z = w
    EXPECT_FLOAT_EQ(cap.lastScaleY, 0.9f + 0.5f * 0.2f);
    EXPECT_FLOAT_EQ(cap.lastScaleZ, 0.9f + 0.5f * 0.2f);
    EXPECT_FLOAT_EQ(d.info().height, 0.9f + 0.5f * 0.2f);
    EXPECT_FLOAT_EQ(d.info().width,  0.9f + 0.5f * 0.2f);
}

TEST(CharChangeDlgTest, ProcessIsNoOpWhenValuesUnchanged) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    c.height.InitGuageBar(100, false);
    c.width.InitGuageBar(100, false);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    CharacterChangeInfo in{};
    in.height = 0.9f;
    in.width = 0.9f;
    d.SetCharacterInfo(in);
    // After SetCharacterInfo, rates are 0.0, which corresponds to
    // 0.9 + 0 * 0.2 = 0.9 — same as info. So Process() is no-op.
    d.Process();
    EXPECT_EQ(cap.scaleCalls, 0);
}

TEST(CharChangeDlgTest, ProcessIsNoOpWhenShapeChange) {
    cCharChangeDlg d;
    Controls c;
    c.Attach(d);
    c.height.InitGuageBar(100, false);
    c.width.InitGuageBar(100, false);
    CallbackCapture cap;
    InstallCallbacks(d, cap);

    d.SetShapeChange(true);
    d.Process();
    EXPECT_EQ(cap.scaleCalls, 0);
}

TEST(CharChangeDlgTest, ProcessIsNoOpWhenHeightWidthNull) {
    cCharChangeDlg d;
    Controls c;
    c.sex.Init(0, 0, 0, 0, nullptr, 0);
    d.SetControlsForTest(&c.name, &c.sex, &c.hair, &c.face,
                        &c.sexBtn0, &c.sexBtn1, nullptr, nullptr);
    CallbackCapture cap;
    InstallCallbacks(d, cap);
    d.Process();
    EXPECT_EQ(cap.scaleCalls, 0);
}

// --------------------------------------------------------------------------
// cGuageBar port (auxiliary — charchangedlg depends on it)
// --------------------------------------------------------------------------

TEST(CGuageBarTest, InitGuageBarStoresIntervalAndVertical) {
    cGuageBar bar;
    bar.InitGuageBar(200, true);
    EXPECT_EQ(bar.GetInterval(), 200);
    EXPECT_TRUE(bar.IsVertical());
}

TEST(CGuageBarTest, InitGuageBarClampsZeroIntervalToOne) {
    cGuageBar bar;
    bar.InitGuageBar(0, false);
    EXPECT_EQ(bar.GetInterval(), 1);
    EXPECT_EQ(bar.GetCurRate(), 0.0f);
}

TEST(CGuageBarTest, InitValueStoresMinMaxCur) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(0, 10, 5);
    EXPECT_EQ(bar.GetMinValue(), 0);
    EXPECT_EQ(bar.GetMaxValue(), 10);
    EXPECT_EQ(bar.GetCurValue(), 5);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.5f);
}

TEST(CGuageBarTest, SetCurRateRoundTripsThroughGetCurRate) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.SetCurRate(0.25f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.25f);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 25.0f);
}

TEST(CGuageBarTest, SetCurValueUpdatesBarRelPos) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(0, 10, 0);
    bar.SetCurValue(7);
    EXPECT_EQ(bar.GetCurValue(), 7);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.7f);
}

TEST(CGuageBarTest, SetGuageLockStoresFlag) {
    cGuageBar bar;
    bar.SetGuageLock(true, 0xFF112233u);
    EXPECT_TRUE(bar.IsLocked());
    EXPECT_EQ(bar.GetBarBtnColor(), 0xFF112233u);
}

TEST(CGuageBarTest, ActionEventReturnsZeroWhenInactive) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.SetActive(false);
    EXPECT_EQ(bar.ActionEvent(), 0u);
}

TEST(CGuageBarTest, ActionEventReturnsZeroWhenLocked) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.SetActive(true);
    bar.SetGuageLock(true, 0);
    EXPECT_EQ(bar.ActionEvent(), 0u);
}

TEST(CGuageBarTest, SetIntervalRepositionsBarRelPos) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(0, 10, 5);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 50.0f);
    bar.SetInterval(200);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 100.0f);
}

TEST(CGuageBarTest, AddStoresButtonAndShrinksInterval) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    cButton btn;
    btn.Init(0, 0, 10, 10, nullptr, nullptr, nullptr, [](std::int32_t, void*){}, nullptr, 0);
    bar.Add(&btn);
    EXPECT_EQ(bar.GetInterval(), 90);  // 100 - 10 (btn width)
}
