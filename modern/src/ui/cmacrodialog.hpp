// cmacrodialog.hpp — modern port of 墨香 CMacroDialog (macro key
// bindings dialog).
//
// 1:1 port of legacy `CMacroDialog` from
//   `墨香【源码】\[Client]MH\MacroDialog.h` (1,427 B) and
//   `墨香【源码】\[Client]MH\MacroDialog.cpp` (13,211 B).
//
// MacroDialog is the keyboard-shortcut binding UI. It shows a list
// of cEditBox children (one per macro event), each displaying the
// current key combination for that event. The user clicks an
// edit box, presses a new key, and the binding updates.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this is the
// first **Tier 2** dialog port (after 5 base widgets + 2 dialogs +
// 3 subcontrols in 0.13.10–0.13.13). Tier 2 dialogs have local
// state but no service injection — the macro data is local UI
// state that the host app persists via OptionManager (not yet
// ported; deferred to a later phase).
//
// The modern port covers the data model + the dialog's
// SetActive / Linking / ConvertMacroToText methods. The full
// keyboard-capture / OptionManager integration is out of scope
// for this 1:1 port — the host app is expected to register a
// callback via `SetOnMacroChanged` to be notified when the user
// rebinds a key, and to persist the binding externally.
//
// 1:1 quirks (preserved from legacy):
//   - ConvertMacroToText produces "CTRL + KEY" style strings,
//     with MSK_NONE rendering as the bare key (no prefix).
//   - The mode (MM_CHAT / MM_MACRO) is held internally; the
//     modern port exposes SetMode() / Mode() but does not
//     implement the actual key-press routing (host does it).
//   - sMACRO struct uses int (legacy), std::uint16_t for wKey,
//     bool for bAllMode / bUp — same wire format as legacy.

#pragma once

#include "cDialog.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace mxh::ui {

class cEditBox;

// ===== 1:1 mirror of legacy MacroManager.h enums =========================

// Macro event IDs (subset of ME_* that the modern port supports.
// The full legacy ME_COUNT has ~30 entries; the modern port
// covers the 7 quick-slot bindings + 4 toggle-dialog bindings
// + 2 mode bindings + 1 minimap + 1 screencapture = 15 entries).
// Pinned so a future port that adds more events knows exactly
// which slots are reserved.
enum class MacroEvent : std::uint8_t {
    USE_QUICKITEM01     = 0,
    USE_QUICKITEM02     = 1,
    USE_QUICKITEM03     = 2,
    USE_QUICKITEM04     = 3,
    USE_QUICKITEM05     = 4,
    USE_QUICKITEM06     = 5,
    USE_QUICKITEM07     = 6,
    TOGGLE_INVENTORYDLG = 7,
    TOGGLE_CHARACTERDLG = 8,
    TOGGLE_MUGONGDLG    = 9,
    TOGGLE_QUESTDLG     = 10,
    TOGGLE_EXITDLG      = 11,
    TOGGLE_MOVEMODE     = 12,
    TOGGLE_PEACEWARMODE = 13,
    TOGGLE_AUTOATTACK   = 14,
    TOGGLE_MINIMAP      = 15,
    TOGGLE_CAMERAVIEWMODE = 16,
    SCREENCAPTURE       = 17,
    // The original legacy has more (PAGEUP/DN quick slot, help,
    // camera move/zoom, etc.). They are reserved below so the
    // array index is stable across the modern / legacy boundary.
    ME_COUNT            = 18,
};

// Macro mode (1:1 with legacy MM_* enum).
enum class MacroMode : std::uint8_t {
    Chat  = 0,   // MM_CHAT — chat takes priority over macro
    Macro = 1,   // MM_MACRO — macro key is processed first
};

// System key modifier (1:1 with legacy eSysKey).
enum class SysKey : std::uint8_t {
    None  = 1,   // MSK_NONE
    Ctrl  = 2,   // MSK_CTRL
    Alt   = 4,   // MSK_ALT
    Shift = 8,   // MSK_SHIFT
    All   = None | Ctrl | Alt | Shift,
};

// 1:1 with legacy sMACRO struct. Wire-format-compatible with the
// legacy engine (uses int for nSysKey / wKey for direct copy
// from/to a legacy CMacroManager::sMACRO).
struct sMACRO {
    int            nSysKey = static_cast<int>(SysKey::None);  // modifier
    std::uint16_t  wKey    = 0;                              // VK code
    bool           bAllMode = false;                         // applies in both Chat / Macro mode
    bool           bUp      = false;                         // fire on key-up
};

