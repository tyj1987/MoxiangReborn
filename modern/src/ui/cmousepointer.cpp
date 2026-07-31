// cmousepointer.cpp -- modern implementation of
//   Moxiang CMousePointer (cursor / monster-target
//   pointer dialog).

#include "cmousepointer.hpp"

namespace mxh::ui {

cMousePointer::cMousePointer() = default;

cMousePointer::~cMousePointer() = default;

void cMousePointer::Linking() {
    // 1:1 with legacy empty Linking body.  The
    // legacy GetWindowForID(INT_MOUSEBASIC) and
    // GetWindowForID(INT_MOUSECLICK) calls are
    // commented out in the legacy source.
}

void cMousePointer::MonsterAttack() {
    // 1:1 with legacy empty MonsterAttack body.
    // The legacy m_pAniBasic->SetActive(FALSE) +
    // m_pAniClick->SetActive(TRUE) + Play()
    // sequence is commented out in the legacy
    // source.
}

void cMousePointer::MonsterMouseOver() {
    // 1:1 with legacy empty MonsterMouseOver body.
    // The legacy m_pAniBasic->SetActive(FALSE) +
    // m_pAniClick->SetActive(TRUE) sequence is
    // commented out in the legacy source.
}

void cMousePointer::MonsterLeave() {
    // 1:1 with legacy empty MonsterLeave body.
    // The legacy m_pAniBasic->SetActive(TRUE) +
    // m_pAniClick->SetActive(FALSE) sequence is
    // commented out in the legacy source.
}

}  // namespace mxh::ui
