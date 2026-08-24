// skilloptioncleardlg.hpp — modern port of 墨香 CSkillOptionClearDlg.
//
// 1:1 port of legacy `CSkillOptionClearDlg` from
//   `墨香【源码】\[Client]MH\SkillOptionClearDlg.{h,cpp}`.
//
// CSkillOptionClearDlg is a "skill option clear" dialog: the
// player drops a mugong/jinbub icon into a 1-cell icon dialog,
// then clicks OK to send MP_MUGONG_OPTION_CLEAR_SYN to the
// server. The dialog is a cIconDialog subclass (so the dialog
// itself owns 1 icon cell by default) and additionally resolves
// one inner cIconDialog (m_pMugongIconDlg) at Linking() by id.
//
// 1:1 contract preserved:
//   - Linking() resolves m_pMugongIconDlg by id (T_DefaultICON
//     in legacy). Modern port uses kMugongIconId constant.
//   - FakeMoveIcon(mouseX, mouseY, icon) — checks the dropped
//     icon's type against WT_MUGONG / WT_JINBUB, then against
//     the mugong's option (eSkillOption_None → reject with
//     WINDOWMGR msgbox + return FALSE), else replaces the icon
//     at cell 0. Modern port: type / option checks stubbed
//     (CMugongBase not ported, WINDOWMGR/CHATMGR not ported).
//   - OnActionEvent(lId, p, we) — branches on T_DefaultOKBTN
//     (WINDOWMGR confirm msgbox stubbed) and T_DefaultCANCERBTN
//     (closes dialog, ITEMMGR stubbed to release item lock).
//     Modern port: all 5 singletons stubbed.
//   - SetActive(BOOL val) override — on val==FALSE deletes the
//     mugong icon at cell 0 (matching legacy "dialog close
//     clears the slot"). Then calls cDialog::SetActive(val).
//   - SetItem(CItem* pItem) — stores the item's position.
//   - OptionClearSyn() — sends MSG_WORD4 to NETWORK. Modern
//     port: NETWORK stubbed.
//
// 1:1 quirks preserved:
//   - 1:1 quirk: legacy has a typo `OnActionEvnet` (missing 't')
//     in the cpp. Modern port uses the correctly-spelled
//     `OnActionEvent` (cGuildNoticeDlg/cUnionNoteDlg follow the
//     same correction pattern).
//   - 1:1 quirk: legacy `m_ClearOption` is declared in the
//     header but never used in the cpp. Modern port omits
//     this field (no 1:1 fidelity gained by including a dead
//     field — same pattern as cTitanGuageDlg::m_pMpPercent).
//   - 1:1 quirk: legacy ctor does not explicitly initialise
//     m_ItemPos (it is a WORD). The ctor's only effect is
//     implicit member init. Modern port preserves this 1:1
//     quirk (m_ItemPos is default-initialised in the header
//     to 0).
//   - 1:1 quirk: legacy `cIcon* temp;` in FakeMoveIcon is
//     declared but never used after the DeleteIcon call.
//     Modern port omits the unused local (same pattern as
//     cMPRegistDialog::AddLink / cMPRegistDialog::SetActive).
//   - 1:1 quirk: legacy `if (!(icon->GetType() == WT_MUGONG ||
//     icon->GetType() == WT_JINBUB))` evaluates the negation
//     of the type check. Modern port preserves the same
//     control flow (early return FALSE on invalid type).
//   - 1:1 quirk: legacy cIconDialog::GetType() returns the
//     cWindow::m_type field (WT_MUGONG / WT_JINBUB). Modern
//     cWindow has no m_type field (Phase 6 removed it).
//     Modern port uses cIcon::GetType() which returns 0 by
//     default. The check is therefore effectively "always
//     accept" in the modern port; real CMugongBase port is
//     deferred.
//   - 1:1 quirk: legacy ctor `CSkillOptionClearDlg(void)` +
//     dtor `~CSkillOptionClearDlg(void)` have no body. Modern
//     port preserves the empty bodies (with `= default`).
//   - 1:1 quirk: legacy `FakeMoveIcon` returns FALSE even on
//     the success path (after the icon is added). Modern port
//     preserves this verbatim — the slot is filled, but the
//     function always returns FALSE (matching legacy's
//     "return FALSE" at the very end).
//   - 1:1 quirk: legacy `OnActionEvent` checks `we & WE_BTNCLICK`
//     (legacy == 64). Modern port uses `we == WindowEvent::
//     LButtonClick` (modern == 4) — different bit position;
//     1:1 quirks cPetStateMiniDlg/CharStateDialog/etc. follow
//     the same pattern.
//   - 1:1 quirk: legacy `MSG_WORD4 msg; msg.Category = ...`
//     uses the [CC]Header/CommonStruct.h struct. Modern port
//     provides an inline `struct MsgWord4` with the same field
//     layout (Category / Protocol / dwObjectID / wData1..4).
//   - 1:1 quirk: legacy OptionClearSyn uses HEROID (object
//     manager macro) for `msg.dwObjectID`. Modern port uses
//     0u as a placeholder (HEROID/HERO are singletons).
//   - 1:1 quirk: legacy `OnActionEvent` default branch is
//     implicit (no `else` clause). Modern port: the switch
//     statement's default is also implicit (no `else` branch).
//   - 1:1 quirk: legacy T_DefaultICON / T_DefaultOKBTN /
//     T_DefaultCANCERBTN come from `WindowIDEnum.h`.
//     Modern port uses local kMugongIconId=2000 /
//     kOkBtnId=2001 / kCancelBtnId=2002 to avoid coupling
//     to the legacy header.
//   - 1:1 quirk: legacy MBI_SKILLOPTIONCLEAR_NACK (1338) /
//     MBI_SKILLOPTIONCLEAR_ACK (1339) / MBT_OK / MBT_YESNO
//     are unused in the modern port (WINDOWMGR stubbed). The
//     placeholder strings are documented in the .cpp.
//   - 1:1 quirk: legacy eSkillOption_None is unused in the
//     modern port (CMugongBase not ported). The early-return
//     guard is preserved as a comment.
//   - 1:1 quirk: SetActive override must be `noexcept` to
//     match the virtual spec (R-12 fix).

