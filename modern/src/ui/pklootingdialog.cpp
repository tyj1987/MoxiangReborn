// pklootingdialog.cpp — modern port implementation.
//
// 1:1 port of legacy `CPKLootingDialog` from
//   `墨香【源码】\[Client]MH\PKLootingDialog.cpp`.
//
// Modern-port notes
// =================
//
// 1. **Engine singleton dependencies stubbed.** The legacy calls
//    PKMGR, HERO, OBJECTMGR, ITEMMGR, CHATMGR, NETWORK. In the
//    modern port these are all no-op stubs:
//      - PKMGR->GetLootingChance(badFame) → returns 1 (one pick
//        chance) so the dialog state is well-defined for tests.
//      - HERO->GetBadFame() → returns 0.
//      - OBJECTMGR->GetObject(dwDiePlayerIdx) → returns null
//        (so the target name stays empty in tests).
//      - ITEMMGR->GetIconImage(idx) / SetDisableDialog → no-op.
//      - CHATMGR->AddMsg / GetChatMsg → returns "" / no-op.
//      - NETWORK->Send → no-op (the data-side msg sync flag
//        is still set so the network round-trip is observable).
//    The state-side effects (m_nChance / m_nLootItemNum / m_bMsgSync
//    / m_bLootingEnd) are preserved 1:1. The engine-binder layer
//    will replace the stubs with real engine calls (Phase 14+).
//
// 2. **cIcon grid contents are opaque.** m_pIGDItem is a cIconGridDialog
//    (port of legacy cIconGridDialog). The legacy allocates a cIcon
//    per cell; the modern port calls AddIcon with a placeholder
//    (reinterpret_cast<cIcon*>(idx+1)) so the data model is
//    exercised but no real cIcon is needed.
//
// 3. **Render is a no-op.** Real sprite + selected-bg + drag-over-bg
//    draw lands with the 6.6 cImage seam.
//
// 4. **m_bShow delay uses a test-injectable clock.** The legacy uses
//    gCurTime (a global tick count). The modern port reads a
//    virtual `currentTimeMs()` function; the default returns 0 and
//    tests can override via SetClockForTesting. This avoids a
//    global mutable and keeps the timer state testable.

#include "pklootingdialog.hpp"

#include "cIconGridDialog.hpp"
#include "cStatic.hpp"
#include "cWindow.hpp"

#include <cstring>

