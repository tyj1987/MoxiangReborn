// chasedialog.hpp — modern port of 墨香 CChaseDialog
// (chase target dialog: shows minimap + target position
// + target name + map info).
//
// 1:1 port of legacy `CChaseDialog` from
//   `墨香【源码】\[Client]MH\ChaseDialog.h` (775 B) and
//   `墨香【源码】\[Client]MH\ChaseDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_CHASE_DLG (legacy cWindow
//     type tag; modern cWindow / cDialog don't have
//     m_type, so modern port drops the ctor body).
//   - Linking: resolve cStatic m_pMap + cTextArea
//     m_TextArea by id, init m_bActive = FALSE, init
//     m_MapNum = 0, fetch m_pIconImage from
//     SCRIPTMGR. The _JP_LOCAL_ / _HK_LOCAL_ / _TL_LOCAL_
//     PosMsg localizations are wrapped in #ifdef
//     (1:1 quirk: locale-specific message blocks are
//     not in the modern port's scope).
//   - SetActive override: 1:1 with base noexcept. Body:
//     base SetActive + m_bActive = val.
//   - InitMiniMap: 1:1 wrapper. Sets m_EventMapNum +
//     m_TargetPos + m_WantedName + calls
//     LoadMinimapImageInfo.
//   - LoadMinimapImageInfo: 7-singleton dispatch
//     (DIRECTORYMGR/GAMERESRCMNGR/CMHFile).
//   - Render: real GPU draw (the minimap + target
//     icon). Modern port is a no-op stub (Phase 6.13+
//     deferred).
//
// The modern port covers the public API:
//   - Linking REAL (resolve 2 children + init m_bActive +
//     m_MapNum)
//   - SetActive override
//   - InitMiniMap: data-model update (m_EventMapNum +
//     m_TargetPos + m_WantedName); LoadMinimapImageInfo
//     is TODO (7-singleton).
//   - LoadMinimapImageInfo: TODO.
//   - Render: no-op stub.
//
// The unported types (MINIMAPIMAGE / cImageSelf /
// VECTOR2 / MAPTYPE) are not in modern — the modern
// port uses placeholder data fields (int / float /
// std::string) that match the 1:1 semantics without
// requiring the unported types. When the minimap
// sub-system is ported, the placeholders can be
// replaced with the real types.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 15th **Tier 2** dialog port. The dialog
// exercises cTextArea (already ported in 0.13.23) +
// cStatic (already ported). The minimap sub-system
// (MINIMAPIMAGE + cImageSelf + minimap sprite
// rendering) is a future Tier 3+ work item that
// will be ported when the world rendering layer
// grows to support it.
//
// 1:1 quirks preserved:
//   - Ctor drops m_type = WT_CHASE_DLG (legacy
//     cWindow type tag removed in Phase 6).
//   - The legacy's m_pIconImage is fetched from
//     SCRIPTMGR->GetImage(126, ...). Modern port
//     drops the SCRIPTMGR singleton call (the
//     minimap icon is Phase 6.13+ deferred).
//   - The legacy's _JP_LOCAL_ / _HK_LOCAL_ / _TL_LOCAL_
//     PosMsg localizations are not in the modern
//     port (1:1 quirk: locale-specific code blocks
//     are out of scope for the initial 1:1 port).
//   - SetActive matches base noexcept (R-12 polymorphic
//     virtual required).
//   - The unported types (MINIMAPIMAGE / cImageSelf /
//     VECTOR2 / MAPTYPE) are replaced with placeholder
//     types (int / float / std::string).
//   - LoadMinimapImageInfo + the actual minimap render
//     are documented as TODO.

#pragma once

#include "cdialog.hpp"

#include <cstdint>
#include <string>

namespace mxh::ui {

class cStatic;
class cTextArea;

class cChaseDialog : public cDialog {
public:
    cChaseDialog();
    ~cChaseDialog() override;

    // ----- 1:1 with legacy CChaseDialog::Linking -----

    // Resolves 2 children (cStatic m_pMap + cTextArea
    // m_TextArea) by id 310-311 + inits m_bActive +
    // m_MapNum. The SCRIPTMGR icon fetch + the locale
    // localizations are documented as TODO.
    void Linking();

    // ----- 1:1 with legacy CChaseDialog::SetActive -----

    // 1:1 override: calls base SetActive + m_bActive
    // = val.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CChaseDialog::InitMiniMap -----

    // Sets m_EventMapNum + m_TargetPos + m_WantedName +
    // calls LoadMinimapImageInfo. Modern port updates
    // the data-model fields; LoadMinimapImageInfo is
    // TODO (7-singleton dispatch).
    bool InitMiniMap(int mapNum, std::uint16_t posX,
                     std::uint16_t posY, const char* strName,
                     int eventMapNum);

    // ----- 1:1 with legacy CChaseDialog::LoadMinimapImageInfo -----

    // 7-singleton dispatch. Modern port: TODO until
    // DIRECTORYMGR + GAMERESRCMNGR + CMHFile +
    // minimap sprite are ported.
    bool LoadMinimapImageInfo(int mapNum);

    // ----- 1:1 with legacy CChaseDialog::Render -----

    // Real GPU draw (minimap + target icon). Modern
    // port is a no-op stub (Phase 6.13+ deferred).
    void Render() override {}

    // ----- Accessors (used by tests) -----

    cStatic*  GetMap()       const noexcept { return m_pMap; }
    cTextArea* GetTextArea() const noexcept { return m_TextArea; }
    bool       IsChaseActive() const noexcept { return m_bActive; }
    int        GetMapNum()   const noexcept { return m_MapNum; }
    int        GetEventMapNum() const noexcept { return m_EventMapNum; }
    float      GetTargetPosX() const noexcept { return m_TargetPosX; }
    float      GetTargetPosY() const noexcept { return m_TargetPosY; }
    const std::string& GetWantedName() const noexcept { return m_WantedName; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kMapId       = 310;  // was SEE_CHASE_MAP
    static constexpr std::int32_t kTextAreaId  = 311;  // was SEE_CHASE_MSG

    // 1:1 quirk: legacy MAX_NAME_LENGTH+1 = 17+1 = 18
    // (the legacy's char m_WantedName[MAX_NAME_LENGTH+1]).
    static constexpr std::size_t kMaxWantedNameLen = 18;

private:
    cStatic*  m_pMap      = nullptr;
    cTextArea* m_TextArea = nullptr;

    // 1:1 with legacy state (legacy uses BOOL / VECTOR2
    // / MAPTYPE / char[]). Modern port uses bool / float
    // / int / std::string.
    bool         m_bActive     = false;
    float        m_TargetPosX  = 0.0f;
    float        m_TargetPosY  = 0.0f;
    int          m_MapNum      = 0;
    int          m_EventMapNum = 0;
    std::string  m_WantedName;
};

}  // namespace mxh::ui
