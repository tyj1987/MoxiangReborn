#include "mxh/ui/monsterguagedlg.hpp"

#include "mxh/ui/cStatic.hpp"
#include "mxh/ui/cobjectguagen.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

using mxh::ui::cDialog;
using mxh::ui::cMonsterGuageDlg;
using mxh::ui::cObjectGuagen;
using mxh::ui::cStatic;
using mxh::ui::MonsterGaugeInfo;
using mxh::ui::MonsterGuageMode;

namespace {

struct Controls {
    cStatic name;
    cStatic lifeText;
    cObjectGuagen life;
    cStatic shieldText;
    cObjectGuagen shield;
    cStatic guildName;
    cStatic guildUnion;
    cStatic npcName;
    cStatic lifeBase;

    Controls() {
        name.Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kNameId);
        lifeText.Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kLifeTextId);
        life.Init(0, 0, 100, 10, nullptr, cMonsterGuageDlg::kLifeGaugeId);
        shieldText.Init(0, 0, 100, 20, nullptr, 0);
        shield.Init(0, 0, 100, 10, nullptr, cMonsterGuageDlg::kShieldGaugeId);
        guildName.Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kGuildNameId);
        guildUnion.Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kGuildUnionNameId);
        npcName.Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kNpcNameId);
        lifeBase.Init(0, 0, 100, 10, nullptr, cMonsterGuageDlg::kLifeBaseId);
    }

    void Attach(cMonsterGuageDlg& dialog) {
        dialog.SetControlsForTest(&name, &lifeText, &life, &shieldText,
                                  &shield, &guildName, &guildUnion,
                                  &npcName, &lifeBase);
    }

    void SetAllActive(bool active) {
        name.SetActive(active);
        lifeText.SetActive(active);
        life.SetActive(active);
        shieldText.SetActive(active);
        shield.SetActive(active);
        guildName.SetActive(active);
        guildUnion.SetActive(active);
        npcName.SetActive(active);
        lifeBase.SetActive(active);
    }
};

void ExpectActive(const Controls& controls, bool active) {
    EXPECT_EQ(controls.life.isActive(), active);
    EXPECT_EQ(controls.shield.isActive(), active);
    EXPECT_EQ(controls.lifeText.isActive(), active);
    EXPECT_EQ(controls.lifeBase.isActive(), active);
}

struct ChatCapture {
    int calls = 0;
    int lastId = 0;
};

const char* ChatFormat(int id, void* user) {
    auto* capture = static_cast<ChatCapture*>(user);
    ++capture->calls;
    capture->lastId = id;
    return id == cMonsterGuageDlg::kGuildNameChatMsg
        ? "Guild:%s"
        : "Union:%s";
}

}  // namespace

TEST(MonsterGuageDlgTest, InheritsFromDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<cDialog, cMonsterGuageDlg>);
    static_assert(!std::is_copy_constructible_v<cMonsterGuageDlg>);
    static_assert(!std::is_copy_assignable_v<cMonsterGuageDlg>);
    SUCCEED();
}

TEST(MonsterGuageDlgTest, ConstantsMatchLegacyModesAndIds) {
    EXPECT_EQ(static_cast<int>(MonsterGuageMode::Monster), 0);
    EXPECT_EQ(static_cast<int>(MonsterGuageMode::Character), 1);
    EXPECT_EQ(static_cast<int>(MonsterGuageMode::Npc), 2);
    EXPECT_EQ(static_cast<int>(MonsterGuageMode::Pet), 3);
    EXPECT_EQ(static_cast<int>(MonsterGuageMode::Max), 4);
    EXPECT_EQ(cMonsterGuageDlg::kDialogId, 1047);
    EXPECT_EQ(cMonsterGuageDlg::kNameId, 1048);
    EXPECT_EQ(cMonsterGuageDlg::kLifeTextId, 1049);
    EXPECT_EQ(cMonsterGuageDlg::kLifeGaugeId, 1050);
    EXPECT_EQ(cMonsterGuageDlg::kShieldGaugeId, 1051);
    EXPECT_EQ(cMonsterGuageDlg::kGuildNameId, 1052);
    EXPECT_EQ(cMonsterGuageDlg::kGuildUnionNameId, 1053);
    EXPECT_EQ(cMonsterGuageDlg::kNpcNameId, 1057);
    EXPECT_EQ(cMonsterGuageDlg::kLifeBaseId, 1058);
    EXPECT_EQ(cMonsterGuageDlg::kMonsterObjectType, 32u);
}