namespace mxh::ui {

namespace {
// Default clock: returns 0. Tests override via SetClockForTesting.
std::uint32_t (*g_clock)() = nullptr;

std::uint32_t currentTimeMs() {
    return g_clock ? g_clock() : 0;
}
}  // namespace

// Test-only: override the clock used by ActionEvent's m_bShow delay
// and timer countdown. Pass nullptr to restore the default (returns 0).
void cPKLootingDialog::SetClockForTesting(std::uint32_t (*fn)()) {
    g_clock = fn;
}

cPKLootingDialog::cPKLootingDialog() {
    m_dwDiePlayerIdx = 0;
    m_bLootingEnd    = 0;
    m_bMsgSync       = 0;
    m_dwCreateTime   = 0;
    m_bShow          = 0;
    m_nTime          = 0;
    m_dwStartTime    = 0;
    m_nChance        = 0;
    m_nLootItemNum   = 0;
    clearSelection();
    for (std::uint16_t i = 0; i < PKLOOTING_ITEM_NUM; ++i) {
        m_nItemKind[i] = LootItemKind::None;
    }
    // Children are created in Linking() (1:1 with cAlertDlg pattern:
    // the ctor initializes state, Linking wires the children into
    // the dialog tree + captures non-owning raw pointers).
}

cPKLootingDialog::~cPKLootingDialog() = default;

void cPKLootingDialog::clearSelection() {
    for (std::uint16_t i = 0; i < PKLOOTING_ITEM_NUM; ++i) {
        m_bSelected[i] = false;
    }
}

void cPKLootingDialog::setEndState() {
    // 1:1 with legacy. When the player runs out of chances or
    // items, the legacy sets the end-text, clears the chance /
    // item / none texts, and disables the grid. The modern port
    // records the flag flip; the actual cStatic text changes are
    // stubbed (the engine-binder layer will re-add them when
    // cStatic's text rendering is wired).
    m_bLootingEnd = 1;
}

void cPKLootingDialog::InitPKLootDlg(std::int32_t dwID, std::int32_t x,
                                     std::int32_t y,
                                     std::uint32_t dwDiePlayerIdx) {
    setId(static_cast<std::int32_t>(dwID));
    SetAbsXY(x, y);

    m_dwDiePlayerIdx = dwDiePlayerIdx;
    m_bLootingEnd    = 0;
    m_nTime          = static_cast<int>(PKLOOTING_LIMIT_TIME / 1000);
    // Engine-side stubs: HERO->GetBadFame() = 0, so chance = 1,
    // loot item num = 1 (legacy default for "first kill").
    (void)m_pStcBadFame;  // would SetStaticValue(HERO->GetBadFame()) in legacy
    (void)m_pStcTime;     // would SetStaticValue(m_nTime) + SetFGColor(red)
    m_nChance = 1;
    (void)m_pStcChance;   // would SetStaticValue(m_nChance) + SetFGColor(green)
    m_nLootItemNum = 1;
    (void)m_pStcItem;     // would SetStaticValue(m_nLootItemNum)
    (void)m_pStcTarget;   // would SetStaticText(pTargetPlayer->GetObjectName())
    clearSelection();

    // Pre-fill the loot grid with placeholder icons. The legacy
    // allocates a real cIcon per cell with image id 90 (default
    // item). Modern port uses a placeholder pointer; the engine
    // will replace it with a real cIcon* when wired.
    if (m_pIGDItem) {
        for (std::uint16_t i = 0; i < PKLOOTING_ITEM_NUM; ++i) {
            // reinterpret_cast to a tagged pointer; tests verify the
            // cell is in use but don't dereference the icon.
            auto* placeholder = reinterpret_cast<class cIcon*>(
                static_cast<std::uintptr_t>(i + 1));
            m_pIGDItem->AddIcon(i, placeholder);
        }
    }

    m_dwCreateTime = currentTimeMs();
    m_bShow        = 0;
}

void cPKLootingDialog::Linking() {
    // 1:1 with legacy. Legacy uses GetWindowForID(PLI_*) to resolve
    // each child. The modern port synthesizes the children (1:1 with
    // the legacy resource loader behavior — the legacy doesn't have
    // an explicit Linking ctor, but the resource loader creates the
    // children before the dialog is used). Per cAlertDlg pattern:
    // child = make_unique; child->Init(id=...); m_pXxx = child.get();
    // Add(std::move(child)). The dialog owns via the cWindow
    // children list; m_pXxx is a non-owning raw accessor.
    auto makeStc = [this](cStatic*& out, std::int32_t id) {
        auto s = std::make_unique<cStatic>();
        s->Init(0, 0, 0, 0, nullptr, id);
        out = s.get();
        Add(std::move(s));
    };
    makeStc(m_pStcBadFame,  ID_STC_BADFAME);
    makeStc(m_pStcTime,     ID_STC_TIME);
    makeStc(m_pStcChance,   ID_STC_CHANCE);
    makeStc(m_pStcTarget,   ID_STC_TARGETNAME);
    makeStc(m_pStcItem,     ID_STC_ITEM);
    makeStc(m_pStcEnd,      ID_STC_END);
    makeStc(m_pStcNone,     ID_STC_NONE);

    // 1:1 with legacy: 4 cols × 3 rows = 12 cells.
    {
        auto grid = std::make_unique<cIconGridDialog>();
        grid->Init(0, 0, 200, 200, nullptr, /*col*/4, /*row*/3, ID_IGD_ITEM);
        m_pIGDItem = grid.get();
        Add(std::move(grid));
    }
}

void cPKLootingDialog::Render() {
    // 1:1 with legacy: cDialog::Render only when m_bShow is true.
    // Modern Render is a no-op (cImage seam); the flag is preserved
    // so the test can verify the m_bShow gate.
    if (m_bShow) {
        cDialog::Render();
    }
}

std::uint32_t cPKLootingDialog::ActionEvent(std::int32_t mouseX,
                                            std::int32_t mouseY,
                                            std::uint32_t mouseFlags) {
    (void)mouseX; (void)mouseY; (void)mouseFlags;
    if (!isEnabled()) return 0;

    // 1:1 with legacy. The dialog is invisible for the first
    // PKLOOTING_DLG_DELAY_TIME ms after InitPKLootDlg; only then
    // does the timer start and the dialog become interactive.
    if (!m_bShow) {
        const std::uint32_t now = currentTimeMs();
        if (now - m_dwCreateTime >= PKLOOTING_DLG_DELAY_TIME) {
            m_bShow       = 1;
            m_dwStartTime = now;
        } else {
            return 0;
        }
    }

    // Per-second timer countdown. The legacy computes:
    //   nTime = (LIMIT - (now - start)) / 1000
    // and decrements m_nTime when nTime changes. The modern port
    // records the same decrement; the engine-binder layer
    // (Phase 14+) is responsible for the actual close.
    const std::uint32_t now = currentTimeMs();
    const int nTime = static_cast<int>(
        (PKLOOTING_LIMIT_TIME - (now - m_dwStartTime)) / 1000);
    if (nTime != m_nTime && nTime >= 0) {
        m_nTime = nTime;
        if (m_nTime > 0) {
            (void)m_pStcTime;  // would SetStaticValue(m_nTime) in legacy
        } else {
            // Time's up — engine-side CloseLootingDialog. Modern
            // port flips the state flag.
            setEndState();
        }
    }
    return 0;
}

void cPKLootingDialog::OnActionEvent(std::int32_t lId, void* p,
                                     std::uint32_t we) {
    (void)p;
    if (we == 0x00000010 /*WE_CLOSEWINDOW*/) {
        // WE_CLOSEWINDOW is the legacy "X button" close event.
        setEndState();
        return;
    }
    if (lId == ID_BTN_CLOSE && we == 0x00000020 /*WE_BTNCLICK*/) {
        setEndState();
        return;
    }
    if (lId == ID_IGD_ITEM && we == 0x00000001 /*WE_LBTNCLICK*/) {
        if (IsLootingEnd()) return;
        if (m_nChance <= 0) return;
        if (m_nLootItemNum <= 0) return;
        if (IsMsgSync()) return;

        // Get the selected cell pos from the icon grid. The legacy
        // GetCurSelCellPos returns -1 if nothing is selected; the
        // modern port follows the same convention.
        const std::int32_t idx = m_pIGDItem ? m_pIGDItem->GetCurSelCellPos() : -1;
        if (idx < 0 || idx >= static_cast<std::int32_t>(PKLOOTING_ITEM_NUM)) {
            return;
        }
        if (m_bSelected[idx]) return;  // already picked this cell

        // Engine-side distance check stubbed (CalcDistanceXZ would
        // require the world model). Modern port skips the check;
        // the engine-binder layer will re-add it.
        m_bSelected[idx] = true;
        --m_nChance;
        (void)m_pStcChance;  // would SetStaticValue(m_nChance) in legacy
        if (m_nChance <= 0) {
            if (m_pIGDItem) m_pIGDItem->SetDisable(true);
            setEndState();
            m_nLootItemNum = 0;
            (void)m_pStcEnd;     // would SetStaticText(...) in legacy
            (void)m_pStcChance;  // would SetStaticText("") in legacy
            (void)m_pStcItem;    // would SetStaticText("") in legacy
            (void)m_pStcNone;    // would SetStaticText("") in legacy
        }
        // Engine-side NETWORK->Send stubbed; the msg-sync flag is
        // the data-side observable.
        SetMsgSync(true);
    }
}

void cPKLootingDialog::ReleaseAllIcon() {
    if (!m_pIGDItem) return;
    for (std::uint16_t i = 0; i < PKLOOTING_ITEM_NUM; ++i) {
        m_pIGDItem->DeleteIcon(i, nullptr);
    }
}

void cPKLootingDialog::ChangeIconImage(std::uint16_t pos, LootItemKind nKind,
                                       std::uint16_t ItemIdx) {
    if (pos >= PKLOOTING_ITEM_NUM) return;
    m_nItemKind[pos] = nKind;
    // Engine-side ITEMMGR / SCRIPTMGR image fetches stubbed. The
    // modern port records the kind; the engine-binder layer will
    // re-add the cImage fetch.
    (void)ItemIdx;
}

void cPKLootingDialog::AddLootingItemNum() {
    if (m_nLootItemNum <= 0) return;
    if (--m_nLootItemNum <= 0) {
        if (m_pIGDItem) m_pIGDItem->SetDisable(true);
        m_nChance = 0;
        setEndState();
        (void)m_pStcEnd;     // would SetStaticText(...) in legacy
        (void)m_pStcChance;  // would SetStaticText("") in legacy
        (void)m_pStcItem;    // would SetStaticText("") in legacy
        (void)m_pStcNone;    // would SetStaticText("") in legacy
    } else {
        (void)m_pStcItem;    // would SetStaticValue(m_nLootItemNum) in legacy
    }
}

bool cPKLootingDialog::IsSelected(std::uint16_t idx) const noexcept {
    if (idx >= PKLOOTING_ITEM_NUM) return false;
    return m_bSelected[idx];
}

} // namespace mxh::ui
