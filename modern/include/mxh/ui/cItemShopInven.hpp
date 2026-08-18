// citemshopinven.hpp — M-R4.8 stub for cItemShopInven (1:1 port, no legacy .cpp conflict)
//
// 1:1 port of legacy `CItemShopInven` from
//   `墨香【源码】\[Client]MH\interface\ItemShopInven.{h,cpp}` (item
//   shop inventory icon grid).
//
// Modern port scope (M-R4.8 stub):
//   - Inherits cIconGridDialog (full port already in place: cell
//     layout, AddIcon2D).
//   - ctor/dtor = default inline (no legacy .cpp 定义).
//   - cIconGridDialog::Init(x, y, w, h, basic, col=1, row=1, id) is
//     sufficient for cDialogLoader M-R4.8 routing.
//
// 1:1 behavior preserved:
//   - Same cIconGridDialog parent (legacy: public cIconGridDialog).
//   - Same cDialogLoader routing entry (eSHOPITEMINVENGRID).
//   - The legacy drag-drop singleton dispatch is engine-side
//     and not part of the 1:1 UI surface.

#pragma once

#include "cIconGridDialog.hpp"

namespace mxh::ui {

class cItemShopInven : public cIconGridDialog {
public:
    cItemShopInven() = default;
    ~cItemShopInven() override = default;

    cItemShopInven(const cItemShopInven&) = delete;
    cItemShopInven& operator=(const cItemShopInven&) = delete;
};

}  // namespace mxh::ui
