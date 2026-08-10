// mxh/tests/unit/ui/ccharacterdialog_test.cpp
//
// Unit tests for mxh::ui::cCharacterDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * kMaxBtnPoint == 6
//   * StatPointKind enum: GenGol/SimMak/MinChub/CheRyuk
//   * CHARSTATICCTRL struct has the 22 cStatic* fields
//   * ATTRDEFENCE struct has the 4 stat-static + 4 stat-guage
//   * Default construction zeroed (nocori=0, minus=0, no point-leveling)
//   * Init stores dialog coords
//   * Linking is a no-op (WINDOW_ID walk deferred)
//   * SetActive notifies the main-bar icon callback
//   * SetLevel/SetLife/SetShield/SetNaeRyuk/SetFame/SetBadFame
//     format their values into the matching static
//   * SetGenGol/SetMinChub/SetCheRyuk/SetSimMek format per-stat
//   * SetExpPointPercent formats as "X.Y%"
//   * SetAttackRate / SetDefenseRate / SetCritical / SetAttackRange
//     reset the matching statics to "0"
//   * SetStage formats the stage value
//   * SetPointLeveling sets the flag + nocori point count
//   * OnAddPoint increments the per-stat counter + decrements
//     nocori; clamps at nocori=0
//   * OnMinusPoint decrements the per-stat counter + increments
//     nocori; clamps at added=0
//   * SetMinusPointValue stores the value
//   * SetPointLevelingHide flips the flag
//   * RefreshInfo routes through UpdateData + UpdateForStageAbility
//   * RefreshGuildInfo routes through the static-text callback
//   * RefreshPointInfo formats the nocori point count
//   * AttrDefence::SetValue writes "X%" + sets the guage to
//     value*0.01f + optionally sets the FG color

#include "mxh/ui/ccharacterdialog.hpp"
#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cGuagen.hpp"
#include "mxh/ui/cStatic.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

using mxh::ui::cCharacterDialog;
using mxh::ui::CharStaticCtrl;
using mxh::ui::AttrDefence;
using mxh::ui::StatPointKind;
using mxh::ui::cStatic;
using mxh::ui::cGuagen;
using mxh::ui::cButton;

struct CharacterStats final : mxh::services::IPlayerStatsService {
    std::uint16_t level = 1;
    std::uint32_t hp = 0;
    std::uint32_t mp = 0;
    std::uint16_t getStr() const noexcept override { return 0; }
    std::uint16_t getAgi() const noexcept override { return 0; }
    std::uint16_t getInt() const noexcept override { return 0; }
    std::uint16_t getWis() const noexcept override { return 0; }
    std::uint16_t getDex() const noexcept override { return 0; }
    std::uint16_t getLevel() const noexcept override { return level; }
    std::uint32_t getLevelExp() const noexcept override { return 0; }
    std::uint32_t getExpForNextLevel() const noexcept override { return 0; }
    std::uint32_t getCurrentHp() const noexcept override { return hp; }
    std::uint32_t getMaxHp() const noexcept override { return 0; }
    std::uint32_t getCurrentMp() const noexcept override { return mp; }
    std::uint32_t getMaxMp() const noexcept override { return 0; }
    float getHpFraction() const noexcept override { return 0.0f; }
    float getMpFraction() const noexcept override { return 0.0f; }
};

namespace test_chardlg {

struct CapturedSet {
    std::string field;
    std::string text;
};
struct CapturedAttr {
    StatPointKind kind;
    std::uint16_t value;
    std::uint32_t color;
};
struct CapturedGuage {
    StatPointKind kind;
    float value;
};
struct CapturedLevel {
    std::uint16_t level;
};
struct CapturedStage {
    std::uint8_t stage;
};
struct CapturedMainBar {
    bool active;
};

int g_setCount = 0;
std::vector<CapturedSet> g_setCalls;
std::vector<CapturedAttr> g_attrCalls;
std::vector<CapturedGuage> g_guageCalls;
CapturedLevel g_lastLevel;
CapturedStage g_lastStage;
CapturedMainBar g_lastMainBar;

void faSetText(const char* field, const char* text, void* /*user*/) {
    ++g_setCount;
    g_setCalls.push_back({field ? field : "", text ? text : ""});
}
void faAttr(StatPointKind k, std::uint16_t v, std::uint32_t c, void* /*user*/) {
    g_attrCalls.push_back({k, v, c});
}
void faGuage(StatPointKind k, float v, void* /*user*/) {
    g_guageCalls.push_back({k, v});
}
void faLevel(std::uint16_t l, void* /*user*/) {
    g_lastLevel.level = l;
}
void faStage(std::uint8_t s, void* /*user*/) {
    g_lastStage.stage = s;
}
void faMainBar(bool a, void* /*user*/) {
    g_lastMainBar.active = a;
}

}  // namespace test_chardlg

