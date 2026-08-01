#include "mxh/ui/charchangedlg.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cGuageBar.hpp"
#include "mxh/ui/cStatic.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

namespace {
constexpr std::size_t kBufferSize = 32;
constexpr float kMinScale = 0.9f;
constexpr float kScaleRange = 0.2f;
}

cCharChangeDlg::cCharChangeDlg() = default;
cCharChangeDlg::~cCharChangeDlg() = default;

void cCharChangeDlg::SetCallbacks(
    ChatTextFn chatText,
    SetItemTableDisabledFn setItemTableDisabled,
    EndObjectStateFn endObjectState,
    SetHeroNameFn heroName,
    SetHeroCharChangeInfoFn setHeroCharChangeInfo,
    TriggerCharacterPartChangeFn triggerPartChange,
    SetHeroScaleFn setHeroScale,
    SendCharacterChangeFn sendCharacterChange,
    void* userData) noexcept {
    m_chatTextFn = chatText;
    m_setItemTableDisabledFn = setItemTableDisabled;
    m_endObjectStateFn = endObjectState;
    m_heroNameFn = heroName;
    m_setHeroCharChangeInfoFn = setHeroCharChangeInfo;
    m_triggerPartChangeFn = triggerPartChange;
    m_setHeroScaleFn = setHeroScale;
    m_sendCharacterChangeFn = sendCharacterChange;
    m_callbackUserData = userData;
}

void cCharChangeDlg::SetControlsForTest(cStatic* name, cStatic* sex,
                                        cStatic* hair, cStatic* face,
                                        cButton* sexBtn0, cButton* sexBtn1,
                                        cGuageBar* height,
                                        cGuageBar* width) noexcept {
    m_pName = name;
    m_pSex = sex;
    m_pHair = hair;
    m_pFace = face;
    m_pSexBtn[0] = sexBtn0;
    m_pSexBtn[1] = sexBtn1;
    m_pHeight = height;
    m_pWidth = width;
}

void cCharChangeDlg::SetItemTablesEnabled(bool disabled) {
    if (!m_setItemTableDisabledFn) {
        return;
    }
    // 1:1 with legacy: SetActive(false) calls SetDisableDialog(FALSE, ...)
    // i.e. disabled is the literal arg to legacy ITEMMGR->SetDisableDialog.
    m_setItemTableDisabledFn(disabled, kItemTableInventory, m_callbackUserData);
    m_setItemTableDisabledFn(disabled, kItemTablePyoguk, m_callbackUserData);
    m_setItemTableDisabledFn(disabled, kItemTableMunpaWarehouse, m_callbackUserData);
    m_setItemTableDisabledFn(disabled, kItemTableShop, m_callbackUserData);
}

bool cCharChangeDlg::HasObjectStateDeal() const {
    // 1:1 with legacy OBJECTSTATEMGR->GetObjectState(HERO) ==
    // eObjectState_Deal. R-12.x: fetch via host-injected callback when
    // OBJECTSTATEMGR is ported. For now, return false so the
    // m_endObjectStateFn branch is skipped during Phase C.
    return false;
}

void cCharChangeDlg::DispatchScale(float h, float w) {
    if (m_setHeroScaleFn) {
        // 1:1 with legacy VECTOR3 scale(w, h, w) -> SetHeroScale(w, h, w).
        m_setHeroScaleFn(w, h, w, m_callbackUserData);
    }
}

void cCharChangeDlg::DispatchHeroCharChangeInfo(const CharacterChangeInfo& info) {
    if (m_setHeroCharChangeInfoFn) {
        m_setHeroCharChangeInfoFn(info, m_callbackUserData);
    }
}

void cCharChangeDlg::RefreshCharacterShape() {
    DispatchHeroCharChangeInfo(m_CharacterInfo);
    if (m_triggerPartChangeFn) {
        m_triggerPartChangeFn(m_callbackUserData);
    }
}

