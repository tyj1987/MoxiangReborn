// chelper.hpp -- modern port of Moxiang
//   cHelper (guide / newbie helper animation).
//
// 1:1 port of legacy cHelper from
//   [Client]MH/Helper.{h,cpp}.
//
// Surface (legacy):
//   - 1 cAni array (m_MotionList[emHM_MAX]) of
//     helper motions (Stand at the moment).
//   - Render() override: forwards to the current
//     motion (m_MotionList[m_wCurMotion]) cAni
//     when the dialog is active.
//   - ActionEvent(CMouse*) override: forwards to
//     the current motion (returns WE_NULL).
//     Modern port uses the R-12.x ActionEvent
//     signature: ActionEvent(int x, int y, int flags).
//   - SetMotion / GetMotion: pick which motion in
//     m_MotionList is active.
//   - SetMaxSprite / AddSprite: configure sprites
//     for a given motion slot.
//   - SetStartTime / SetGreetTime / GetGreetTime /
//     IsGreetCheck / StopGreetCheck: helper tutorial
//     timer state (legacy greet check body is
//     commented out but the timer fields persist).
//
// Modern port:
//   - Inherits cDialog (1:1 with legacy cHelper :
//     public cDialog).
//   - enum HelperMotion { Stand, Max } (1:1 with
//     legacy HELPER_MOTION { emHM_Stand, emHM_MAX }).
//   - m_MotionList becomes an array of void*
//     because cAni is R-12.x deferred (not yet
//     ported to modern).  Storage slots are
//     preserved 1:1 with legacy.
//   - WORD m_wCurMotion -> uint16_t m_curMotion.
//   - DWORD m_dwStartTime / m_dwGreetTime /
//     m_dwCurTime -> uint32_t m_startTime /
//     m_greetTime / m_curTime.
//   - BOOL m_bGreetCheck -> bool m_greetCheck.
//   - Render() override forwards to the current
//     motion via a host-injected cAni* callback
//     (the cAni framework is R-12.x deferred).
//   - ActionEvent(x, y, flags) override returns
//     WE_NULL (1:1 with legacy).
//   - AddSprite / SetMaxSprite: host-injected
//     callbacks (cAni is R-12.x deferred).
//   - SetMotion: 1:1 with legacy motion swap;
//     m_curMotion = Idx.
//   - SetStartTime / SetGreetTime / GetGreetTime:
//     1:1 with legacy timer fields.
//   - IsGreetCheck / StopGreetCheck: 1:1 with
//     legacy greet-check flag.
//   - No m_type assignment (modern cWindow has no
//     m_type field, removed in Phase 6).
//
// 1:1 quirks:
//   - 1:1 with legacy m_MotionList[emHM_MAX]: the
//     modern port keeps an array of kMaxMotionSlots
//     void* pointers (1:1 storage shape).
//   - 1:1 with legacy SetMotion swap: m_curMotion
//     tracks the active motion index.
//   - 1:1 with legacy Render() active check:
//     the modern port returns early if !IsActive().
//   - 1:1 with legacy ActionEvent returning WE_NULL.
//   - 1:1 with legacy greet-check commented-out
//     body: the timer fields are preserved but no
//     time-check fires.
//   - 1:1 with legacy cAni type: modern uses void*
//     (R-12.x deferred).
//   - 1:1 with legacy WE_NULL (0) ActionEvent return.

#pragma once

#include "cdialog.hpp"
#include "cwindow.hpp"

#include <cstdint>

namespace mxh::ui {

// 1:1 with legacy HELPER_MOTION enum.
enum class HelperMotion : std::uint8_t {
    Stand = 0,
    Max   = 1,
};

class cHelper : public cDialog {
public:
    // 1:1 with legacy emHM_MAX.
    static constexpr std::size_t kMaxMotionSlots = 1;

    cHelper();
    ~cHelper() override;

    cHelper(const cHelper&) = delete;
    cHelper& operator=(const cHelper&) = delete;

    // 1:1 with legacy cHelper::Render override.
    // Returns early when the dialog is not active.
    // Renders the current motion via a host-
    // injected callback (cAni is R-12.x deferred).
    void Render() override;

