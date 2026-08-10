// ccharacterdialog.hpp — modern port of 墨香 CCharacterDialog (character stat panel).
//
// 1:1 port of legacy `CCharacterDialog` from
//   `墨香【源码】\[Client]MH\CharacterDialog.h` (no .cpp in legacy).
//
// The character dialog shows the player's stat panel:
// guild, name, stage, melee/range attack, defense, life,
// shield, naeryuk, plus per-attribute (GenGol/SimMak/MinChub/
// CheRyuk) values.  It also has a "point leveling" mode that
// lets the player invest stat points by clicking +/- buttons.
//
// The modern port keeps the 1:1 surface:
//   * CHARSTATICCTRL struct (24 cStatic* fields)
//   * ATTRDEFENCE struct (attribute value + guage)
//   * 6 point btn + 6 point minus btn
//   * SetLevel/SetLife/SetShield/SetNaeRyuk setters
//   * SetGenGol/SetMinChub/SetCheRyuk/SetSimMek stat setters
//   * SetExpPointPercent
//   * SetAttackRate/SetDefenseRate
//   * SetCritical/SetAttackRange
//   * SetFame/SetBadFame
//   * SetPointLeveling + OnAddPoint + OnMinusPoint
//   * SetStage
//   * RefreshInfo / RefreshGuildInfo / RefreshPointInfo
//
// Per-attribute writes are routed through callbacks (the
// modern cStatic / cGuagen have a minimal API; the host
// injects the binding).

#pragma once

#include "mxh/ui/cDialog.hpp"
#include "mxh/services/IPlayerStatsService.hpp"
#include "mxh/ui/cstatic.hpp"
#include "mxh/ui/cwindow.hpp"

#include <array>
#include <cstdint>

namespace mxh::ui {

class cButton;
class cGuagen;

// 1:1 with legacy MAX_BTN_POINT = 6 (genGoal/simMak/minChub/
// cheRyuk + 2 reserved).
inline constexpr std::int32_t kMaxBtnPoint = 6;

// 1:1 with legacy stat-point kinds used by OnAddPoint /
// OnMinusPoint (legacy "whatsPoint" BYTE).  The legacy uses
// a small enum; the modern port keeps the same values so
// callers can pass the legacy literal.
enum class StatPointKind : std::uint8_t {
    GenGol  = 0,
    SimMak  = 1,
    MinChub = 2,
    CheRyuk = 3,
};

// 1:1 with legacy CHARSTATICCTRL (legacy CharacterDialog.h).
// The struct preserves the legacy cStatic* fields (24
// distinct cStatic controls).  Field order matches the
// legacy header.
struct CharStaticCtrl {
    cStatic* munpa         = nullptr;
    cStatic* jikwe         = nullptr;
    cStatic* fame          = nullptr;
    cStatic* badfame       = nullptr;
    cStatic* name          = nullptr;
    cStatic* stage         = nullptr;
    cStatic* genGoal       = nullptr;
    cStatic* simmak        = nullptr;
    cStatic* minchub       = nullptr;
    cStatic* cheryuk       = nullptr;
    cStatic* level         = nullptr;
    cStatic* expPercent    = nullptr;
    cStatic* point         = nullptr;
    cStatic* meleeattack   = nullptr;
    cStatic* rangeattack   = nullptr;
    cStatic* defense       = nullptr;
    cStatic* life          = nullptr;
    cStatic* Shield        = nullptr;
    cStatic* naeryuk       = nullptr;
    cStatic* spname        = nullptr;
    cStatic* critical      = nullptr;
    cStatic* attackdistance = nullptr;
};

// 1:1 with legacy ATTRDEFENCE.  Each attribute has a
// cStatic* text view + cGuagen* bar.  SetValue(attrib, value,
// color) writes the value as a percent + updates the guage.
struct AttrDefence {
    // 4 attributes: GenGol/SimMak/MinChub/CheRyuk.  The
    // legacy uses ATTRIBUTE_VAL<> (templated array); the
    // modern port uses 4 explicit cStatic* / cGuagen* pairs.
    cStatic* pStaticGenGol   = nullptr;
    cStatic* pStaticSimMak   = nullptr;
    cStatic* pStaticMinChub  = nullptr;
    cStatic* pStaticCheRyuk  = nullptr;
    cGuagen*  pGuageGenGol   = nullptr;
    cGuagen*  pGuageSimMak   = nullptr;
    cGuagen*  pGuageMinChub  = nullptr;
    cGuagen*  pGuageCheRyuk  = nullptr;

