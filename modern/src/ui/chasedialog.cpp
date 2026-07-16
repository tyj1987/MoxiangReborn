// chasedialog.cpp — 1:1 port of 墨香 CChaseDialog
// (chase target dialog). See chasedialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "chasedialog.hpp"
#include "cstatic.hpp"
#include "ctextarea.hpp"

#include <cstring>

namespace mxh::ui {

cChaseDialog::cChaseDialog() = default;

cChaseDialog::~cChaseDialog() = default;

void cChaseDialog::Linking() {
    // 1:1 with legacy CChaseDialog::Linking. REAL —
    // resolve 2 children + init m_bActive + m_MapNum.
    // The SCRIPTMGR->GetImage(126, &m_pIconImage) call
    // is dropped (1:1 quirk: the minimap icon is
    // Phase 6.13+ deferred). The locale localizations
    // (_JP_LOCAL_ / _HK_LOCAL_ / _TL_LOCAL_) are not
    // in the modern port (1:1 quirk: locale-specific
    // code blocks are out of scope).
    m_pMap      = static_cast<cStatic*>(findWindowById(kMapId));
    m_TextArea  = static_cast<cTextArea*>(findWindowById(kTextAreaId));
    m_bActive   = false;
    m_MapNum    = 0;
    // TODO: fetch m_pIconImage from
    //       SCRIPTMGR->GetImage(126, &m_pIconImage) once
    //       SCRIPTMGR + cImage::LoadSprite are ported.
    //       The _JP_LOCAL_ / _HK_LOCAL_ / _TL_LOCAL_
    //       PosMsg localizations are also deferred.
}

void cChaseDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CChaseDialog::SetActive. The
    // legacy is:
    //   cDialog::SetActive(val);
    //   m_bActive = val;
    cDialog::SetActive(val);
    m_bActive = val;
}

bool cChaseDialog::InitMiniMap(int mapNum, std::uint16_t posX,
                              std::uint16_t posY, const char* strName,
                              int eventMapNum) {
    // 1:1 with legacy CChaseDialog::InitMiniMap. The
    // legacy is:
    //   MAPTYPE LoadMap = MapNum;
    //   if (EventMapNum == 44) LoadMap = EventMapNum;
    //   m_EventMapNum = EventMapNum;
    //   if (!LoadMinimapImageInfo(LoadMap)) return FALSE;
    //   m_TargetPos.x = PosX;
    //   m_TargetPos.y = PosY;
    //   SafeStrCpy(m_WantedName, strName, MAX_NAME_LENGTH+1);
    //   return TRUE;
    //
    // Modern port updates the data-model fields +
    // delegates the LoadMinimapImageInfo call (TODO).
    m_EventMapNum = eventMapNum;
    if (!LoadMinimapImageInfo(mapNum)) {
        return false;
    }
    m_TargetPosX = static_cast<float>(posX);
    m_TargetPosY = static_cast<float>(posY);
    if (strName) {
        // Truncate to kMaxWantedNameLen - 1 to match
        // the legacy SafeStrCpy(buf, MAX_NAME_LENGTH+1)
        // behavior.
        if (std::strlen(strName) >= kMaxWantedNameLen) {
            m_WantedName.assign(strName, kMaxWantedNameLen - 1);
        } else {
            m_WantedName = strName;
        }
    } else {
        m_WantedName.clear();
    }
    return true;
}

bool cChaseDialog::LoadMinimapImageInfo(int /*mapNum*/) {
    // 1:1 with legacy CChaseDialog::LoadMinimapImageInfo.
    // The legacy reads a Minimap<N>.bin / Minimap<N>.txt
    // file from DIRECTORYMGR, parses the image width +
    // height + sprite filename, and loads the sprite.
    // 7-singleton dispatch: DIRECTORYMGR + GAMERESRCMNGR
    // + CMHFile + the minimap sprite + a few more.
    //
    // Modern port: TODO. The function returns false
    // (the legacy returns FALSE on file-not-found,
    // which is the common case during 1:1 port
    // testing). When the minimap sub-system is
    // ported, this will be implemented.
    return false;
}

}  // namespace mxh::ui