#pragma once

#include "cIconDialog.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

class cIconDialog;
class cIcon;

// 1:1 stub: legacy `MSG_WORD4` struct from [CC]Header/
// CommonStruct.h. Modern port provides a minimal inline
// definition with the same field layout (Category / Protocol
// / dwObjectID / wData1..4) so OptionClearSyn can be ported
// 1:1. Production code can replace this stub by linking a
// real CommonStruct.h from the legacy tree.
struct MsgWord4 {
    std::uint8_t  Category   = 0;
    std::uint8_t  Protocol   = 0;
    std::uint32_t dwObjectID = 0;
    std::uint16_t wData1     = 0;
    std::uint16_t wData2     = 0;
    std::uint16_t wData3     = 0;
    std::uint16_t wData4     = 0;
};

// 1:1 stub: legacy `CItem` (defined in [Client]MH/Item.h,
// part of the 2008-era client). Modern port uses an opaque
// forward-declared class with the only API surface the
// SkillOptionClearDlg needs (GetPosition). Production code
// can replace this stub by linking a real Item.h.
class CItem {
public:
    CItem() = default;
    explicit CItem(std::uint16_t pos) noexcept : m_pos(pos) {}
    std::uint16_t GetPosition() const noexcept { return m_pos; }
private:
    std::uint16_t m_pos = 0;
};

// 1:1 stub: legacy `CMugongBase` (defined in [Client]MH/
// MugongBase.h, part of the 2008-era client). Modern port
// uses an opaque forward-declared class with the only API
// surface the SkillOptionClearDlg needs (GetOption /
// GetItemIdx / GetPosition). Production code can replace
// this stub by linking a real MugongBase.h.
class CMugongBase {
public:
    CMugongBase() = default;
    std::int32_t  GetOption()   const noexcept { return m_option; }
    std::uint16_t GetItemIdx()  const noexcept { return m_itemIdx; }
    std::uint16_t GetPosition() const noexcept { return m_position; }
    // Test-injectable setters (production code wires via cIcon cast).
    void SetOption(std::int32_t opt)   noexcept { m_option = opt; }
    void SetItemIdx(std::uint16_t idx) noexcept { m_itemIdx = idx; }
    void SetPosition(std::uint16_t pos) noexcept { m_position = pos; }
private:
    std::int32_t  m_option   = 0;
    std::uint16_t m_itemIdx  = 0;
    std::uint16_t m_position = 0;
};