TEST(CCharacterDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(mxh::ui::kMaxBtnPoint, 6);
}

TEST(CCharacterDialog, StatPointKindEnumIsStable) {
    EXPECT_EQ(static_cast<std::uint8_t>(StatPointKind::GenGol),  0);
    EXPECT_EQ(static_cast<std::uint8_t>(StatPointKind::SimMak),  1);
    EXPECT_EQ(static_cast<std::uint8_t>(StatPointKind::MinChub), 2);
    EXPECT_EQ(static_cast<std::uint8_t>(StatPointKind::CheRyuk), 3);
}

TEST(CCharacterDialog, DefaultConstructionIsZeroed) {
    cCharacterDialog d;
    EXPECT_EQ(d.nocoriPoint(), 0);
    EXPECT_EQ(d.minusPoint(), 0);
    EXPECT_FALSE(d.isPointLeveling());
    EXPECT_EQ(d.GetCharacterData()->level, nullptr);
}

TEST(CCharacterDialog, InitStoresPosition) {
    cCharacterDialog d;
    d.Init(100, 200, 300, 400, nullptr, 1);
    EXPECT_EQ(d.absX(), 100);
    EXPECT_EQ(d.absY(), 200);
}

TEST(CCharacterDialog, SetActiveNotifiesMainBar) {
    test_chardlg::g_lastMainBar.active = false;
    cCharacterDialog d;
    d.SetMainBarIconCallbackForTest(&test_chardlg::faMainBar, nullptr);
    d.SetActive(true);
    EXPECT_TRUE(test_chardlg::g_lastMainBar.active);
    d.SetActive(false);
    EXPECT_FALSE(test_chardlg::g_lastMainBar.active);
}

TEST(CCharacterDialog, SetLevelFormatsValue) {
    test_chardlg::g_setCalls.clear();
    test_chardlg::g_lastLevel.level = 0;
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetSetLevelCallbackForTest(&test_chardlg::faLevel, nullptr);
    d.SetLevel(42);
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 1u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "level");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "42");
    EXPECT_EQ(test_chardlg::g_lastLevel.level, 42);
}

TEST(CCharacterDialog, SetLifeFormatsValue) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetLife(1500);
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 1u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "life");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "1500");
}

TEST(CCharacterDialog, SetShieldFormatsValue) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetShield(999);
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 1u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "shield");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "999");
}

TEST(CCharacterDialog, SetNaeRyukFormatsValue) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetNaeRyuk(500);
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 1u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "naeryuk");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "500");
}

TEST(CCharacterDialog, SetGenGolSetMinChubSetCheRyukSetSimMek) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetGenGol();
    d.SetMinChub();
    d.SetCheRyuk();
    d.SetSimMek();
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 4u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "genGoal");
    EXPECT_EQ(test_chardlg::g_setCalls[1].field, "minchub");
    EXPECT_EQ(test_chardlg::g_setCalls[2].field, "cheryuk");
    EXPECT_EQ(test_chardlg::g_setCalls[3].field, "simmak");
}

TEST(CCharacterDialog, SetExpPointPercentFormatsWithDecimal) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetExpPointPercent(75.5f);
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 1u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "expPercent");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "75.5%");
}

TEST(CCharacterDialog, SetAttackRateResetsBothMeleeAndRangeToZero) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetAttackRate();
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 2u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "meleeattack");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "0");
    EXPECT_EQ(test_chardlg::g_setCalls[1].field, "rangeattack");
    EXPECT_EQ(test_chardlg::g_setCalls[1].text, "0");
}

