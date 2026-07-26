// chelperspeechdlg.hpp — modern port of 墨香 cHelperSpeechDlg.
//
// 1:1 port of legacy `cHelperSpeechDlg` from
//   `墨香【源码】\[Client]MH\HelperSpeechDlg.h` (no .cpp --
//   inline in the header in the legacy code).
//
// The helper speech dialog is a paged help-text overlay shown
// to the player (balloon outline + page-by-page text).  The
// legacy uses a cPtrList<DWORD> for the page id queue; the
// modern port uses std::vector<DWORD>.  The legacy's
// cPageBase / cBalloonOutline / cListItem dependencies are
// stubbed via host-injected callbacks (the modern cBalloon
// / cPageBase port is deferred).
//
// The 1:1 surface kept:
//   * Init(nx, ny, nwid, nhei, nlinehei, ID) -- 1:1 with legacy
//   * Render() / Linking() / ActionEvent()
//   * OpenDialog(DWORD dwPageId) -- dequeue + start fade-in
//   * CloseDialog() / ResetDialog() / Exit()
//   * AddPage(DWORD) / UseComponent(BOOL)
//   * SetHelperPos / SetTextRect
//   * StartFadeIn / StartFadeOut(DWORD dwNextIdx)
//   * GetCurPageIdx / IsListEmpty
//   * m_bFadeIn / m_bFadeOut / m_bClose flags
//   * m_dwStartTime / m_dwCurTime timestamps

#pragma once

#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cwindow.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace mxh::ui {

// 1:1 with the legacy cPageBase (墨香【源码】/[Client]MH/PageBase.h).
// The legacy cPageBase is a heavyweight paged-text widget; the
// modern port just keeps a stub class so unit tests can derive
// their own shims and the dialog can hold a cPageBase* pointer.
class cPageBase {};

// 1:1 with legacy cHelperSpeechDlg API.  The legacy
// `Init` signature is (int nx, int ny, int nwid, int nhei,
// int nlinehei, LONG ID) -- the modern port matches.
struct HelperSpeechLayout {
    std::int32_t nx       = 0;
    std::int32_t ny       = 0;
    std::int32_t nwid     = 0;
    std::int32_t nhei     = 0;
    std::int32_t nlinehei = 0;
    std::int32_t ID       = 0;
};

class cHelperSpeechDlg : public cDialog {
public:
    cHelperSpeechDlg();
    ~cHelperSpeechDlg() override;

    cHelperSpeechDlg(const cHelperSpeechDlg&) = delete;
    cHelperSpeechDlg& operator=(const cHelperSpeechDlg&) = delete;

    // 1:1 with legacy Init.  Stores the layout + clears the
    // page queue + resets the fade flags.
    void Init(int nx, int ny, int nwid, int nhei, int nlinehei, long ID);

    // 1:1 with legacy ActionEvent.
    std::uint32_t ActionEvent(/*CMouse**/ void* mouseInfo);

    // 1:1 with legacy Render.
    void Render();

    // 1:1 with legacy RenderComponent.  The modern port is
    // a no-op (the balloon outline + cPageBase render is
    // deferred).
    void RenderComponent() {}

    // 1:1 with legacy Linking.  Wires any cWindow children.
    void Linking();

    // 1:1 with legacy OpenDialog(DWORD dwPageId).  If a
    // dialog is already open (m_pCurPage != nullptr), the
    // page id is queued; otherwise the dialog starts
    // showing it immediately.
    bool OpenDialog(std::uint32_t dwPageId);

    // 1:1 with legacy CloseDialog.  Hides the dialog + clears
    // the page queue + flips m_bClose.
    void CloseDialog();

    // 1:1 with legacy ResetDialog.  Resets the dialog to a
    // fresh state.
    void ResetDialog();

    // 1:1 with legacy Exit.  Private in the legacy; modern
    // port exposes it as protected.
    void Exit();

    // 1:1 with legacy AddPage(DWORD).
    void AddPage(std::uint32_t dwPageId);

    // 1:1 with legacy UseComponent(BOOL).
    void UseComponent(bool bUse) noexcept { m_bUseComponent = bUse; }

    // 1:1 with legacy SetHelperPos(float, float).
    void SetHelperPos(float x, float y) noexcept {
        m_vHelperPosX = x; m_vHelperPosY = y;
    }

    // 1:1 with legacy SetTextRect(RECT*).  The legacy uses
    // Win32 RECT (LONG top/left/right/bottom); the modern
    // port takes 4 ints.
    void SetTextRect(int left, int top, int right, int bottom) noexcept {
        m_textRelLeft   = left;
        m_textRelTop    = top;
        m_textRelRight  = right;
        m_textRelBottom = bottom;
    }