class cMacroDialog : public cDialog {
public:
    cMacroDialog();
    ~cMacroDialog() override;

    // ----- Init / lifecycle (1:1 with legacy CMacroDialog::Init) -----

    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid, std::uint16_t hei);

    // Override cDialog::SetActive. Legacy does the same (called
    // by the dispatcher when the user toggles the macro dialog).
    // Modern port mirrors the virtual SetActive pattern from
    // cExitDialog (R-12 fix); see cDialog.hpp for the rationale.
    void SetActive(bool val) noexcept override;

    // ----- Linking -----

    // 1:1 with legacy CMacroDialog::Linking. Looks up the 15
    // cEditBox children by id and stores the pointers for
    // later refresh. The caller is expected to have added the
    // children via cDialog::Add() before calling Linking().
    void Linking();

    // ----- Mode (1:1 with legacy SetMode) -----

    void SetMode(MacroMode mode) noexcept { m_nMode = static_cast<std::uint8_t>(mode); }
    MacroMode Mode() const noexcept        { return static_cast<MacroMode>(m_nMode); }

    // ----- Macro data (1:1 with legacy m_MacroKey[ME_COUNT]) -----

    // Set the binding for a macro event. Updates the internal
    // array AND, if the corresponding cEditBox is linked,
    // refreshes the displayed text.
    void SetMacroBinding(MacroEvent evt, const sMACRO& macro);

    // Read the current binding. Returns a default-constructed
    // sMACRO if the event is out of range.
    sMACRO GetMacroBinding(MacroEvent evt) const;

    // Mark the dialog as having unsaved changes (1:1 with
    // legacy m_bChanged). The host app queries this to decide
    // whether to show a "save changes?" prompt.
    bool IsChanged() const noexcept { return m_bChanged; }
    void ClearChanged() noexcept    { m_bChanged = false; }

    // ----- ConvertMacroToText (1:1) -----

    // Format a sMACRO as a "CTRL + F1" style string. The output
    // buffer is `text` (caller-owned, must be at least
    // kMaxMacroTextLen + 1 bytes). Returns the number of bytes
    // written (excluding the null terminator).
    static constexpr std::size_t kMaxMacroTextLen = 31;
    std::size_t ConvertMacroToText(char* text, const sMACRO& macro) const;

    // Convert a virtual-key code to a human-readable key name
    // (e.g. VK_F1 → "F1", 0x41 → "A"). Used by ConvertMacroToText.
    // Returns an empty string if the VK code is unknown.
    static std::string VKeyToName(std::uint16_t vk);

    // ----- Direct data access (for the host OptionManager bridge) -----

    const sMACRO& MacroAt(std::size_t i) const noexcept { return m_MacroKey.at(i); }
    void SetMacroAt(std::size_t i, const sMACRO& m) noexcept { m_MacroKey.at(i) = m; m_bChanged = true; }
    static constexpr std::size_t kMacroCount = static_cast<std::size_t>(MacroEvent::ME_COUNT);

private:
    // 1:1 with legacy m_MacroKey[ME_COUNT]. Array of macro
    // bindings indexed by MacroEvent.
    std::array<sMACRO, kMacroCount> m_MacroKey{};

    // 1:1 with legacy m_pMacroKeyEdit[ME_COUNT]. Pointer to
    // the cEditBox child for each macro event. Non-owning;
    // the cEditBox children are added to the dialog tree by
    // the host and Linking() resolves the pointers.
    std::array<cEditBox*, kMacroCount> m_pMacroKeyEdit{};

    // 1:1 with legacy m_nMode (0 = Chat, 1 = Macro).
    std::uint8_t m_nMode = static_cast<std::uint8_t>(MacroMode::Chat);

    // 1:1 with legacy m_bChanged.
    bool m_bChanged = false;

    // 1:1 with legacy m_bCombining (IME composition flag).
    // The modern port does not implement IME composition (see
    // modern/src/ui/ime.cpp for the separate IME adapter); the
    // field is reserved for future wiring.
    bool m_bCombining = false;

    // 1:1 with legacy m_pFocusEdit. Tracks which cEditBox
    // currently has keyboard focus. nullptr if no edit box is
    // focused.
    cEditBox* m_pFocusEdit = nullptr;

    // 1:1 with legacy m_nCurMacro. The index of the macro
    // currently being edited (for the per-edit-box focus
    // tracking). -1 if no edit in progress.
    int m_nCurMacro = -1;
};

}  // namespace mxh::ui
