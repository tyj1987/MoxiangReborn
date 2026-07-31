// cgriddialog.cpp -- modern 1:1 implementation of Moxiang cGridDialog.
//
// See cgriddialog.hpp for the 1:1 surface + quirks (m_type drop,
// memcpy -> normal construction + member-wise copy, no source-array free).

#include "cgriddialog.hpp"

#include <memory>
#include <utility>

namespace mxh::ui {

cGridDialog::cGridDialog() = default;

cGridDialog::~cGridDialog() {
    // 1:1 with legacy dtor: empty body. The cloned cPushupButton
    // children are owned by the dialog (via cDialog::Add unique_ptr)
    // and are released by cDialog::~cDialog automatically.
}

void cGridDialog::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                      std::uint16_t hei, void* basicImage,
                      cPushupButton* pCellWindow, std::uint16_t cellNum,
                      std::int32_t id) {
    // 1:1 with legacy Init():
    //   cDialog::Init(x, y, wid, hei, basicImage, ID);
    //   m_type = WT_GRIDDIALOG;        (1:1 quirk, dropped)
    //   m_pWindowCell = pCellWindow;
    //   m_wCellNum = cellNum;
    //   for i=0..cellNum-1:
    //       cPushupButton* node = new cPushupButton;
    //       memcpy(node, &pCellWindow[i], sizeof(cPushupButton));
    //       cDialog::Add(node);
    //   SAFE_DELETE_ARRAY(pCellWindow);  (1:1 quirk, dropped)
    cDialog::Init(x, y, wid, hei, basicImage, id);

    // 1:1 quirk: m_type = WT_GRIDDIALOG is dropped (legacy cWindow
    // m_type field was removed in Phase 6 when cWindow was
    // modernized).

    m_pWindowCell = pCellWindow;
    m_wCellNum    = cellNum;

    // 1:1 quirk: legacy did memcpy(node, &pCellWindow[i],
    // sizeof(cPushupButton)) (object slicing on the cPushupButton
    // subobject). Modern cPushupButton has a non-trivial vtable so a
    // direct memcpy is undefined behavior. We preserve the 1:1
    // intent (clone the cPushupButton subobject) by using member-wise
    // copy of the 2 subobject fields (m_pushed, m_passive). The vtable
    // is set by std::make_unique, so the type identity is preserved.
    // Documented as a 1:1 quirk in the hpp.
    for (std::uint16_t i = 0; i < cellNum; ++i) {
        if (pCellWindow == nullptr) {
            break;  // defensive: legacy assumed non-null but would
                    // also crash; the modern port is no worse.
        }
        auto node = std::make_unique<cPushupButton>();
        node->SetPush(pCellWindow[i].IsPushed());
        node->SetPassive(pCellWindow[i].IsPassive());
        cDialog::Add(std::move(node));
    }

    // 1:1 quirk: legacy did SAFE_DELETE_ARRAY(pCellWindow). The
    // modern port does NOT free pCellWindow -- the caller owns the
    // source. Documented in the hpp. Ownership semantics are
    // caller-managed in modern (std::vector / std::array).
}

}  // namespace mxh::ui