TEST(MonsterGuageDlgTest, ConstructorInitializesLegacyState) {
    cMonsterGuageDlg dialog;
    EXPECT_EQ(dialog.currentMode(), -1);
    EXPECT_EQ(dialog.GetObjectType(), cMonsterGuageDlg::kMonsterObjectType);
    EXPECT_EQ(dialog.GetCurMonster(), nullptr);
    EXPECT_EQ(dialog.nameControl(), nullptr);
    EXPECT_EQ(dialog.lifeGauge(), nullptr);
    EXPECT_EQ(dialog.modeControlCount(0), 0u);
}

TEST(MonsterGuageDlgTest, LinkingFindsExactControlsAndBuildsModeLists) {
    cMonsterGuageDlg dialog;
    dialog.Init(0, 0, 300, 200, nullptr, cMonsterGuageDlg::kDialogId);

    auto name = std::make_unique<cStatic>();
    name->Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kNameId);
    auto* nameRaw = name.get();
    dialog.Add(std::move(name));

    auto lifeText = std::make_unique<cStatic>();
    lifeText->Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kLifeTextId);
    auto* lifeTextRaw = lifeText.get();
    dialog.Add(std::move(lifeText));

    auto life = std::make_unique<cObjectGuagen>();
    life->Init(0, 0, 100, 10, nullptr, cMonsterGuageDlg::kLifeGaugeId);
    auto* lifeRaw = life.get();
    dialog.Add(std::move(life));

    auto shield = std::make_unique<cObjectGuagen>();
    shield->Init(0, 0, 100, 10, nullptr, cMonsterGuageDlg::kShieldGaugeId);
    auto* shieldRaw = shield.get();
    dialog.Add(std::move(shield));

    auto guild = std::make_unique<cStatic>();
    guild->Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kGuildNameId);
    auto* guildRaw = guild.get();
    dialog.Add(std::move(guild));

    auto unionName = std::make_unique<cStatic>();
    unionName->Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kGuildUnionNameId);
    auto* unionRaw = unionName.get();
    dialog.Add(std::move(unionName));

    auto npc = std::make_unique<cStatic>();
    npc->Init(0, 0, 100, 20, nullptr, cMonsterGuageDlg::kNpcNameId);
    auto* npcRaw = npc.get();
    dialog.Add(std::move(npc));

    auto base = std::make_unique<cStatic>();
    base->Init(0, 0, 100, 10, nullptr, cMonsterGuageDlg::kLifeBaseId);
    auto* baseRaw = base.get();
    dialog.Add(std::move(base));

    dialog.Linking();
    EXPECT_EQ(dialog.nameControl(), nameRaw);
    EXPECT_EQ(dialog.lifeTextControl(), lifeTextRaw);
    EXPECT_EQ(dialog.lifeGauge(), lifeRaw);
    EXPECT_EQ(dialog.shieldGauge(), shieldRaw);
    EXPECT_EQ(dialog.guildNameControl(), guildRaw);
    EXPECT_EQ(dialog.guildUnionNameControl(), unionRaw);
    EXPECT_EQ(dialog.npcNameControl(), npcRaw);
    EXPECT_EQ(dialog.modeControlCount(0), 4u);
    EXPECT_EQ(dialog.modeControlCount(1), 2u);
    EXPECT_EQ(dialog.modeControlCount(2), 1u);
    EXPECT_EQ(dialog.modeControlCount(3), 0u);
    EXPECT_EQ(dialog.componentAt(7), baseRaw);
}

