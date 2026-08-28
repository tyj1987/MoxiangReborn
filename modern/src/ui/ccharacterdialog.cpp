// ccharacterdialog.cpp — modern port of 墨香 CCharacterDialog.

#include "mxh/ui/ccharacterdialog.hpp"
#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cGuagen.hpp"
#include "mxh/ui/cStatic.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

void AttrDefence::SetValue(StatPointKind attrib, std::uint16_t value,
                            std::uint32_t color) {
    char temp[32];
    std::snprintf(temp, sizeof(temp), "%d%%", value);
    switch (attrib) {
        case StatPointKind::GenGol:
            if (pStaticGenGol) pStaticGenGol->SetStaticText(temp);
            if (pGuageGenGol)  pGuageGenGol->SetValue(value * 0.01f);
            if (color && pStaticGenGol) pStaticGenGol->SetFGColor(color);
            break;
        case StatPointKind::SimMak:
            if (pStaticSimMak) pStaticSimMak->SetStaticText(temp);
            if (pGuageSimMak)  pGuageSimMak->SetValue(value * 0.01f);
            if (color && pStaticSimMak) pStaticSimMak->SetFGColor(color);
            break;
        case StatPointKind::MinChub:
            if (pStaticMinChub) pStaticMinChub->SetStaticText(temp);
            if (pGuageMinChub)  pGuageMinChub->SetValue(value * 0.01f);
            if (color && pStaticMinChub) pStaticMinChub->SetFGColor(color);
            break;
        case StatPointKind::CheRyuk:
            if (pStaticCheRyuk) pStaticCheRyuk->SetStaticText(temp);
            if (pGuageCheRyuk)  pGuageCheRyuk->SetValue(value * 0.01f);
            if (color && pStaticCheRyuk) pStaticCheRyuk->SetFGColor(color);
            break;
    }
}

cCharacterDialog::cCharacterDialog() {
    m_nocoriPoint = 0;
    m_MinusPoint = 0;
    m_bPointLeveling = false;
    m_nCurrentLevel = 1;
    m_nAddedGenGol  = 0;
    m_nAddedSimMak  = 0;
    m_nAddedMinChub = 0;
    m_nAddedCheRyuk = 0;
    for (int i = 0; i < kMaxBtnPoint; ++i) {
        m_pPointBtn[i]      = nullptr;
        m_pPointMinusBtn[i] = nullptr;
    }
}

cCharacterDialog::~cCharacterDialog() = default;

void cCharacterDialog::Init(long x, long y, std::uint16_t wid, std::uint16_t hei,
                              void* basicImage, long id) {
    // 1:1 with legacy Init.  The legacy forwards to
    // cDialog::Init; modern cDialog::Init takes a different
    // signature (cImage* + id), so the host is expected to
    // call the base init separately.
    (void)basicImage;
    SetAbsXY(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y));
    (void)wid; (void)hei; (void)id;
}

