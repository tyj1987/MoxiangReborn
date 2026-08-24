//
// 1:1 port of legacy `CCharChangeDlg` from
//   `墨香【源码】\[Client]MH\CharChangeDlg.h` / `.cpp`.
//
// The character-change dialog is opened when the player uses a
// 2nd character-change item (e.g. Character Shape/Wedding dress
// change item). It shows the player's hero on screen with sliders
// for height/width + buttons for sex/hair/face selection; the
// player can preview the change and then click "Apply" to send
// the new params to the server. The modern port keeps the 1:1
// surface:
//   - 4 cStatic children: name, sex, hair, face
//   - 2 cButton sex-type buttons (sexBtn[0]/sexBtn[1])
//   - 2 cGuageBar sliders (height/width)
//   - 8 host-injected callbacks (chat text, item-table-toggle,
//     object-state-end, hero name, hero char-change-info,
//     trigger character part change, hero scale, send packet)
//   - SetActive(bool) toggles inventory/Warehouse/Shop locks
//   - Reset(true/false) keeps or restores backup info
//   - Process() every frame reads CurRate from the two guages
//     and updates the hero's engine scale
//
// 1:1 quirks preserved:
//   - SetCharacterInfo() calls SetCurRate on the guages using
//     the legacy formula `(value - 0.9) * 5`.
//   - SetActive(false) always disables the 4 item tables via
//     `!enabled` (i.e. enabled=false -> disabled=true to the
//     legacy ITEMMGR API).
//   - ChangeSexType / ChangeHairType / ChangeFaceType are
//     no-ops when m_bShapeChange is true (read-only mode).
//   - ChangeHairType wraps at min=0/max=4 (kHairTypeCount=5).
//   - ChangeFaceType wraps at min=0/max=4 (kFaceTypeCount=5).
//   - The character chat-message IDs (1180..1183) are the
//     legacy Gender/Hair/Face formatter strings.
//
#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;
class cButton;
class cGuageBar;

struct CharacterChangeInfo {
    std::uint8_t gender = 0;
    std::uint8_t hairType = 0;
    std::uint8_t faceType = 0;
    float height = 1.0f;
    float width = 1.0f;
};

class cCharChangeDlg final : public cDialog {
public:
    // ----- 1:1 callbacks (legacy: GUILDMGR / CHATMGR / HERO / -----
    // OBJECTSTATEMGR / APPEARANCEMGR / ITEMMGR / NETWORK globals). -----
    using ChatTextFn = const char* (*)(std::int32_t messageId, void* userData);
    using SetItemTableDisabledFn = void (*)(bool disabled, std::int32_t tableId,
                                            void* userData);
    using EndObjectStateFn = void (*)(void* userData);
    using SetHeroNameFn = const char* (*)();
    using SetHeroCharChangeInfoFn = void (*)(const CharacterChangeInfo& info,
                                             void* userData);
    using TriggerCharacterPartChangeFn = void (*)(void* userData);
    using SetHeroScaleFn = void (*)(float x, float y, float z, void* userData);
    using SendCharacterChangeFn = void (*)(std::uint32_t itemPos,
                                           const CharacterChangeInfo& info,
                                           void* userData);

    // ----- 1:1 with legacy WindowIDs.h (comment-stripped parse) -----
    // CHA_CHANGEDLG = 1436
    static constexpr std::int32_t kChgDlgId = 1436;
    // CHA_NAME = 1437
    static constexpr std::int32_t kNameId = 1437;
    // CHA_CharMake = 1438
    static constexpr std::int32_t kCharMakeId = 1438;
    // CHA_CharCancel = 1439
    static constexpr std::int32_t kCharCancelId = 1439;
    // CHA_SexType = 1440
    static constexpr std::int32_t kSexId = 1440;
    // CHA_HairType = 1441
    static constexpr std::int32_t kHairId = 1441;
    // CHA_FaceType = 1442
    static constexpr std::int32_t kFaceId = 1442;
    // CHA_SexType1 = 1443
    static constexpr std::int32_t kSexButton0Id = 1443;
    // CHA_SexType2 = 1444
    static constexpr std::int32_t kSexButton1Id = 1444;
    // CHA_Height = 1449
    static constexpr std::int32_t kHeightId = 1449;
    // CHA_Width = 1450
    static constexpr std::int32_t kWidthId = 1450;

    // ----- 1:1 with legacy gameplay constants -----
    static constexpr std::int32_t kHairTypeMin = 0;
    static constexpr std::int32_t kHairTypeMax = 4;
    static constexpr std::int32_t kFaceTypeMin = 0;
    static constexpr std::int32_t kFaceTypeMax = 4;
    static constexpr std::int32_t kHeightRateMultiplier = 5;
    static constexpr std::int32_t kChatGenderMale = 1180;
    static constexpr std::int32_t kChatGenderFemale = 1181;
    static constexpr std::int32_t kChatHairFormat = 1182;
    static constexpr std::int32_t kChatFaceFormat = 1183;
    static constexpr std::int32_t kItemTableInventory = 0;
    static constexpr std::int32_t kItemTablePyoguk = 1;
    static constexpr std::int32_t kItemTableMunpaWarehouse = 2;
    static constexpr std::int32_t kItemTableShop = 3;
    static constexpr std::size_t kHairTypeCount = 5;
    static constexpr std::size_t kFaceTypeCount = 5;
    static constexpr std::size_t kSexButtonCount = 2;

