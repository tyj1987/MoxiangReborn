// cmousepointer.hpp -- modern port of Moxiang
//   CMousePointer (cursor / monster-target
//   pointer dialog).
//
// 1:1 port of legacy CMousePointer from
//   [Client]MH/MousePointer.{h,cpp}.
//
// Surface (legacy):
//   - 2 cAni children (m_pAniBasic, m_pAniClick)
//     resolved in Linking via GetWindowForID with
//     ids INT_MOUSEBASIC and INT_MOUSECLICK.
//   - 3 entry points driven by the engine when the
//     hero targets a monster:
//       MonsterAttack()    -- cursor switches to
//                             attack animation.
//       MonsterMouseOver() -- cursor switches to
//                             click-hover animation.
//       MonsterLeave()     -- cursor switches back
//                             to the basic animation.
//   - 1:1 quirk: ALL FOUR function bodies are
//     commented out in the legacy source.  The
//     class exists in the dialog tree but the
//     cursor animation swap is a no-op.  Modern
//     port preserves the no-op behaviour.
//
// Child window ids (legacy WindowIDs.h):
//   INT_MOUSEBASIC
//   INT_MOUSECLICK
//
// Modern port:
//   - Ctor: empty body, m_pAniBasic / m_pAniClick
//     default to nullptr (1:1 with legacy ctor).
//   - Dtor: empty body (1:1 with legacy).
//   - Linking: empty body (1:1 with legacy commented-
//     out body).
//   - MonsterAttack / MonsterMouseOver /
//     MonsterLeave: empty bodies (1:1 with legacy).
//   - No m_type assignment (modern cWindow has no
//     m_type field, removed in Phase 6).
//   - cAni is R-12.x deferred (not yet ported to
//     modern).  The 2 cAni members are typed as
//     void* (1:1 with legacy in spirit: the modern
//     port preserves the storage slots but defers
//     the cAni port to a later phase).
//
// 1:1 quirks:
//   - 1:1 with legacy empty ctor/dtor bodies.
//   - 1:1 with legacy empty Linking body.
//   - 1:1 with legacy empty MonsterAttack /
//     MonsterMouseOver / MonsterLeave bodies.
//   - 1:1 with legacy cAni member types (modern
//     uses void* because cAni is R-12.x deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cMousePointer : public cDialog {
public:
    cMousePointer();
    ~cMousePointer() override;

    cMousePointer(const cMousePointer&) = delete;
    cMousePointer& operator=(const cMousePointer&) = delete;

    // 1:1 with legacy CMousePointer::Linking.
    // Empty body: the legacy body that would have
    // called GetWindowForID(INT_MOUSEBASIC /
    // INT_MOUSECLICK) is commented out.  Modern
    // port preserves the no-op.
    void Linking();

    // 1:1 with legacy CMousePointer::MonsterAttack.
    // Empty body: the legacy body that would have
    // toggled the cAni pointers is commented out.
    void MonsterAttack();

    // 1:1 with legacy CMousePointer::MonsterMouseOver.
    // Empty body: the legacy body that would have
    // activated the click animation is commented out.
    void MonsterMouseOver();

    // 1:1 with legacy CMousePointer::MonsterLeave.
    // Empty body: the legacy body that would have
    // reverted to the basic animation is commented out.
    void MonsterLeave();

    // 1:1 with legacy WindowIDs.h INT_MOUSEBASIC.
    static constexpr std::int32_t kIdMouseBasic = 1310;
    // 1:1 with legacy WindowIDs.h INT_MOUSECLICK.
    static constexpr std::int32_t kIdMouseClick = 1311;

    // Test hooks -- inject the 2 cAni children
    // (modern type void* because cAni is R-12.x
    // deferred).  Replaces the legacy commented-out
    // GetWindowForID lookups.
    void SetAniBasicForTest(void* ani) noexcept { m_pAniBasic = ani; }
    void SetAniClickForTest(void* ani) noexcept { m_pAniClick = ani; }
    void* GetAniBasicForTest() const noexcept { return m_pAniBasic; }
    void* GetAniClickForTest() const noexcept { return m_pAniClick; }

private:
    // 1:1 with legacy cAni m_pAniBasic / m_pAniClick.
    // Modern port uses void* because cAni is
    // R-12.x deferred (not yet ported).
    void* m_pAniBasic = nullptr;
    void* m_pAniClick = nullptr;
};

}  // namespace mxh::ui