TEST(MonsterGuageDlgTest, SetMonsterNameStoresNameClearsTargetAndGauges) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.life.SetValue(0.8f, 0);
    controls.shield.SetValue(0.5f, 0);
    dialog.SetCurrentMonsterHandle(reinterpret_cast<void*>(1));

    dialog.SetMonsterName("Slime");
    EXPECT_EQ(controls.name.GetStaticText(), "Slime");
    EXPECT_EQ(dialog.GetCurMonster(), nullptr);
    EXPECT_FLOAT_EQ(controls.life.GetValue(), 0.0f);
    EXPECT_FLOAT_EQ(controls.shield.GetValue(), 0.0f);
}

TEST(MonsterGuageDlgTest, SetNpcNameAndNameColorForwardToStatics) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetNpcName("Merchant");
    dialog.SetMonsterNameColor(0xFF123456u);
    EXPECT_EQ(controls.npcName.GetStaticText(), "Merchant");
    EXPECT_EQ(controls.name.GetFGColor(), 0xFF123456u);
}

TEST(MonsterGuageDlgTest, LifeClampsMaximumAndUsesImmediateGaugeForTypeZero) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetMonsterLife(150, 100, 0);
    EXPECT_FLOAT_EQ(controls.life.GetValue(), 1.0f);
    EXPECT_EQ(controls.life.GetEffectTime(), 0u);
    dialog.SetMonsterLife(50, 100, 0);
    EXPECT_FLOAT_EQ(controls.life.GetValue(), 0.5f);
    EXPECT_EQ(controls.life.GetEffectTime(), 0u);
}

TEST(MonsterGuageDlgTest, LifeTypeOneUsesLegacyIntegerEffectFormula) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetMonsterLife(50, 100, 1);
    EXPECT_FLOAT_EQ(controls.life.GetValue(), 0.5f);
    EXPECT_EQ(controls.life.GetEffectTime(), 750u);
}

TEST(MonsterGuageDlgTest, ZeroMaximumIsPromotedToOne) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetMonsterLife(0, 0, 0);
    EXPECT_FLOAT_EQ(controls.life.GetValue(), 0.0f);
    EXPECT_EQ(controls.life.GetEffectTime(), 0u);
}

TEST(MonsterGuageDlgTest, ShieldClampsAndUsesSameFormula) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetMonsterShield(80, 40, 1);
    EXPECT_FLOAT_EQ(controls.shield.GetValue(), 1.0f);
    EXPECT_EQ(controls.shield.GetEffectTime(), 1480u);
}

TEST(MonsterGuageDlgTest, CheatTextIsOptInAndUsesClampedValues) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetMonsterLife(80, 100, 0);
    EXPECT_EQ(controls.lifeText.GetStaticText(), "");
    dialog.SetCheatEnabled(true);
    dialog.SetMonsterLife(120, 100, 0);
    EXPECT_EQ(controls.lifeText.GetStaticText(), "100 / 100");
}

TEST(MonsterGuageDlgTest, StructInfoOverloadsForwardLifeAndShield) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    MonsterGaugeInfo info{25, 100, 15, 30};

    dialog.SetMonsterLife(info, 0);
    dialog.SetMonsterShield(info, 0);
    EXPECT_FLOAT_EQ(controls.life.GetValue(), 0.25f);
    EXPECT_FLOAT_EQ(controls.shield.GetValue(), 0.5f);
}

TEST(MonsterGuageDlgTest, GuildAndUnionUseFallbackRawNames) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetGuildUnionName("GuildA", "UnionA");
    EXPECT_EQ(controls.guildName.GetStaticText(), "GuildA");
    EXPECT_EQ(controls.guildUnion.GetStaticText(), "UnionA");
    dialog.SetGuildUnionName("", "");
    EXPECT_EQ(controls.guildName.GetStaticText(), "");
    EXPECT_EQ(controls.guildUnion.GetStaticText(), "");
}