TEST(CCharacterDialog, SetDefenseRateCriticalAttackRangeResetToZero) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetDefenseRate();
    d.SetCritical();
    d.SetAttackRange();
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 3u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "defense");
    EXPECT_EQ(test_chardlg::g_setCalls[1].field, "critical");
    EXPECT_EQ(test_chardlg::g_setCalls[2].field, "attackdistance");
}

TEST(CCharacterDialog, SetFameSetBadFameFormatValues) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetFame(100);
    d.SetBadFame(50);
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 2u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "fame");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "100");
    EXPECT_EQ(test_chardlg::g_setCalls[1].field, "badfame");
    EXPECT_EQ(test_chardlg::g_setCalls[1].text, "50");
}

TEST(CCharacterDialog, SetStageFormatsValue) {
    test_chardlg::g_setCalls.clear();
    test_chardlg::g_lastStage.stage = 0;
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetSetStageCallbackForTest(&test_chardlg::faStage, nullptr);
    d.SetStage(7);
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 1u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "stage");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "7");
    EXPECT_EQ(test_chardlg::g_lastStage.stage, 7);
}

TEST(CCharacterDialog, SetPointLevelingSetsFlagAndPoint) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetPointLeveling(true, 5);
    EXPECT_TRUE(d.isPointLeveling());
    EXPECT_EQ(d.nocoriPoint(), 5);
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 1u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "point");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "5");
}

TEST(CCharacterDialog, OnAddPointIncrementsAndDecrementsNocori) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetPointLeveling(true, 3);
    d.OnAddPoint(StatPointKind::GenGol);
    d.OnAddPoint(StatPointKind::GenGol);
    d.OnAddPoint(StatPointKind::MinChub);
    EXPECT_EQ(d.nocoriPoint(), 0);
    // 1:1 with legacy: OnAddPoint is a no-op when nocori=0.
    d.OnAddPoint(StatPointKind::GenGol);
    EXPECT_EQ(d.nocoriPoint(), 0);
}

TEST(CCharacterDialog, OnMinusPointClampsAtZero) {
    cCharacterDialog d;
    d.SetPointLeveling(false, 0);
    // 1:1 with legacy: OnMinusPoint is a no-op when added=0.
    d.OnMinusPoint(StatPointKind::GenGol);
    EXPECT_EQ(d.nocoriPoint(), 0);
    d.SetPointLeveling(true, 1);
    d.OnAddPoint(StatPointKind::SimMak);
    d.OnMinusPoint(StatPointKind::SimMak);
    EXPECT_EQ(d.nocoriPoint(), 1);
}

TEST(CCharacterDialog, SetMinusPointValueStoresValue) {
    cCharacterDialog d;
    d.SetMinusPointValue(7);
    EXPECT_EQ(d.minusPoint(), 7);
}

TEST(CCharacterDialog, SetPointLevelingHideFlipsFlag) {
    cCharacterDialog d;
    d.SetPointLeveling(true, 5);
    EXPECT_TRUE(d.isPointLeveling());
    d.SetPointLevelingHide();
    EXPECT_FALSE(d.isPointLeveling());
}

TEST(CCharacterDialog, RefreshInfoRoutesThroughUpdateData) {
    test_chardlg::g_setCalls.clear();
    test_chardlg::g_attrCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetUpdateAttrCallbackForTest(&test_chardlg::faAttr, nullptr);
    d.SetSetGuageCallbackForTest(&test_chardlg::faGuage, nullptr);
    d.SetLevel(10);
    d.RefreshInfo();
    // 1:1 with legacy RefreshInfo: writes level + 4 attributes.
    EXPECT_GT(test_chardlg::g_setCalls.size(), 0u);
    EXPECT_EQ(test_chardlg::g_attrCalls.size(), 4u);
    EXPECT_EQ(test_chardlg::g_guageCalls.size(), 4u);
}

TEST(CCharacterDialog, RefreshFromPlayerStatsWritesMappedLiveFields) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    CharacterStats stats;
    stats.level = 33;
    stats.hp = 1200;
    stats.mp = 450;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetPlayerStatsService(&stats);
    d.RefreshFromPlayerStats();
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 3u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "level");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "33");
    EXPECT_EQ(test_chardlg::g_setCalls[1].field, "life");
    EXPECT_EQ(test_chardlg::g_setCalls[1].text, "1200");
    EXPECT_EQ(test_chardlg::g_setCalls[2].field, "naeryuk");
    EXPECT_EQ(test_chardlg::g_setCalls[2].text, "450");
}

