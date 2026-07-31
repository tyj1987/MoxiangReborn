// ckeysettingtipdlg.hpp -- modern port of Moxiang
//   CKeySettingTipDlg (keyboard shortcut tip dialog).
//
// 1:1 port of legacy `CKeySettingTipDlg` from
//   `[Client]MH\KeySettingTipDlg.{h,cpp}`.
//
// Surface (legacy):
//   - Ctor: m_wMode = 2 (default to hidden mode).
//   - Dtor: empty body.
//   - Linking: 2 cImageSelf slots, each loads
//     "Image/2D/KeySetting1.tga" / "KeySetting2.tga"
//     and SetImageSrcRect(0,0,1024,768).
//   - SetMode(WORD): inline setter, m_wMode = mode.
//   - Render: 2 guards (`!m_bActive` and `m_wMode
//     > 1`), then cDialog::RenderWindow + RenderSprite
//     for image[m_wMode] + cDialog::RenderComponent.
//
// Modern port:
//   - Ctor: m_wMode = kModeHidden (default member init).
//   - Dtor: default.
//   - Linking: stores 2 image paths as std::string +
//     fires host-injected LoadSprite + SetImageSrcRect
//     callbacks (cImageSelf is R-12.x deferred).
//   - SetMode / GetMode: 1:1 with legacy.
//   - Render: 1:1 with legacy guards + dispatches
//     3 host-injected RenderWindow / RenderSprite /
//     RenderComponent callbacks in order.
//
// 1:1 quirks:
//   - 1:1 with legacy `m_wMode = 2` (default hidden).
//   - 1:1 with legacy `if(!m_bActive) return;` guard.
//   - 1:1 with legacy `if(m_wMode > 1) return;` guard.
//   - 1:1 with legacy Render order: RenderWindow first,
//     then RenderSprite with m_wMode index, then
//     RenderComponent last.
//   - 1:1 with legacy image source rect (0,0,1024,768).
//   - 1:1 with legacy 2 image paths (KeySetting1/2.tga).

#pragma once

#include "cdialog.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace mxh::ui {

class cKeySettingTipDlg : public cDialog {
public:
    cKeySettingTipDlg();
    ~cKeySettingTipDlg() override;

    cKeySettingTipDlg(const cKeySettingTipDlg&) = delete;
    cKeySettingTipDlg& operator=(const cKeySettingTipDlg&) = delete;

    // ----- 1:1 with legacy CKeySettingTipDlg::Linking -----

    // 1:1 with legacy Linking. Stores the 2 .tga
    // resource paths (1:1 with legacy's "Image/2D/
    // KeySetting{1,2}.tga") and fires host-injected
    // LoadSprite + SetImageSrcRect callbacks for
    // each slot. cImageSelf is R-12.x deferred;
    // when ported, the body becomes:
    //   m_KeyImage[0].LoadSprite(m_imagePaths[0].c_str());
    //   m_KeyImage[0].SetImageSrcRect(&rt);
    //   m_KeyImage[1].LoadSprite(m_imagePaths[1].c_str());
    //   m_KeyImage[1].SetImageSrcRect(&rt);
    void Linking();

    // ----- 1:1 with legacy CKeySettingTipDlg::SetMode -----

    // 1:1 with legacy inline SetMode setter.
    void SetMode(std::uint16_t mode) noexcept { m_wMode = mode; }

    // 1:1 with legacy m_wMode getter.
    std::uint16_t GetMode() const noexcept { return m_wMode; }

    // ----- 1:1 with legacy CKeySettingTipDlg::Render override -----

    // 1:1 with legacy Render. Two guards:
    //   if (!m_bActive) return;
    //   if (m_wMode > 1) return;
    // then 3 host-injected callbacks in order:
    //   1) RenderWindow
    //   2) RenderSprite (with m_wMode index)
    //   3) RenderComponent
    // The actual cImageSelf::RenderSprite body is
    // TODO (R-12.x deferred) -- the modern port
    // dispatches via callbacks so tests can assert
    // the order without needing the GPU sprite.
    void Render() override;