    void SetValue(StatPointKind attrib, std::uint16_t value,
                  std::uint32_t color = 0);
};

class cCharacterDialog : public cDialog {
public:
    cCharacterDialog();
    ~cCharacterDialog() override;

    cCharacterDialog(const cCharacterDialog&) = delete;
    cCharacterDialog& operator=(const cCharacterDialog&) = delete;

    // 1:1 with legacy Init.  Stores the dialog dimensions.
    void Init(long x, long y, std::uint16_t wid, std::uint16_t hei,
              void* basicImage, long id = 0);

    // 1:1 with legacy Linking.  Wires the 24 cStatic + 6
    // cButton + 4 cGuagen children.
    void Linking();

    // 1:1 with legacy SetActive(BOOL).
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy UpdateData / UpdateForStageAbility /
    // RefreshInfo / RefreshGuildInfo / RefreshPointInfo.
    void UpdateData();
    void UpdateForStageAbility();
    void RefreshInfo();
    // Refresh only fields with an unambiguous IPlayerStatsService mapping:
    // level, current HP (life), and current MP (naeryuk).
    void RefreshFromPlayerStats();
    void SetPlayerStatsService(const mxh::services::IPlayerStatsService* service) noexcept {
        m_playerStatsService = service;
    }
    void RefreshGuildInfo();
    void RefreshPointInfo();

    // 1:1 with legacy stat setters.  Each setter routes the
    // value through a "static set text" callback.
    void SetLevel(std::uint16_t level);
    void SetLife(std::uint32_t life);
    void SetShield(std::uint32_t shield);
    void SetNaeRyuk(std::uint32_t naeryuk);
    void SetGenGol();
    void SetMinChub();
    void SetCheRyuk();
    void SetSimMek();
    void SetExpPointPercent(float perc);
    void SetAttackRate();
    void SetDefenseRate();
    void SetCritical();
    void SetAttackRange();
    void SetFame(std::uint32_t fame);
    void SetBadFame(std::uint32_t badfame);
    void SetStage(std::uint8_t stage);

    // 1:1 with legacy point-leveling API.
    void SetPointLeveling(bool val, std::uint16_t point = 0);
    void OnAddPoint(StatPointKind whatsPoint);
    void OnMinusPoint(StatPointKind whatsPoint);

    // 1:1 with legacy SetMinusPointValue.
    void SetMinusPointValue(std::uint16_t point) noexcept { m_MinusPoint = point; }

    // 1:1 with legacy SetPointLevelingHide.
    void SetPointLevelingHide();

    // 1:1 with legacy GetCharacterData.
    CharStaticCtrl* GetCharacterData() noexcept { return &m_ppStatic; }
    const CharStaticCtrl* GetCharacterData() const noexcept { return &m_ppStatic; }
    AttrDefence* GetAttrDefence() noexcept { return &m_AttrDefComponent; }
    const AttrDefence* GetAttrDefence() const noexcept { return &m_AttrDefComponent; }

    int  nocoriPoint()      const noexcept { return m_nocoriPoint; }
    int  minusPoint()       const noexcept { return m_MinusPoint; }
    bool isPointLeveling()  const noexcept { return m_bPointLeveling; }

    // Test hook -- inject the 6 + 6 point buttons.
    using PointBtnArray = std::array<cButton*, kMaxBtnPoint>;
    void SetPointBtnsForTest(const PointBtnArray& plus,
                              const PointBtnArray& minus) {
        for (int i = 0; i < kMaxBtnPoint; ++i) {
            m_pPointBtn[i]      = plus[i];
            m_pPointMinusBtn[i] = minus[i];
        }
    }