    // 1:1 with legacy cHelper::ActionEvent override.
    // Forwards to the current motion via the host-
    // injected callback (cAni is R-12.x deferred).
    // Modern R-12.x signature uses (x, y, flags)
    // instead of (CMouse*).  Returns WE_NULL (0)
    // (1:1 with legacy).
    std::uint32_t ActionEvent(std::int32_t mouseX,
                              std::int32_t mouseY,
                              std::uint32_t mouseFlags) noexcept override;

    // 1:1 with legacy cHelper::SetMotion.
    // Sets m_curMotion = Idx (no-op on the cAni
    // storage because cAni is R-12.x deferred).
    void SetMotion(HelperMotion idx) noexcept;

    // 1:1 with legacy cHelper::GetMotion.
    HelperMotion GetMotion() const noexcept;

    // 1:1 with legacy cHelper::SetMaxSprite.
    // Forwards to the cAni slot via host-injected
    // callback (R-12.x deferred).
    void SetMaxSprite(std::uint16_t idx, std::int32_t nMaxNum);

    // 1:1 with legacy cHelper::AddSprite.
    // Forwards to the cAni slot via host-injected
    // callback (R-12.x deferred).
    void AddSprite(std::uint16_t idx, void* sprite, std::uint16_t delay);

    // 1:1 with legacy cHelper::SetStartTime.
    void SetStartTime(std::uint32_t time) noexcept;

    // 1:1 with legacy cHelper::SetGreetTime.
    void SetGreetTime(std::uint32_t time) noexcept;

    // 1:1 with legacy cHelper::GetGreetTime.
    std::uint32_t GetGreetTime() const noexcept;

    // 1:1 with legacy cHelper::IsGreetCheck.
    bool IsGreetCheck() const noexcept;

    // 1:1 with legacy cHelper::StopGreetCheck.
    void StopGreetCheck() noexcept;

    // ---- Test hooks (host-injected cAni slots) ----
    // 1:1 with legacy cAni m_MotionList[emHM_MAX].
    // Modern uses void* because cAni is R-12.x deferred.
    void SetMotionAniForTest(std::size_t slot, void* ani) noexcept;
    void* GetMotionAniForTest(std::size_t slot) const noexcept;

    // Host-injected callback signatures for the
    // 3 R-12.x deferred cAni surfaces.
    using RenderCallback = void(*)(void* ani, void* user);
    using ActionCallback = void(*)(void* ani, void* user);
    using AddSpriteCallback = void(*)(void* ani, void* sprite, std::uint16_t delay, void* user);
    using SetMaxSpriteCallback = void(*)(void* ani, std::int32_t nMaxNum, void* user);

    void SetRenderCallbackForTest(RenderCallback cb, void* user) noexcept;
    void SetActionCallbackForTest(ActionCallback cb, void* user) noexcept;
    void SetAddSpriteCallbackForTest(AddSpriteCallback cb, void* user) noexcept;
    void SetSetMaxSpriteCallbackForTest(SetMaxSpriteCallback cb, void* user) noexcept;

    // 1:1 with legacy WE_NULL return from ActionEvent.
    static constexpr std::uint32_t kWeNull = 0;

private:
    // 1:1 with legacy cAni m_MotionList[emHM_MAX].
    // Modern port uses void* because cAni is R-12.x deferred.
    void* m_motionList[kMaxMotionSlots] = {nullptr};
    std::uint16_t m_curMotion = 0;
    std::uint32_t m_startTime = 0;
    std::uint32_t m_greetTime = 0;
    std::uint32_t m_curTime = 0;
    bool m_greetCheck = true;

    RenderCallback m_renderCb = nullptr;
    void* m_renderUser = nullptr;
    ActionCallback m_actionCb = nullptr;
    void* m_actionUser = nullptr;
    AddSpriteCallback m_addSpriteCb = nullptr;
    void* m_addSpriteUser = nullptr;
    SetMaxSpriteCallback m_setMaxSpriteCb = nullptr;
    void* m_setMaxSpriteUser = nullptr;
};

}  // namespace mxh::ui
