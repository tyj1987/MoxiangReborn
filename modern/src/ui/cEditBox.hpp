// mxh/ui/cEditBox.hpp
// Phase 6.2 — modern C++ cEditBox widget. Single-line text input. Builds
// on cWindow (6.0) and follows the same 1:1 legacy contract as cButton.
//
// Scope (this phase):
//   - text buffer (UTF-8) with fixed-byte capacity
//   - caret position (insert / overwrite mode)
//   - key handling: Char / Backspace / Delete / Left / Right / Home / End
//     / Enter (submits) / Escape (cancel)
//   - read-only mode (chars rejected, caret optionally shown)
//   - secret mode (display as bullets)
//   - text-changed callback (modern: std::function)
//   - 2 state images: basicImage / focusImage
//   - SetFocus / focus image swap
//
// Deferred (later phases):
//   - IME / cIMEex integration (the legacy engine has a full Korean/JP
//     IME with composition window + candidate list; that's its own module
//     and warrants a dedicated 6.x phase)
//   - clipboard paste (Ctrl+V)
//   - multi-line + scroll
//   - input validation (numeric-only, etc.)
//   - undo / redo
//
// All of the above are noted in the legacy cEditBox.h but the most common
// call site in the game is a single-line username / chat input, which is
// exactly what this skeleton handles. IME is the next obvious addition.
#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "cWindow.hpp"

namespace mxh::ui {

class cEditBox : public cWindow {
public:
    // Key codes we interpret. Mirrors the legacy engine's VK_* constants
    // for the subset we care about; production code in 6.x will fold in
    // a proper keyboard adapter (currently the framework just takes raw
    // key codes via ActionKeyboardEvent).
    enum class Key : std::int32_t {
        None    = 0,
        Back    = 8,    // VK_BACK
        Tab     = 9,    // VK_TAB
        Enter   = 13,   // VK_RETURN
        Escape  = 27,   // VK_ESCAPE
        Left    = 37,   // VK_LEFT
        Up      = 38,
        Right   = 39,
        Down    = 40,
        Delete  = 46,   // VK_DELETE
        Home    = 36,
        End     = 35,
    };

    // Notification callbacks (modern: std::function).
    //   onChange fires on every text mutation (insertion / deletion).
    //   onEnter  fires when the user presses Enter while focused.
    //   onEscape fires when the user presses Escape while focused.
    using TextCallback = std::function<void(cEditBox& self, void* userdata)>;

    cEditBox() = default;
    ~cEditBox() override = default;

    cEditBox(const cEditBox&) = delete;
    cEditBox& operator=(const cEditBox&) = delete;

    // Init: position, size, two state images (basic / focus), id.
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
              std::uint16_t hei, void* basicImage, void* focusImage,
              std::int32_t id = 0);

    // InitEditbox: configure the text buffer. `pixelWidth` is informational
    // (the legacy engine used it to clip overflow); `bufBytes` is the
    // maximum storage including the trailing NUL. Must be called before
    // the editbox is used; calling it again resets the buffer.
    void InitEditbox(std::uint16_t /*pixelWidth*/, std::uint16_t bufBytes);

    // Render placeholder (real GPU draw in 6.3).
    void Render() override {}

    // Mouse event: clicking inside the editbox focuses it (focusImage
    // shown); clicking outside blurs it.
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // Keyboard event: feed one keystroke at a time. `key` is the VK_*
    // code (or any of the Key enum values); `ch` is the character payload
    // (0 for non-character keys). The action is applied to the buffer.
    std::uint32_t ActionKeyboardEvent(std::int32_t key, std::int32_t ch) override;

    // -------------------------------------------------------------------------
    // Text access. GetEditText() returns the raw UTF-8 buffer; SetEditText
    // replaces it (caret reset to end). displayText() returns either the
    // raw text or bullets if secret mode is on.
    // -------------------------------------------------------------------------
    const std::string& editText() const noexcept { return m_text; }
    std::string displayText() const;

    void SetEditText(std::string text);

    // Read-only mode: characters / Backspace / Delete all no-op; caret
    // optionally still shown (legacy m_bShowCaretInReadOnly).
    void SetReadOnly(bool v) noexcept        { m_bReadOnly = v; }
    bool IsReadOnly() const noexcept        { return m_bReadOnly; }
    void ShowCaretInReadOnly(bool v) noexcept { m_bShowCaretInReadOnly = v; }
    bool showCaretInReadOnly() const noexcept { return m_bShowCaretInReadOnly; }

    // Secret (password) mode: displayText() returns bullets of the same
    // length as the actual text.
    void SetSecret(bool v) noexcept          { m_bSecret = v; }
    bool IsSecret() const noexcept           { return m_bSecret; }

    // Caret visibility (blinking caret on/off — actual blink timing is a
    // render concern, not a state concern).
    void SetCaret(bool v) noexcept           { m_bCaret = v; }
    bool HasCaret() const noexcept           { return m_bCaret; }

