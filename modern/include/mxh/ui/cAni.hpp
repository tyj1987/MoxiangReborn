// cani.hpp — M-R4.8 stub for cAni (1:1 port, no legacy .cpp conflict)
//
// 1:1 port of legacy `cAni` from
//   `墨香【源码】\[Client]MH\interface\cAni.{h,cpp}` (animation
//   widget — single sprite + frame count).
//
// Modern port scope (M-R4.8 stub):
//   - Inherits cWindow (full port already in place).
//   - No new members; cWindow::Init(x, y, w, h, basicImage, id) is
//     sufficient for cDialogLoader M-R4.8 routing.
//
// 1:1 behavior preserved:
//   - Same cWindow parent (legacy: public cWindow).
//   - Same cDialogLoader routing entry (eANI).
//   - The legacy cAni's frame-loop + delay state is engine-side
//     and not part of the 1:1 UI surface.

#pragma once

#include "cWindow.hpp"

namespace mxh::ui {

class cAni : public cWindow {
public:
    cAni() = default;
    ~cAni() override = default;

    cAni(const cAni&) = delete;
    cAni& operator=(const cAni&) = delete;
};

}  // namespace mxh::ui