    // 1:1 with legacy StartFadeOut(DWORD dwNextIdx).  Returns
    // TRUE on success (legacy returns BOOL).
    bool StartFadeOut(std::uint32_t dwNextIdx);

    // 1:1 with legacy StartFadeIn.
    void StartFadeIn();

    // 1:1 with legacy GetCurPageIdx.
    std::uint32_t GetCurPageIdx() const noexcept;

    // 1:1 with legacy IsListEmpty.
    bool IsListEmpty() const noexcept { return m_NextPagelist.empty(); }

    // Test introspection.
    bool                isFadeIn()         const noexcept { return m_bFadeIn; }
    bool                isFadeOut()        const noexcept { return m_bFadeOut; }
    bool                isClose()          const noexcept { return m_bClose; }
    bool                isUseComponent()   const noexcept { return m_bUseComponent; }
    std::uint32_t       startTime()        const noexcept { return m_dwStartTime; }
    std::uint32_t       curTime()          const noexcept { return m_dwCurTime; }
    int                 lineHeight()       const noexcept { return m_nLineHeight; }
    int                 lineNum()          const noexcept { return m_nLineNum; }
    float               helperPosX()       const noexcept { return m_vHelperPosX; }
    float               helperPosY()       const noexcept { return m_vHelperPosY; }
    bool                hasCurPage()       const noexcept { return m_hasCurPage; }
    int                 queuedPageCount()  const noexcept { return static_cast<int>(m_NextPagelist.size()); }

    // Test hook -- override the current time (legacy gCurTime).
    void SetNowForTest(std::uint32_t now) noexcept { m_nowForTest = now; }

    // Test hook -- inject a "create a cPageBase for the page
    // id" callback (legacy: GAMERESRCMNGR->GetHelperPage(id)).
    // The modern port returns a placeholder cPageBase*.
    using CreatePageCallback = std::function<cPageBase*(std::uint32_t pageId)>;
    void SetCreatePageCallbackForTest(CreatePageCallback cb) {
        m_createPageCb = std::move(cb);
    }

    // Test hook -- inject a "destroy a cPageBase" callback
    // (legacy: delete m_pCurPage).
    using DestroyPageCallback = std::function<void(cPageBase* page)>;
    void SetDestroyPageCallbackForTest(DestroyPageCallback cb) {
        m_destroyPageCb = std::move(cb);
    }

    // Test hook -- inject a "page action event" callback
    // (legacy: m_pCurPage->ActionEvent(mouseInfo)).
    using PageActionCallback = std::function<std::uint32_t(cPageBase* page, void* mouseInfo)>;
    void SetPageActionCallbackForTest(PageActionCallback cb) {
        m_pageActionCb = std::move(cb);
    }

    // Test hook -- inject a "page render" callback (legacy:
    // m_pCurPage->Render()).
    using PageRenderCallback = std::function<void(cPageBase* page)>;
    void SetPageRenderCallbackForTest(PageRenderCallback cb) {
        m_pageRenderCb = std::move(cb);
    }

private:
    // 1:1 with legacy m_bUseComponent.
    bool               m_bUseComponent = false;
    // 1:1 with legacy m_textRelRect (RECT in legacy = 4 LONGs).
    int                m_textRelLeft   = 0;
    int                m_textRelTop    = 0;
    int                m_textRelRight  = 0;
    int                m_textRelBottom = 0;
    int                m_nLineHeight   = 0;
    int                m_nLineNum      = 0;
    // 1:1 with legacy m_pCurPage (forward-declared cPageBase).
    cPageBase*         m_pCurPage      = nullptr;
    bool               m_hasCurPage    = false;
    // 1:1 with legacy m_vHelperPos (VECTOR2 = 2 floats).
    float              m_vHelperPosX   = 0.0f;
    float              m_vHelperPosY   = 0.0f;
    bool               m_bFadeIn       = false;
    bool               m_bFadeOut      = false;
    bool               m_bClose        = false;
    std::uint32_t      m_dwStartTime   = 0;
    std::uint32_t      m_dwCurTime     = 0;
    // 1:1 with legacy m_NextPagelist (cPtrList<DWORD>).
    std::vector<std::uint32_t> m_NextPagelist;

    std::uint32_t      m_nowForTest    = 0;
    std::uint32_t      m_curPageIdx     = 0;

    CreatePageCallback m_createPageCb;
    DestroyPageCallback m_destroyPageCb;
    PageActionCallback m_pageActionCb;
    PageRenderCallback m_pageRenderCb;
};

}  // namespace mxh::ui