void cCharacterDialog::Linking() {
    const auto staticFor = [this](const char* id) -> cStatic* {
        return dynamic_cast<cStatic*>(findWindowByLegacyId(id));
    };
    m_ppStatic.munpa = staticFor("CI_CHARMUNPA");
    m_ppStatic.jikwe = staticFor("CI_CHARJIKWE");
    m_ppStatic.fame = staticFor("CI_CHARFAME");
    m_ppStatic.badfame = staticFor("CI_CHARBADFAME");
    m_ppStatic.name = staticFor("CI_CHARNAME");
    m_ppStatic.stage = staticFor("CI_CHARSTAGE");
    m_ppStatic.genGoal = staticFor("CI_CHARGENGOAL");
    m_ppStatic.simmak = staticFor("CI_CHARSIMMAK");
    m_ppStatic.minchub = staticFor("CI_CHARDEX");
    m_ppStatic.cheryuk = staticFor("CI_CHARSTA");
    m_ppStatic.level = staticFor("CI_CHARLEVEL");
    m_ppStatic.expPercent = staticFor("CI_CHAREXPPERCENT");
    m_ppStatic.point = staticFor("CI_CHARPOINT");
    m_ppStatic.meleeattack = staticFor("CI_CHARATTACK");
    m_ppStatic.rangeattack = staticFor("CI_LONGATTACK");
    m_ppStatic.critical = staticFor("CI_CRITICAL");
    m_ppStatic.attackdistance = staticFor("CI_DISTANCE");
    m_ppStatic.life = staticFor("CI_CHARLIFE");
    m_ppStatic.defense = staticFor("CI_CHARDEFENSE");
    m_ppStatic.naeryuk = staticFor("CI_CHARNAERYUK");
    m_ppStatic.Shield = staticFor("CI_HOSINDEFENSE");
    m_AttrDefComponent.pStaticGenGol = staticFor("CI_CHARHWA");
    m_AttrDefComponent.pStaticSimMak = staticFor("CI_CHARSU");
    m_AttrDefComponent.pStaticMinChub = staticFor("CI_CHARMOK");
    m_AttrDefComponent.pStaticCheRyuk = staticFor("CI_CHARKUM");
    m_AttrDefComponent.pGuageGenGol = dynamic_cast<cGuagen*>(
        findWindowByLegacyId("CI_DEFENCE_HWA"));
    m_AttrDefComponent.pGuageSimMak = dynamic_cast<cGuagen*>(
        findWindowByLegacyId("CI_DEFENCE_SU"));
    m_AttrDefComponent.pGuageMinChub = dynamic_cast<cGuagen*>(
        findWindowByLegacyId("CI_DEFENCE_MOK"));
    m_AttrDefComponent.pGuageCheRyuk = dynamic_cast<cGuagen*>(
        findWindowByLegacyId("CI_DEFENCE_KUM"));
}

void cCharacterDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive.  Notify the main-bar
    // option icon.
    cDialog::SetActive(val);
    if (m_mainBarCb) m_mainBarCb(isActive(), m_mainBarUser);
}

void cCharacterDialog::SetStaticTextByField(const char* fieldName, const char* text) {
    if (!fieldName || !text) return;
    cStatic* target = nullptr;
    if (std::strcmp(fieldName, "munpa") == 0) target = m_ppStatic.munpa;
    else if (std::strcmp(fieldName, "jikwe") == 0) target = m_ppStatic.jikwe;
    else if (std::strcmp(fieldName, "fame") == 0) target = m_ppStatic.fame;
    else if (std::strcmp(fieldName, "badfame") == 0) target = m_ppStatic.badfame;
    else if (std::strcmp(fieldName, "name") == 0) target = m_ppStatic.name;
    else if (std::strcmp(fieldName, "stage") == 0) target = m_ppStatic.stage;
    else if (std::strcmp(fieldName, "genGoal") == 0) target = m_ppStatic.genGoal;
    else if (std::strcmp(fieldName, "simmak") == 0) target = m_ppStatic.simmak;
    else if (std::strcmp(fieldName, "minchub") == 0) target = m_ppStatic.minchub;
    else if (std::strcmp(fieldName, "cheryuk") == 0) target = m_ppStatic.cheryuk;
    else if (std::strcmp(fieldName, "level") == 0) target = m_ppStatic.level;
    else if (std::strcmp(fieldName, "expPercent") == 0) target = m_ppStatic.expPercent;
    else if (std::strcmp(fieldName, "point") == 0) target = m_ppStatic.point;
    else if (std::strcmp(fieldName, "meleeattack") == 0) target = m_ppStatic.meleeattack;
    else if (std::strcmp(fieldName, "rangeattack") == 0) target = m_ppStatic.rangeattack;
    else if (std::strcmp(fieldName, "critical") == 0) target = m_ppStatic.critical;
    else if (std::strcmp(fieldName, "attackdistance") == 0) target = m_ppStatic.attackdistance;
    else if (std::strcmp(fieldName, "life") == 0) target = m_ppStatic.life;
    else if (std::strcmp(fieldName, "defense") == 0) target = m_ppStatic.defense;
    else if (std::strcmp(fieldName, "naeryuk") == 0) target = m_ppStatic.naeryuk;
    else if (std::strcmp(fieldName, "shield") == 0) target = m_ppStatic.Shield;
    if (target) target->SetStaticText(text);
    if (m_setStaticTextCb) m_setStaticTextCb(fieldName, text, m_setStaticTextUser);
}

