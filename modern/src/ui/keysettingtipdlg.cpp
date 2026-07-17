// keysettingtipdlg.cpp — 1:1 port of 墨香 CKeySettingTipDlg
// (keyboard shortcut tip dialog). See
// keysettingtipdlg.hpp for the data-model rationale
// + 1:1 quirks.

#include "keysettingtipdlg.hpp"

namespace mxh::ui {

cKeySettingTipDlg::cKeySettingTipDlg() {
    // 1:1 with legacy CKeySettingTipDlg::CKeySettingTipDlg:
    //   m_wMode = 2; (default to hidden mode)
    // Modern port uses default member init in the
    // header (m_wMode = kModeHidden) so the body
    // is empty.
}

cKeySettingTipDlg::~cKeySettingTipDlg() = default;

void cKeySettingTipDlg::Linking() {
    // 1:1 with legacy CKeySettingTipDlg::Linking.
    // The legacy is:
    //   RECT rt;
    //   rt.top = rt.left = 0;
    //   rt.right = 1024;
    //   rt.bottom = 768;
    //   m_KeyImage[0].LoadSprite("Image/2D/KeySetting1.tga");
    //   m_KeyImage[0].SetImageSrcRect(&rt);
    //   m_KeyImage[1].LoadSprite("Image/2D/KeySetting2.tga");
    //   m_KeyImage[1].SetImageSrcRect(&rt);
    //
    // The modern port:
    //   - Stores the 2 .tga resource paths as
    //     std::string (1:1 with legacy path
    //     strings).
    //   - cImageSelf::LoadSprite and
    //     cImageSelf::SetImageSrcRect are TODO
    //     (cImageSelf not ported, R-12.x deferred).
    //   - When cImageSelf is ported, the body
    //     becomes:
    //       RECT rt = {0, 0, 1024, 768};
    //       m_KeyImage[0].LoadSprite(m_imagePaths[0].c_str());
    //       m_KeyImage[0].SetImageSrcRect(&rt);
    //       m_KeyImage[1].LoadSprite(m_imagePaths[1].c_str());
    //       m_KeyImage[1].SetImageSrcRect(&rt);
    m_imagePaths[0] = "Image/2D/KeySetting1.tga";
    m_imagePaths[1] = "Image/2D/KeySetting2.tga";
    // TODO: cImageSelf not ported (R-12.x deferred).
    //       When ported, add LoadSprite +
    //       SetImageSrcRect calls on the 2 slots.
}

}  // namespace mxh::ui