void cCharChangeDlg::FormatGenderText(char* buffer,
                                      std::size_t bufferSize) const {
    if (!buffer || bufferSize == 0) {
        return;
    }
    const char* fmt = m_chatTextFn
        ? m_chatTextFn(m_CharacterInfo.gender == 0 ? kChatGenderMale
                                                   : kChatGenderFemale,
                       m_callbackUserData)
        : nullptr;
    if (fmt) {
        std::snprintf(buffer, bufferSize, "%s", fmt);
    } else {
        buffer[0] = '\0';
    }
}

void cCharChangeDlg::FormatHairText(char* buffer,
                                    std::size_t bufferSize) const {
    if (!buffer || bufferSize == 0) {
        return;
    }
    const char* fmt = m_chatTextFn
        ? m_chatTextFn(kChatHairFormat, m_callbackUserData) : nullptr;
    if (fmt) {
        std::snprintf(buffer, bufferSize, fmt,
                      m_CharacterInfo.hairType + 1);
    } else {
        buffer[0] = '\0';
    }
}

void cCharChangeDlg::FormatFaceText(char* buffer,
                                    std::size_t bufferSize) const {
    if (!buffer || bufferSize == 0) {
        return;
    }
    const char* fmt = m_chatTextFn
        ? m_chatTextFn(kChatFaceFormat, m_callbackUserData) : nullptr;
    if (fmt) {
        std::snprintf(buffer, bufferSize, fmt,
                      m_CharacterInfo.faceType + 1);
    } else {
        buffer[0] = '\0';
    }
}

void cCharChangeDlg::Linking() {
    // 1:1 with legacy CCharChangeDlg::Linking: cast each child
    // pointer from the WINDOW_ID lookup. cDialog::findWindowById does
    // the depth-first walk; the cast is safe at the legacy layer
    // because the matching WINDOW_ID resolves to the right type.
    m_pName = static_cast<cStatic*>(findWindowById(kNameId));
    m_pSex = static_cast<cStatic*>(findWindowById(kSexId));
    m_pHair = static_cast<cStatic*>(findWindowById(kHairId));
    m_pFace = static_cast<cStatic*>(findWindowById(kFaceId));
    m_pSexBtn[0] = static_cast<cButton*>(findWindowById(kSexButton0Id));
    m_pSexBtn[1] = static_cast<cButton*>(findWindowById(kSexButton1Id));
    m_pHeight = static_cast<cGuageBar*>(findWindowById(kHeightId));
    m_pWidth = static_cast<cGuageBar*>(findWindowById(kWidthId));
    m_ItemPos = 0;
    m_bShapeChange = false;
}

void cCharChangeDlg::SetActive(bool val) noexcept {
    cDialog::SetActive(val);
    if (!val) {
        // 1:1 with legacy: re-enable inventory/warehouse/shop dialogs
        // (disabled=FALSE) when the charchange dialog closes.
        SetItemTablesEnabled(false);
        if (HasObjectStateDeal() && m_endObjectStateFn) {
            m_endObjectStateFn(m_callbackUserData);
        }
        return;
    }
    const bool enableShapeControls = !m_bShapeChange;
    for (std::size_t i = 0; i < kSexButtonCount; ++i) {
        if (m_pSexBtn[i]) {
            m_pSexBtn[i]->SetActive(enableShapeControls);
        }
    }
    if (m_pHeight) {
        m_pHeight->SetActive(enableShapeControls);
    }
    if (m_pWidth) {
        m_pWidth->SetActive(enableShapeControls);
    }
}

void cCharChangeDlg::Process() {
    if (m_bShapeChange || !m_pHeight || !m_pWidth) {
        return;
    }
    const float rateH = m_pHeight->GetCurRate();
    const float rateW = m_pWidth->GetCurRate();
    const float bh = kMinScale + rateH * kScaleRange;
    const float bw = kMinScale + rateW * kScaleRange;
    if (bh == m_CharacterInfo.height && bw == m_CharacterInfo.width) {
        return;
    }
    m_CharacterInfo.height = bh;
    m_CharacterInfo.width = bw;
    DispatchScale(bh, bw);
}