    // Focus image swap mirrors the legacy cEditBox::SetFocusEdit path:
    // focused => m_basicImage (parent's cImage), blurred => m_focusImage.
    // We keep the focus image pointer around and the rendering layer
    // (6.3) decides which one to draw based on hasFocus().
    void SetFocusEdit(bool v) noexcept       { SetFocus(v); }
    void* focusImage() const noexcept        { return m_focusImage; }

    // SetActive / SetDisable: legacy API. SetActive toggles caret blink
    // (in legacy); we map it to SetCaret. SetDisable routes through
    // SetEnabled (blurs the editbox + blocks input).
    void SetActive(bool v) noexcept          { SetCaret(v); }
    void SetDisable(bool v) noexcept         { SetEnabled(!v); if (v) SetFocus(false); }

    // Caret position.
    std::size_t caretPos() const noexcept    { return m_caret; }
    void SetCaretPos(std::size_t pos) noexcept;

    // Insert / overwrite mode.
    void SetInsertMode(bool v) noexcept      { m_bInsert = v; }
    bool insertMode() const noexcept         { return m_bInsert; }

    // Text colors (legacy: m_activeTextColor / m_nonactiveTextColor).
    void SetActiveTextColor(std::uint32_t c) noexcept    { m_activeTextColor = c; }
    void SetNonactiveTextColor(std::uint32_t c) noexcept { m_nonactiveTextColor = c; }
    std::uint32_t activeTextColor() const noexcept    { return m_activeTextColor; }
    std::uint32_t nonactiveTextColor() const noexcept { return m_nonactiveTextColor; }

    // Text offsets (legacy: SetTextOffset).
    void SetTextOffset(std::int32_t left, std::int32_t right,
                       std::int32_t top) noexcept;
    std::int32_t textLeftOffset() const noexcept   { return m_textLeftOffset; }
    std::int32_t textRightOffset() const noexcept  { return m_textRightOffset; }
    std::int32_t textTopOffset() const noexcept    { return m_textTopOffset; }

    // Text alignment (legacy: SetAlign, 0=left, 1=center, 2=right).
    enum class TextAlign : std::int32_t { Left = 0, Center = 1, Right = 2 };
    void SetAlign(TextAlign a) noexcept      { m_align = a; }
    TextAlign textAlign() const noexcept     { return m_align; }

    // Change callback.
    void SetEditFunc(TextCallback cb)        { m_onChange = std::move(cb); }
    void SetEnterFunc(TextCallback cb)       { m_onEnter  = std::move(cb); }
    void SetEscapeFunc(TextCallback cb)      { m_onEscape = std::move(cb); }
    void SetUserdata(void* u)                { m_userdata = u; }

    // Validity check method (legacy SetValidCheck). 0 = none, 1 = digits only,
    // 2 = alpha only, 3 = alnum. The check is applied in HandleChar().
    void SetValidCheck(int m) noexcept        { m_validCheck = m; }
    int  GetValidCheckMethod() const noexcept { return m_validCheck; }

    // Test accessors.
    std::size_t maxBytes() const noexcept    { return m_maxBytes; }
    bool        textChanged() const noexcept  { return m_bTextChanged != 0; }
    void        clearTextChanged() noexcept   { m_bTextChanged = 0; }

private:
    void* m_basicImage = nullptr;
    void* m_focusImage = nullptr;

    std::string m_text;          // UTF-8 text (current content)
    std::size_t m_caret   = 0;   // byte offset in m_text
    std::size_t m_maxBytes = 0;  // 0 = not yet InitEditbox'd
    bool        m_bInsert = true;

    // Style / colors.
    std::uint32_t m_activeTextColor    = 0xFF000000;
    std::uint32_t m_nonactiveTextColor = 0xFF808080;
    std::int32_t  m_textLeftOffset     = 2;
    std::int32_t  m_textRightOffset    = 2;
    std::int32_t  m_textTopOffset      = 0;
    TextAlign     m_align              = TextAlign::Left;
    bool          m_bCaret             = false;
    bool          m_bSecret            = false;
    bool          m_bReadOnly          = false;
    bool          m_bShowCaretInReadOnly = false;

    // Legacy m_bTextChanged is LONG (0/1/2); we just need 0 vs nonzero.
    std::int32_t  m_bTextChanged = 0;
    int           m_validCheck   = 0;  // 0 = none

    // Callbacks.
    TextCallback  m_onChange;
    TextCallback  m_onEnter;
    TextCallback  m_onEscape;
    void*         m_userdata = nullptr;

    // Helpers.
    void insertCharAtCaret(char c);
    void deleteAtCaret();        // backspace
    void deleteForwardAtCaret();  // delete
    bool charAllowed(char c) const noexcept;
    void fireChange();
};

} // namespace mxh::ui
