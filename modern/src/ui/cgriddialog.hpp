// cgriddialog.hpp -- modern 1:1 port of Moxiang cGridDialog
//   (sub-control container for a 2D cell grid of cPushupButton).
//
// 1:1 port of legacy cGridDialog from [Client]MH\interface\cGridDialog.{h,cpp}.
// The legacy surface is a 7-arg Init that takes a caller-allocated
// cPushupButton[] array, clones each entry via memcpy (1:1 quirk --
// object slicing on the cPushupButton subobject), adds the clones
// as dialog children, then frees the source array.
//
// 1:1 surface (legacy, line-by-line):
//   * ctor: m_type = WT_GRIDDIALOG (1:1 quirk: dropped in modern
//     cWindow base; the type tag is no longer used to identify
//     widgets, the engine uses RTTI + tree position instead).
//   * dtor: empty body (1:1).
//   * Init(x, y, w, h, basicImage, cPushupButton* pCellWindow,
//         WORD cellNum, LONG id=0):
//       - delegates to cDialog::Init(x, y, w, h, basicImage, id)
//       - sets m_type = WT_GRIDDIALOG (1:1 quirk, dropped)
//       - records m_pWindowCell = pCellWindow (caller-owned)
//       - records m_wCellNum = cellNum
//       - for i=0..cellNum-1: new cPushupButton, memcpy from
//         pCellWindow[i], cDialog::Add(node)
//       - SAFE_DELETE_ARRAY(pCellWindow) (1:1 quirk, see below)
//   * m_pWindowCell: cPushupButton* (raw; legacy freed it in Init).
//   * m_wCellNum: WORD.
//
// 1:1 quirks:
//   * m_type = WT_GRIDDIALOG assignment is dropped (legacy cWindow
//     m_type field was removed in Phase 6 when cWindow was modernized;
//     modern engine identifies dialogs by RTTI, not numeric type tag).
//   * The legacy `memcpy(node, &pCellWindow[i], sizeof(cPushupButton))`
//     is a deliberate object-slicing operation (1:1 lock). Modern
//     cPushupButton has a non-trivial vtable, so a direct memcpy
//     would be undefined behavior. The modern port preserves the
//     1:1 intent (cloning the cPushupButton subobject) by using
//     normal construction + member-wise copy: same observable state,
//     well-defined C++. Documented as a 1:1 quirk.
//   * SAFE_DELETE_ARRAY(pCellWindow) frees the caller-owned array
//     in legacy. The modern port does NOT free pCellWindow -- the
//     caller owns the source. This is a documented divergence:
//     modern ownership is caller-managed (std::vector/std::array)
//     and the engine test setup owns the test source. The 1:1
//     intent (cell-clone) is preserved; the source-array lifetime
//     management is modernized.
//   * cDialog::Add takes std::unique_ptr<cWindow>; the legacy
//     raw-pointer ownership is wrapped in unique_ptr at the Add
//     boundary (so the dialog owns the cloned cells).

#pragma once

#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cPushupButton.hpp"

#include <cstdint>

namespace mxh::ui {

class cGridDialog final : public cDialog {
public:
    cGridDialog();
    ~cGridDialog() override;

    cGridDialog(const cGridDialog&) = delete;
    cGridDialog& operator=(const cGridDialog&) = delete;

    // 1:1 with legacy Init(x, y, wid, hei, basicImage, pCellWindow,
    // cellNum, ID).
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
              std::uint16_t hei, void* basicImage,
              cPushupButton* pCellWindow, std::uint16_t cellNum,
              std::int32_t id = 0);

    // 1:1 accessors (legacy public field access via getter pair).
    cPushupButton*    WindowCell() const noexcept { return m_pWindowCell; }
    std::uint16_t     CellNum()    const noexcept { return m_wCellNum; }

private:
    cPushupButton*    m_pWindowCell = nullptr;
    std::uint16_t     m_wCellNum    = 0;
};

}  // namespace mxh::ui