class cSkillOptionClearDlg : public cIconDialog {
public:
    // 1:1 with legacy T_DefaultICON / T_DefaultOKBTN / T_DefaultCANCERBTN
    // (from legacy WindowIDEnum.h). Modern port uses a local id range
    // to avoid coupling to the legacy header.
    static constexpr int kMugongIconId = 2000;
    static constexpr int kOkBtnId      = 2001;
    static constexpr int kCancelBtnId  = 2002;

    // 1:1 with legacy protocol ids (from legacy [CC]Header/Protocol.h).
    static constexpr std::uint8_t kCategoryMpMugong        = 0x4A; // MP_MUGONG
    static constexpr std::uint8_t kProtocolOptionClearSyn  = 0x4B; // MP_MUGONG_OPTION_CLEAR_SYN

    // 1:1 with legacy chat message ids (from legacy ChatManager.h).
    static constexpr int kChatMsgNoOption   = 1338; // MBI_SKILLOPTIONCLEAR_NACK
    static constexpr int kChatMsgConfirmAsk = 1339; // MBI_SKILLOPTIONCLEAR_ACK

    cSkillOptionClearDlg();
    ~cSkillOptionClearDlg() override;

    cSkillOptionClearDlg(const cSkillOptionClearDlg&) = delete;
    cSkillOptionClearDlg& operator=(const cSkillOptionClearDlg&) = delete;

    void Linking();
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy surface.
    bool FakeMoveIcon(std::int32_t mouseX, std::int32_t mouseY, cIcon* icon);
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
    void SetItem(CItem* pItem);
    void OptionClearSyn();

    // Test accessors.
    cIconDialog* mugongIcon() const noexcept { return m_pMugongIconDlg; }
    std::uint16_t itemPos() const noexcept { return m_ItemPos; }
    bool lastFakeMoveResult() const noexcept { return m_lastFakeMoveResult; }

    // Test-injectable singletons (per Phase 6 stub pattern). Production
    // code would link a real NETWORK / ITEMMGR / WINDOWMGR / CHATMGR /
    // OBJECTMGR by defining real symbols before the dialog lib.
    static void SetMugongForTesting(CMugongBase* m) noexcept { s_mugong = m; }
    static void SetItemForTesting(std::uint16_t pos) noexcept { s_itemPos = pos; }
    static void ClearTestInjections() noexcept {
        s_mugong = nullptr;
        s_itemPos = 0xFFFFu;
    }
    static const MsgWord4& lastSentMessage() noexcept { return s_lastSentMsg; }
    static void ClearLastSentMessage() noexcept { s_lastSentMsg = MsgWord4{}; }
    static std::uint32_t fakeMoveIconCallCount() noexcept { return s_fakeMoveIconCalls; }
    static std::uint32_t optionClearSynCallCount() noexcept { return s_optionClearSynCalls; }
    static std::uint32_t onActionEventCallCount() noexcept { return s_onActionEventCalls; }
    static void resetCallCounts() noexcept {
        s_fakeMoveIconCalls = 0;
        s_optionClearSynCalls = 0;
        s_onActionEventCalls = 0;
    }

private:
    cIconDialog* m_pMugongIconDlg = nullptr;
    std::uint16_t m_ItemPos = 0;
    bool m_lastFakeMoveResult = false;

    // Test-injectable singletons (replace the legacy globals: HERO,
    // ITEMMGR, WINDOWMGR, CHATMGR, NETWORK).
    static inline CMugongBase* s_mugong = nullptr;
    static inline std::uint16_t s_itemPos = 0xFFFFu; // "no item" sentinel
    static inline MsgWord4 s_lastSentMsg{};
    static inline std::uint32_t s_fakeMoveIconCalls = 0;
    static inline std::uint32_t s_optionClearSynCalls = 0;
    static inline std::uint32_t s_onActionEventCalls = 0;
};

} // namespace mxh::ui