    cCharChangeDlg();
    ~cCharChangeDlg() override;

    cCharChangeDlg(const cCharChangeDlg&) = delete;
    cCharChangeDlg& operator=(const cCharChangeDlg&) = delete;

    // 1:1 with legacy Linking() / SetActive / Process / SetCharacterInfo.
    void Linking();
    void SetActive(bool val) noexcept override;
    void Process();
    void SetCharacterInfo(const CharacterChangeInfo& info);
    void SetItemInfo(std::uint32_t pos) noexcept { m_ItemPos = pos; }
    void SetShapeChange(bool val) noexcept { m_bShapeChange = val; }

    // 1:1 with legacy Reset(bool) / ChangeSexType / ChangeHairType /
    // ChangeFaceType / CharacterChangeSyn.
    void Reset(bool bSave);
    void ChangeSexType(bool bPrev);
    void ChangeHairType(bool bPrev);
    void ChangeFaceType(bool bPrev);
    void CharacterChangeSyn();

    // Test seam: bypass Linking() WINDOW_ID walk.
    void SetControlsForTest(cStatic* name, cStatic* sex, cStatic* hair,
                            cStatic* face, cButton* sexBtn0, cButton* sexBtn1,
                            cGuageBar* height, cGuageBar* width) noexcept;

    // Test seam: replace global manager calls with host-injected
    // function pointers + userdata.
    void SetCallbacks(ChatTextFn chatText,
                      SetItemTableDisabledFn setItemTableDisabled,
                      EndObjectStateFn endObjectState,
                      SetHeroNameFn heroName,
                      SetHeroCharChangeInfoFn setHeroCharChangeInfo,
                      TriggerCharacterPartChangeFn triggerPartChange,
                      SetHeroScaleFn setHeroScale,
                      SendCharacterChangeFn sendCharacterChange,
                      void* userData = nullptr) noexcept;

    // Read-only accessors (used by tests).
    CharacterChangeInfo info() const noexcept { return m_CharacterInfo; }
    CharacterChangeInfo backupInfo() const noexcept { return m_CharacterInfoBackup; }
    std::uint32_t itemPos() const noexcept { return m_ItemPos; }
    bool shapeChange() const noexcept { return m_bShapeChange; }

    cStatic* nameStatic() const noexcept { return m_pName; }
    cStatic* sexStatic() const noexcept { return m_pSex; }
    cStatic* hairStatic() const noexcept { return m_pHair; }
    cStatic* faceStatic() const noexcept { return m_pFace; }
    cButton* sexButton(std::size_t idx) const noexcept {
        return idx < kSexButtonCount ? m_pSexBtn[idx] : nullptr;
    }
    cGuageBar* heightGuage() const noexcept { return m_pHeight; }
    cGuageBar* widthGuage() const noexcept { return m_pWidth; }

private:
    void RefreshCharacterShape();
    void SetItemTablesEnabled(bool enabled);
    bool HasObjectStateDeal() const;
    void FormatHairText(char* buffer, std::size_t bufferSize) const;
    void FormatFaceText(char* buffer, std::size_t bufferSize) const;
    void FormatGenderText(char* buffer, std::size_t bufferSize) const;
    void DispatchScale(float height, float width);
    void DispatchHeroCharChangeInfo(const CharacterChangeInfo& info);

    cStatic* m_pName = nullptr;
    cStatic* m_pSex = nullptr;
    cStatic* m_pHair = nullptr;
    cStatic* m_pFace = nullptr;
    cButton* m_pSexBtn[kSexButtonCount] = {nullptr, nullptr};
    cGuageBar* m_pHeight = nullptr;
    cGuageBar* m_pWidth = nullptr;

    CharacterChangeInfo m_CharacterInfoBackup{};
    CharacterChangeInfo m_CharacterInfo{};
    std::uint32_t m_ItemPos = 0;
    bool m_bShapeChange = false;

    ChatTextFn m_chatTextFn = nullptr;
    SetItemTableDisabledFn m_setItemTableDisabledFn = nullptr;
    EndObjectStateFn m_endObjectStateFn = nullptr;
    SetHeroNameFn m_heroNameFn = nullptr;
    SetHeroCharChangeInfoFn m_setHeroCharChangeInfoFn = nullptr;
    TriggerCharacterPartChangeFn m_triggerPartChangeFn = nullptr;
    SetHeroScaleFn m_setHeroScaleFn = nullptr;
    SendCharacterChangeFn m_sendCharacterChangeFn = nullptr;
    void* m_callbackUserData = nullptr;
};

}  // namespace mxh::ui