void cCharChangeDlg::SetCharacterInfo(const CharacterChangeInfo& info) {
    m_CharacterInfoBackup = info;
    m_CharacterInfo = info;
    if (m_pName) {
        if (m_heroNameFn) {
            m_pName->SetStaticText(m_heroNameFn());
        } else {
            m_pName->SetStaticText("");
        }
    }
    char buf[kBufferSize]{};
    if (m_pSex) {
        FormatGenderText(buf, sizeof(buf));
        m_pSex->SetStaticText(buf);
    }
    if (m_pHair) {
        FormatHairText(buf, sizeof(buf));
        m_pHair->SetStaticText(buf);
    }
    if (m_pFace) {
        FormatFaceText(buf, sizeof(buf));
        m_pFace->SetStaticText(buf);
    }
    if (m_pHeight) {
        m_pHeight->SetCurRate(static_cast<float>((info.height - kMinScale)
                                                  * kHeightRateMultiplier));
    }
    if (m_pWidth) {
        m_pWidth->SetCurRate(static_cast<float>((info.width - kMinScale)
                                                 * kHeightRateMultiplier));
    }
    RefreshCharacterShape();
}

void cCharChangeDlg::Reset(bool bSave) {
    if (bSave) {
        DispatchHeroCharChangeInfo(m_CharacterInfo);
    } else {
        DispatchHeroCharChangeInfo(m_CharacterInfoBackup);
        if (m_triggerPartChangeFn) {
            m_triggerPartChangeFn(m_callbackUserData);
        }
    }
    m_ItemPos = 0;
    m_bShapeChange = false;
    m_CharacterInfoBackup = CharacterChangeInfo{};
    m_CharacterInfo = CharacterChangeInfo{};
}

void cCharChangeDlg::ChangeSexType(bool /*bPrev*/) {
    if (m_bShapeChange) {
        return;
    }
    m_CharacterInfo.gender = m_CharacterInfo.gender == 0 ? 1 : 0;
    char buf[kBufferSize]{};
    FormatGenderText(buf, sizeof(buf));
    if (m_pSex) {
        m_pSex->SetStaticText(buf);
    }
    RefreshCharacterShape();
}

void cCharChangeDlg::ChangeHairType(bool bPrev) {
    // 1:1 with legacy: NO shape-change gate here. ChangeSexType has
    // the gate; ChangeHairType / ChangeFaceType do not.
    auto& h = m_CharacterInfo.hairType;
    if (bPrev) {
        h = h == kHairTypeMin ? static_cast<std::uint8_t>(kHairTypeMax)
                              : static_cast<std::uint8_t>(h - 1);
    } else {
        h = h == kHairTypeMax ? static_cast<std::uint8_t>(kHairTypeMin)
                              : static_cast<std::uint8_t>(h + 1);
    }
    char buf[kBufferSize]{};
    FormatHairText(buf, sizeof(buf));
    if (m_pHair) {
        m_pHair->SetStaticText(buf);
    }
    RefreshCharacterShape();
}

void cCharChangeDlg::ChangeFaceType(bool bPrev) {
    // 1:1 with legacy: NO shape-change gate here.
    auto& f = m_CharacterInfo.faceType;
    if (bPrev) {
        f = f == kFaceTypeMin ? static_cast<std::uint8_t>(kFaceTypeMax)
                              : static_cast<std::uint8_t>(f - 1);
    } else {
        f = f == kFaceTypeMax ? static_cast<std::uint8_t>(kFaceTypeMin)
                              : static_cast<std::uint8_t>(f + 1);
    }
    char buf[kBufferSize]{};
    FormatFaceText(buf, sizeof(buf));
    if (m_pFace) {
        m_pFace->SetStaticText(buf);
    }
    RefreshCharacterShape();
}

void cCharChangeDlg::CharacterChangeSyn() {
    if (m_sendCharacterChangeFn) {
        m_sendCharacterChangeFn(m_ItemPos, m_CharacterInfo, m_callbackUserData);
    }
    SetActive(false);
}

}  // namespace mxh::ui