TEST(MonsterGuageDlgTest, GuildAndUnionUseLocalizedChatFormats) {
    cMonsterGuageDlg dialog;
    Controls controls;
    ChatCapture capture;
    controls.Attach(dialog);
    dialog.SetChatMsgCallback(&ChatFormat, &capture);

    dialog.SetGuildUnionName("GuildA", "UnionA");
    EXPECT_EQ(controls.guildName.GetStaticText(), "Guild:GuildA");
    EXPECT_EQ(controls.guildUnion.GetStaticText(), "Union:UnionA");
    EXPECT_EQ(capture.calls, 2);
    EXPECT_EQ(capture.lastId, cMonsterGuageDlg::kGuildUnionNameChatMsg);
}

TEST(MonsterGuageDlgTest, ShowModesDisablePreviousAndEnableCurrent) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.SetAllActive(false);

    dialog.ShowMonsterGuageMode(0);
    EXPECT_EQ(dialog.currentMode(), 0);
    ExpectActive(controls, true);
    EXPECT_FALSE(controls.guildName.isActive());

    dialog.ShowMonsterGuageMode(1);
    EXPECT_EQ(dialog.currentMode(), 1);
    ExpectActive(controls, false);
    EXPECT_TRUE(controls.guildName.isActive());
    EXPECT_TRUE(controls.guildUnion.isActive());
}

TEST(MonsterGuageDlgTest, PetModeDisablesPreviousWithoutActivatingPetControls) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.SetAllActive(false);

    dialog.ShowMonsterGuageMode(0);
    dialog.ShowMonsterGuageMode(3);
    EXPECT_EQ(dialog.currentMode(), 3);
    ExpectActive(controls, false);
    EXPECT_FALSE(controls.npcName.isActive());
}

TEST(MonsterGuageDlgTest, SameModeAndInvalidModeAreNoOps) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.SetAllActive(false);

    dialog.ShowMonsterGuageMode(0);
    controls.life.SetActive(false);
    dialog.ShowMonsterGuageMode(0);
    EXPECT_FALSE(controls.life.isActive());
    dialog.ShowMonsterGuageMode(-1);
    dialog.ShowMonsterGuageMode(4);
    EXPECT_EQ(dialog.currentMode(), 0);
}

TEST(MonsterGuageDlgTest, CharacterRenderAdjustsGuildNameRelativeY) {
    cMonsterGuageDlg dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.ShowMonsterGuageMode(1);

    controls.guildUnion.SetStaticText("UnionA");
    dialog.Render();
    EXPECT_EQ(controls.guildName.relY(), 18);

    controls.guildUnion.SetStaticText("");
    dialog.Render();
    EXPECT_EQ(controls.guildName.relY(), 26);
}

TEST(MonsterGuageDlgTest, ObjectTypeAndHandleAreSettable) {
    cMonsterGuageDlg dialog;
    EXPECT_EQ(dialog.GetObjectType(), cMonsterGuageDlg::kMonsterObjectType);
    dialog.SetObjectType(99);
    dialog.SetCurrentMonsterHandle(reinterpret_cast<void*>(7));
    EXPECT_EQ(dialog.GetObjectType(), 99u);
    EXPECT_EQ(dialog.GetCurMonster(), reinterpret_cast<void*>(7));
}

TEST(MonsterGuageDlgTest, MissingControlsAreGuarded) {
    cMonsterGuageDlg dialog;
    dialog.Linking();
    dialog.SetMonsterName("x");
    dialog.SetNpcName("x");
    dialog.SetMonsterLife(1, 1);
    dialog.SetMonsterShield(1, 1);
    dialog.SetGuildUnionName("g", "u");
    dialog.SetMonsterNameColor(0xFFFFFFFFu);
    dialog.ShowMonsterGuageMode(0);
    dialog.ShowMonsterGuageMode(1);
    dialog.Render();
    SUCCEED();
}
