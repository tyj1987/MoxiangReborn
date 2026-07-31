// ckeysettingtipdlg.cpp -- modern implementation of Moxiang
//   CKeySettingTipDlg (keyboard shortcut tip dialog).

#include "ckeysettingtipdlg.hpp"

namespace mxh::ui {

cKeySettingTipDlg::cKeySettingTipDlg() = default;

cKeySettingTipDlg::~cKeySettingTipDlg() = default;

void cKeySettingTipDlg::Linking() {
    // 1:1 with legacy CKeySettingTipDlg::Linking.
    // The legacy body is:
    //   RECT rt = {0, 0, 1024, 768};
    //   m_KeyImage[0].LoadSprite("Image/2D/KeySetting1.tga");
    //   m_KeyImage[0].SetImageSrcRect(&rt);
    //   m_KeyImage[1].LoadSprite("Image/2D/KeySetting2.tga");
    //   m_KeyImage[1].SetImageSrcRect(&rt);
    //
    // Modern port:
    //   - Stores the 2 .tga resource paths as
    //     std::string (1:1 with legacy path strings).
    //   - Fires host-injected LoadSprite + SetImageSrcRect
    //     callbacks for each slot (cImageSelf framework
    //     is R-12.x deferred).
    m_imagePaths[0] = kPathImage0;
    m_imagePaths[1] = kPathImage1;
    if (m_loadSpriteCb) {
        m_loadSpriteCb(0, m_imagePaths[0].c_str(), m_loadSpriteUser);
        m_loadSpriteCb(1, m_imagePaths[1].c_str(), m_loadSpriteUser);
    }
    if (m_setSrcRectCb) {
        m_setSrcRectCb(0, kSrcRectLeft, kSrcRectTop,
                       kSrcRectRight, kSrcRectBottom, m_setSrcRectUser);
        m_setSrcRectCb(1, kSrcRectLeft, kSrcRectTop,
                       kSrcRectRight, kSrcRectBottom, m_setSrcRectUser);
    }
}

void cKeySettingTipDlg::Render() {
    // 1:1 with legacy CKeySettingTipDlg::Render. The
    // legacy body is:
    //   if (!m_bActive) return;
    //   if (m_wMode > 1) return;
    //   cDialog::RenderWindow();
    //   m_KeyImage[m_wMode].RenderSprite(&vScale, NULL, 0,
    //                                    &m_absPos, color);
    //   cDialog::RenderComponent();
    //
    // Modern port:
    //   - 1:1 with the 2 guards (if !m_bActive or
    //     m_wMode > 1, return immediately).
    //   - 1:1 with the 3-step render order.
    if (!isActive()) {
        return;
    }
    if (m_wMode > 1u) {
        // 1:1 quirk: legacy uses `m_wMode > 1` not
        // `m_wMode >= 2`. kModeHidden is 2, so this
        // is equivalent.
        return;
    }
    if (m_renderCb) {
        m_renderCb(static_cast<int>(m_wMode),
                   RenderStep::RenderWindow, m_renderUser);
        m_renderCb(static_cast<int>(m_wMode),
                   RenderStep::RenderSprite, m_renderUser);
        m_renderCb(static_cast<int>(m_wMode),
                   RenderStep::RenderComponent, m_renderUser);
    }
}

}  // namespace mxh::ui