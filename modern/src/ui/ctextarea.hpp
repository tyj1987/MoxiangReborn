// ctextarea.hpp — modern port of 墨香 cTextArea (multi-line
// text area with scrollbar, caret, and IME support).
//
// 1:1 port of legacy `cTextArea` from
//   `墨香【源码】\[Client]MH\interface\cTextArea.h` (2055 B).
//
// Phase 12.x minimal port: only the data model + the
// most-used public methods are implemented. The complex
// scroll state + IME + actual render are deferred
// (Phase 12.x + 6.13). This is the minimum needed to
// unlock the ~30 dialogs that depend on cTextArea
// (BailDialog / ChaseDialog / EventNotifyDialog /
// GuildCreateDialog / GuildInviteDialog / GuildMarkDialog
// / GuildNickNameDialog / GuildFieldWarDialog /
// AutoAnswerDlg / AutoNoteDlg / ChinaAdviceDlg / cMsgBox /
// MPNoticeDialog / etc.).
//
// The methods listed here are the ones the legacy
// callers actually use. Other methods (GetCaretPos /
// SetEnterAllow / SetCaretMoveFirst / OnUpwardItem /
// OnDownwardItem / etc.) are documented in the header
// as Phase 12.x deferred and will be added in a
// follow-up commit when the dialogs that use them are
// ported.

#pragma once

#include "cdialog.hpp"

#include <cstdint>
#include <string>

namespace mxh::ui {

// Forward-declare RECT (it lives in the Windows SDK
// headers; if those aren't included we use a minimal
// local definition).
struct TextRect {
    std::int32_t left   = 0;
    std::int32_t top    = 0;
    std::int32_t right  = 0;
    std::int32_t bottom = 0;
};

class cTextArea : public cDialog {
public:
    cTextArea();
    ~cTextArea() override;

    // ----- 1:1 with legacy cTextArea::InitTextArea -----

    // 1:1 quirk: legacy has 2 InitTextArea overloads.
    //   (1) Full: 3 chrome images + 3 heights + text
    //       rect + buffer size.
    //   (2) Simple: text rect + buffer size only.
    // Modern port implements both. Chrome image
    // pointers are void* (1:1 with the cImage opaque
    // pointer pattern).
    void InitTextArea(const TextRect& textRelRect, int bufSize,
                      void* topImage, std::uint16_t topHeight,
                      void* middleImage, std::uint16_t middleHeight,
                      void* downImage, std::uint16_t downHeight);
    void InitTextArea(const TextRect& textRelRect, int bufSize);

    // ----- 1:1 with legacy cTextArea::SetActive -----

    // 1:1 override: cTextArea::SetActive toggles the
    // caret visibility (legacy: SetActive(false) hides
    // the caret). The modern port stores the intent —
    // actual caret rendering is Phase 6.13+ deferred.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy cTextArea::SetFocusEdit / SetFocus -----

    void SetFocusEdit(bool val) noexcept;
    void SetFocus(bool val) noexcept            { SetFocusEdit(val); }

    // ----- 1:1 with legacy cTextArea::GetScriptText / SetScriptText -----

    // 1:1 quirk: legacy uses char* buffers (with
    // outText pre-allocated by caller). Modern uses
    // std::string. The legacy c-string API is
    // available via GetScriptTextCString.
    void SetScriptText(const char* inText);
    const std::string& GetScriptText() const noexcept { return m_scriptText; }
    void GetScriptTextCString(char* outText, int bufSize) const;

    // ----- 1:1 with legacy cTextArea::SetReadOnly -----

    void SetReadOnly(bool val) noexcept          { m_bReadOnly = val; }
    bool IsReadOnly() const noexcept            { return m_bReadOnly; }

    // ----- 1:1 with legacy cTextArea::SetEnterAllow -----

    // 1:1 quirk: legacy cTextArea::SetEnterAllow
    // toggles whether the user can press Enter to
    // insert a newline (FALSE disables Enter for
    // single-line notice / input dialogs). Modern
    // port stores the boolean. Actual Enter handling
    // is Phase 12.x deferred (the keyboard callback
    // is wired in cTextArea's full render path).
    void SetEnterAllow(bool val) noexcept        { m_bEnterAllow = val; }
    bool IsEnterAllow() const noexcept          { return m_bEnterAllow; }

    // ----- 1:1 with legacy cTextArea::SetLimitLine -----

    bool SetLimitLine(int nMaxLine) noexcept;

    // ----- 1:1 with legacy cTextArea::SetTextColor -----

    void SetTextColor(std::uint32_t dwColor) noexcept { m_dwTextColor = dwColor; }
    std::uint32_t GetTextColor() const noexcept      { return m_dwTextColor; }

    // ----- 1:1 with legacy cTextArea::Add -----

    // 1:1 quirk: legacy cTextArea::Add overrides
    // cDialog to add a child window. Modern port
    // delegates to cDialog::Add (1:1 with legacy
    // behavior since the legacy override just calls
    // cDialog::Add).
    void Add(cWindow* window);

    // ----- 1:1 with legacy cTextArea::Render (placeholder) -----

    // 1:1 quirk: legacy cTextArea::Render draws the
    // 3-row chrome + text + scrollbar. Modern port
    // is a no-op (render path lands in Phase 6.13+).
    void Render() override {}

private:
    // Legacy fields (data model only — actual scroll /
    // caret / IME logic is Phase 12.x deferred).
    int            m_nTopLineIdx  = 0;
    int            m_nLineNum     = 0;
    int            m_nLineHeight  = 14;     // 1:1 default
    int            m_nMaxLine     = 0;      // SetLimitLine target
    TextRect       m_rcTextRelRect{};

    bool           m_bReadOnly    = false;
    bool           m_bCaret       = false;
    bool           m_bEnterAllow  = true;     // 1:1 default (legacy ctor sets TRUE)

    // 1:1 quirk: legacy stores 3 cImage objects.
    // Modern port stores them as void* (1:1 with the
    // opaque-pointer pattern used by cButton /
    // cIconDialog).
    void*          m_TopImage     = nullptr;
    std::uint16_t  m_topHeight    = 0;
    void*          m_MiddleImage  = nullptr;
    std::uint16_t  m_middleHeight = 0;
    void*          m_DownImage    = nullptr;
    std::uint16_t  m_downHeight   = 0;

    std::uint32_t  m_dwTextColor  = 0xFF000000;

    std::string    m_scriptText;
};

}  // namespace mxh::ui