    // ----- 1:1 constants -----

    // 1:1 with legacy m_wMode = 2 default. Modes 0/1
    // show image 0/1; mode 2 (and above) hides.
    static constexpr std::uint16_t kModeHidden = 2;

    // 1:1 with legacy m_KeyImage[2] (2 cImageSelf slots).
    static constexpr std::size_t kNumImages = 2;

    // 1:1 with legacy image source rect (0,0,1024,768).
    static constexpr int kSrcRectLeft   = 0;
    static constexpr int kSrcRectTop    = 0;
    static constexpr int kSrcRectRight  = 1024;
    static constexpr int kSrcRectBottom = 768;

    // 1:1 with legacy image paths.
    static constexpr const char* kPathImage0 = "Image/2D/KeySetting1.tga";
    static constexpr const char* kPathImage1 = "Image/2D/KeySetting2.tga";

    // ----- 1:1 callbacks for legacy cImageSelf (R-12.x deferred) -----

    // 1:1 with legacy cImageSelf::LoadSprite(slot, path).
    // Host wires the actual sprite load (R-12.x deferred).
    using LoadSpriteCallback = void(*)(int slot, const char* path, void* user);
    void SetLoadSpriteCallbackForTest(LoadSpriteCallback cb,
                                      void* user) noexcept {
        m_loadSpriteCb   = cb;
        m_loadSpriteUser = user;
    }

    // 1:1 with legacy cImageSelf::SetImageSrcRect(slot, &rt).
    // The rect is (0,0,1024,768) -- matches kSrcRect*.
    using SetImageSrcRectCallback = void(*)(int slot, int left, int top,
                                              int right, int bottom, void* user);
    void SetImageSrcRectCallbackForTest(SetImageSrcRectCallback cb,
                                        void* user) noexcept {
        m_setSrcRectCb   = cb;
        m_setSrcRectUser = user;
    }

    // 1:1 with legacy Render's 3-step:
    //   cDialog::RenderWindow();
    //   m_KeyImage[m_wMode].RenderSprite(...);
    //   cDialog::RenderComponent();
    // The single callback receives a step tag (0/1/2)
    // so tests can assert the order.
    enum class RenderStep : int {
        RenderWindow    = 0,
        RenderSprite    = 1,
        RenderComponent = 2,
    };
    using RenderStepCallback = void(*)(int mode, RenderStep step, void* user);
    void SetRenderCallbacksForTest(RenderStepCallback renderCb,
                                   void* user) noexcept {
        m_renderCb   = renderCb;
        m_renderUser = user;
    }

    // ----- Test hooks -----

    // 1:1 with legacy m_KeyImage[i].LoadSprite path.
    const std::string& GetImagePathForTest(int slot) const noexcept {
        return m_imagePaths[slot];
    }

    // Has the host wired a render callback?
    bool HasRenderCallbackForTest() const noexcept {
        return m_renderCb != nullptr;
    }

private:
    // 1:1 with legacy m_KeyImage[2] (2 cImageSelf slots).
    // Modern port stores resource paths as std::string;
    // the cImageSelf framework is R-12.x deferred.
    std::string m_imagePaths[kNumImages];

    // 1:1 with legacy m_wMode. Default = 2 (hidden).
    std::uint16_t m_wMode = kModeHidden;

    // Host-injected callbacks (R-12.x deferred).
    LoadSpriteCallback     m_loadSpriteCb    = nullptr;
    void*                  m_loadSpriteUser  = nullptr;
    SetImageSrcRectCallback m_setSrcRectCb   = nullptr;
    void*                  m_setSrcRectUser  = nullptr;
    RenderStepCallback     m_renderCb        = nullptr;
    void*                  m_renderUser      = nullptr;
};

}  // namespace mxh::ui