    // Test hook -- inject the CHARSTATICCTRL + ATTRDEFENCE
    // children directly (skips the WINDOW_ID walk in Linking).
    void SetChildrenForTest(const CharStaticCtrl& st, const AttrDefence& attr) {
        m_ppStatic          = st;
        m_AttrDefComponent  = attr;
    }

    // Test hook -- inject a "set static text" callback the
    // setters route through.  The legacy writes via
    // m_ppStatic.<field>->SetStaticText; the modern port
    // routes the (field-id, text) pair through this callback
    // so tests can verify the calls.
    using SetStaticTextCallback = void(*)(const char* fieldName,
                                           const char* text,
                                           void* user);
    void SetSetStaticTextCallbackForTest(SetStaticTextCallback cb, void* user) {
        m_setStaticTextCb = cb; m_setStaticTextUser = user;
    }

    // Test hook -- inject an "update attribute" callback.
    using UpdateAttrCallback = void(*)(StatPointKind kind,
                                        std::uint16_t value,
                                        std::uint32_t color,
                                        void* user);
    void SetUpdateAttrCallbackForTest(UpdateAttrCallback cb, void* user) {
        m_updateAttrCb = cb; m_updateAttrUser = user;
    }

    // Test hook -- inject a "guage set value" callback.
    using SetGuageCallback = void(*)(StatPointKind kind,
                                      float value,
                                      void* user);
    void SetSetGuageCallbackForTest(SetGuageCallback cb, void* user) {
        m_setGuageCb = cb; m_setGuageUser = user;
    }

    // Test hook -- inject a "stage / level / etc" callback
    // for stage + level setters.
    using SetStageCallback = void(*)(std::uint8_t stage, void* user);
    void SetSetStageCallbackForTest(SetStageCallback cb, void* user) {
        m_setStageCb = cb; m_setStageUser = user;
    }

    using SetLevelCallback = void(*)(std::uint16_t level, void* user);
    void SetSetLevelCallbackForTest(SetLevelCallback cb, void* user) {
        m_setLevelCb = cb; m_setLevelUser = user;
    }

    // 1:1 with legacy RefreshInfo's main bar notification
    // (legacy: SetPushBarIcon( OPT_CHARDLGICON, m_bActive )).
    // The modern port lets the host inject a callback.
    using MainBarIconCallback = void(*)(bool active, void* user);
    void SetMainBarIconCallbackForTest(MainBarIconCallback cb, void* user) {
        m_mainBarCb = cb; m_mainBarUser = user;
    }

private:
    void SetStaticTextByField(const char* fieldName, const char* text);
    void UpdateAttrByKind(StatPointKind kind, std::uint16_t value,
                           std::uint32_t color);

    CharStaticCtrl m_ppStatic;
    cButton*       m_pPointBtn[kMaxBtnPoint]      = {};
    cButton*       m_pPointMinusBtn[kMaxBtnPoint] = {};
    int            m_nocoriPoint = 0;
    AttrDefence    m_AttrDefComponent;
    int            m_MinusPoint = 0;
    bool           m_bPointLeveling = false;
    int            m_nCurrentLevel  = 1;
    int            m_nAddedGenGol   = 0;
    int            m_nAddedSimMak   = 0;
    int            m_nAddedMinChub  = 0;
    int            m_nAddedCheRyuk  = 0;

    SetStaticTextCallback m_setStaticTextCb   = nullptr;
    void*                 m_setStaticTextUser = nullptr;
    UpdateAttrCallback    m_updateAttrCb      = nullptr;
    void*                 m_updateAttrUser    = nullptr;
    SetGuageCallback      m_setGuageCb        = nullptr;
    void*                 m_setGuageUser      = nullptr;
    SetStageCallback      m_setStageCb        = nullptr;
    void*                 m_setStageUser      = nullptr;
    SetLevelCallback      m_setLevelCb        = nullptr;
    void*                 m_setLevelUser      = nullptr;
    const mxh::services::IPlayerStatsService* m_playerStatsService = nullptr;
    MainBarIconCallback   m_mainBarCb         = nullptr;
    void*                 m_mainBarUser       = nullptr;
};

}  // namespace mxh::ui