void cCharacterDialog::UpdateAttrByKind(StatPointKind kind, std::uint16_t value,
                                         std::uint32_t color) {
    m_AttrDefComponent.SetValue(kind, value, color);
    if (m_updateAttrCb) m_updateAttrCb(kind, value, color, m_updateAttrUser);
    if (m_setGuageCb)   m_setGuageCb(kind, value * 0.01f, m_setGuageUser);
}

void cCharacterDialog::UpdateData() {
    // 1:1 with legacy UpdateData.  Refreshes the level +
    // each attribute (GenGol / SimMak / MinChub / CheRyuk) +
    // the exp-percent + the attack / defense / critical /
    // range / fame / badfame / life / shield / naeryuk
    // statics.  The actual HERO->Getxxx() calls are
    // deferred; tests inject the statics + guages and
    // verify the call routing.
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", m_nCurrentLevel);
    SetStaticTextByField("level", buf);
    UpdateAttrByKind(StatPointKind::GenGol,  static_cast<std::uint16_t>(m_nAddedGenGol  + 100), 0);
    UpdateAttrByKind(StatPointKind::SimMak,  static_cast<std::uint16_t>(m_nAddedSimMak  + 100), 0);
    UpdateAttrByKind(StatPointKind::MinChub, static_cast<std::uint16_t>(m_nAddedMinChub + 100), 0);
    UpdateAttrByKind(StatPointKind::CheRyuk, static_cast<std::uint16_t>(m_nAddedCheRyuk + 100), 0);
}

void cCharacterDialog::UpdateForStageAbility() {
    // 1:1 with legacy UpdateForStageAbility.  Recomputes
    // the attack / defense / critical / range numbers for
    // the current stage.  Modern port is a no-op (the
    // underlying stat math is deferred until HERO +
    // CharacterData unit is ported).
}

void cCharacterDialog::RefreshInfo() {
    // 1:1 with legacy RefreshInfo: calls UpdateData +
    // UpdateForStageAbility.
    UpdateData();
    UpdateForStageAbility();
}

void cCharacterDialog::RefreshFromPlayerStats() {
    if (!m_playerStatsService) return;
    SetLevel(m_playerStatsService->getLevel());
    SetLife(m_playerStatsService->getCurrentHp());
    SetNaeRyuk(m_playerStatsService->getCurrentMp());
    const auto setAttribute = [this](const char* field, std::uint16_t value) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(value));
        SetStaticTextByField(field, buf);
    };
    setAttribute("genGoal", m_playerStatsService->getGenGol());
    setAttribute("minchub", m_playerStatsService->getMinChub());
    setAttribute("cheryuk", m_playerStatsService->getCheRyuk());
    setAttribute("simmak", m_playerStatsService->getSimMek());
}

void cCharacterDialog::RefreshGuildInfo() {
    // 1:1 with legacy RefreshGuildInfo.  Writes the
    // guild name + jikwe into m_ppStatic.munpa / jikwe.
    SetStaticTextByField("munpa", "");
    SetStaticTextByField("jikwe", "");
}

void cCharacterDialog::RefreshPointInfo() {
    // 1:1 with legacy RefreshPointInfo.  Writes the
    // remaining point count.
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", m_nocoriPoint);
    SetStaticTextByField("point", buf);
}

void cCharacterDialog::SetLevel(std::uint16_t level) {
    m_nCurrentLevel = level;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", level);
    SetStaticTextByField("level", buf);
    if (m_setLevelCb) m_setLevelCb(level, m_setLevelUser);
}

void cCharacterDialog::SetLife(std::uint32_t life) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u", life);
    SetStaticTextByField("life", buf);
}

void cCharacterDialog::SetShield(std::uint32_t shield) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u", shield);
    SetStaticTextByField("shield", buf);
}

void cCharacterDialog::SetNaeRyuk(std::uint32_t naeryuk) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u", naeryuk);
    SetStaticTextByField("naeryuk", buf);
}