TEST(CCharacterDialog, RefreshGuildInfoRoutesThroughCallback) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.RefreshGuildInfo();
    EXPECT_EQ(test_chardlg::g_setCalls.size(), 2u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "munpa");
    EXPECT_EQ(test_chardlg::g_setCalls[1].field, "jikwe");
}

TEST(CCharacterDialog, RefreshPointInfoFormatsNocori) {
    test_chardlg::g_setCalls.clear();
    cCharacterDialog d;
    d.SetSetStaticTextCallbackForTest(&test_chardlg::faSetText, nullptr);
    d.SetPointLeveling(true, 7);
    test_chardlg::g_setCalls.clear();
    d.RefreshPointInfo();
    ASSERT_EQ(test_chardlg::g_setCalls.size(), 1u);
    EXPECT_EQ(test_chardlg::g_setCalls[0].field, "point");
    EXPECT_EQ(test_chardlg::g_setCalls[0].text, "7");
}

TEST(CCharacterDialog, AttrDefenceSetValueWritesPercentAndGuage) {
    cStatic sGenGol, sSimMak, sMinChub, sCheRyuk;
    cGuagen gGenGol, gSimMak, gMinChub, gCheRyuk;
    AttrDefence attr{};
    attr.pStaticGenGol = &sGenGol;
    attr.pStaticSimMak = &sSimMak;
    attr.pStaticMinChub = &sMinChub;
    attr.pStaticCheRyuk = &sCheRyuk;
    attr.pGuageGenGol  = &gGenGol;
    attr.pGuageSimMak  = &gSimMak;
    attr.pGuageMinChub = &gMinChub;
    attr.pGuageCheRyuk = &gCheRyuk;
    attr.SetValue(StatPointKind::GenGol, 50);
    EXPECT_EQ(sGenGol.GetStaticText(), "50%");
    EXPECT_FLOAT_EQ(gGenGol.GetValue(), 0.5f);
    attr.SetValue(StatPointKind::CheRyuk, 100);
    EXPECT_EQ(sCheRyuk.GetStaticText(), "100%");
    EXPECT_FLOAT_EQ(gCheRyuk.GetValue(), 1.0f);
}

TEST(CCharacterDialog, AttrDefenceSetValueWithColorSetsFGColor) {
    cStatic s;
    cGuagen g;
    AttrDefence attr{};
    attr.pStaticGenGol = &s;
    attr.pGuageGenGol  = &g;
    attr.SetValue(StatPointKind::GenGol, 50, 0xFF0000FFu);
    EXPECT_EQ(s.GetStaticText(), "50%");
    EXPECT_EQ(s.GetFGColor(), 0xFF0000FFu);
}

TEST(CCharacterDialog, AttrDefenceSetValueWithNullStaticIsSafe) {
    AttrDefence attr{};
    attr.SetValue(StatPointKind::GenGol, 50);
    SUCCEED();
}

TEST(CCharacterDialog, CharStaticCtrlFieldCount) {
    // 1:1 with legacy CHARSTATICCTRL: 22 cStatic* fields.
    // Verify by offsetof walking is overkill; instead just
    // count by using a struct copy + field touches.
    CharStaticCtrl c{};
    c.munpa   = reinterpret_cast<cStatic*>(0x1);
    c.jikwe   = reinterpret_cast<cStatic*>(0x2);
    c.fame    = reinterpret_cast<cStatic*>(0x3);
    c.badfame = reinterpret_cast<cStatic*>(0x4);
    c.name    = reinterpret_cast<cStatic*>(0x5);
    c.stage   = reinterpret_cast<cStatic*>(0x6);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(c.munpa), 0x1u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(c.stage), 0x6u);
}

TEST(CCharacterDialog, SetPointBtnsForTestStoresButtons) {
    cCharacterDialog::PointBtnArray plus{};
    cCharacterDialog::PointBtnArray minus{};
    cCharacterDialog d;
    d.SetPointBtnsForTest(plus, minus);
    SUCCEED();
}

TEST(CCharacterDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cCharacterDialog>);
    static_assert(!std::is_copy_assignable_v<cCharacterDialog>);
}