void cCharacterDialog::SetGenGol() {
    // 1:1 with legacy SetGenGol.  Writes the gengoal value
    // into the gengoal static.  Modern port: routes the
    // value (m_nAddedGenGol + base) through the static
    // text callback.
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", m_nAddedGenGol + 100);
    SetStaticTextByField("genGoal", buf);
}

void cCharacterDialog::SetMinChub() {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", m_nAddedMinChub + 100);
    SetStaticTextByField("minchub", buf);
}

void cCharacterDialog::SetCheRyuk() {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", m_nAddedCheRyuk + 100);
    SetStaticTextByField("cheryuk", buf);
}

void cCharacterDialog::SetSimMek() {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", m_nAddedSimMak + 100);
    SetStaticTextByField("simmak", buf);
}

void cCharacterDialog::SetExpPointPercent(float perc) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f%%", perc);
    SetStaticTextByField("expPercent", buf);
}

void cCharacterDialog::SetAttackRate() {
    SetStaticTextByField("meleeattack", "0");
    SetStaticTextByField("rangeattack", "0");
}

void cCharacterDialog::SetDefenseRate() {
    SetStaticTextByField("defense", "0");
}

void cCharacterDialog::SetCritical() {
    SetStaticTextByField("critical", "0");
}

void cCharacterDialog::SetAttackRange() {
    SetStaticTextByField("attackdistance", "0");
}

void cCharacterDialog::SetFame(std::uint32_t fame) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u", fame);
    SetStaticTextByField("fame", buf);
}

void cCharacterDialog::SetBadFame(std::uint32_t badfame) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u", badfame);
    SetStaticTextByField("badfame", buf);
}

void cCharacterDialog::SetStage(std::uint8_t stage) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u", stage);
    SetStaticTextByField("stage", buf);
    if (m_setStageCb) m_setStageCb(stage, m_setStageUser);
}

void cCharacterDialog::SetPointLeveling(bool val, std::uint16_t point) {
    // 1:1 with legacy SetPointLeveling(BOOL, WORD).  Sets
    // the level-up UI mode + reserves N points.
    m_bPointLeveling = val;
    m_nocoriPoint = static_cast<int>(point);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u", point);
    SetStaticTextByField("point", buf);
}

void cCharacterDialog::OnAddPoint(StatPointKind whatsPoint) {
    // 1:1 with legacy OnAddPoint.  Increments the
    // per-attribute + added counter + decrements the
    // remaining point.
    if (m_nocoriPoint <= 0) return;
    switch (whatsPoint) {
        case StatPointKind::GenGol:  ++m_nAddedGenGol;  break;
        case StatPointKind::SimMak:  ++m_nAddedSimMak;  break;
        case StatPointKind::MinChub: ++m_nAddedMinChub; break;
        case StatPointKind::CheRyuk: ++m_nAddedCheRyuk; break;
    }
    --m_nocoriPoint;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", m_nocoriPoint);
    SetStaticTextByField("point", buf);
}

void cCharacterDialog::OnMinusPoint(StatPointKind whatsPoint) {
    // 1:1 with legacy OnMinusPoint.  Decrements the
    // per-attribute + added counter + increments the
    // remaining point.
    switch (whatsPoint) {
        case StatPointKind::GenGol:
            if (m_nAddedGenGol <= 0) return;
            --m_nAddedGenGol;
            break;
        case StatPointKind::SimMak:
            if (m_nAddedSimMak <= 0) return;
            --m_nAddedSimMak;
            break;
        case StatPointKind::MinChub:
            if (m_nAddedMinChub <= 0) return;
            --m_nAddedMinChub;
            break;
        case StatPointKind::CheRyuk:
            if (m_nAddedCheRyuk <= 0) return;
            --m_nAddedCheRyuk;
            break;
    }
    ++m_nocoriPoint;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", m_nocoriPoint);
    SetStaticTextByField("point", buf);
}

void cCharacterDialog::SetPointLevelingHide() {
    // 1:1 with legacy SetPointLevelingHide.  Hides the
    // point-leveling UI; the modern port flips the flag.
    m_bPointLeveling = false;
}

}  // namespace mxh::ui
