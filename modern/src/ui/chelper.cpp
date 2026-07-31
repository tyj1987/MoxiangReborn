// chelper.cpp -- modern implementation of
//   Moxiang cHelper (guide / newbie helper
//   animation dialog).

#include "chelper.hpp"

namespace mxh::ui {

cHelper::cHelper() = default;

cHelper::~cHelper() = default;

void cHelper::Render() {
    // 1:1 with legacy cHelper::Render early-exit:
    // returns immediately when the dialog is not
    // active.  Legacy uses IsActive(); the modern
    // base class exposes isActive() (lowercase a).
    if (!isActive()) return;

    // 1:1 with legacy m_MotionList[m_wCurMotion]
    // .Render().  The cAni is R-12.x deferred, so
    // the modern port forwards to the host-injected
    // render callback with the active motion slot.
    if (m_curMotion < kMaxMotionSlots &&
        m_motionList[m_curMotion] && m_renderCb) {
        m_renderCb(m_motionList[m_curMotion], m_renderUser);
    }
}

std::uint32_t cHelper::ActionEvent(std::int32_t mouseX,
                                  std::int32_t mouseY,
                                  std::uint32_t mouseFlags) noexcept {
    // 1:1 with legacy cHelper::ActionEvent forwards
    // to m_MotionList[m_wCurMotion].ActionEvent(...).
    // The cAni is R-12.x deferred, so the modern
    // port forwards to the host-injected action
    // callback with the active motion slot.
    if (m_curMotion < kMaxMotionSlots &&
        m_motionList[m_curMotion] && m_actionCb) {
        m_actionCb(m_motionList[m_curMotion], m_actionUser);
    }

    // 1:1 with legacy WE_NULL return (the legacy
    // greet-check body is commented out).
    (void)mouseX;
    (void)mouseY;
    (void)mouseFlags;
    return kWeNull;
}

void cHelper::SetMotion(HelperMotion idx) noexcept {
    // 1:1 with legacy cHelper::SetMotion: m_wCurMotion
    // = Idx.  The legacy body also toggles the cAni
    // SetActive / Stop / SetCurSpriteIdx on the old
    // motion and SetActive / Play on the new motion.
    // The cAni is R-12.x deferred, so the modern
    // port preserves the m_curMotion swap and the
    // active/play transitions are deferred to the
    // host via the test hooks.
    const auto i = static_cast<std::size_t>(idx);
    if (i < kMaxMotionSlots) {
        m_curMotion = static_cast<std::uint16_t>(i);
    }
}

HelperMotion cHelper::GetMotion() const noexcept {
    return static_cast<HelperMotion>(m_curMotion);
}

void cHelper::SetMaxSprite(std::uint16_t idx, std::int32_t nMaxNum) {
    // 1:1 with legacy cHelper::SetMaxSprite: forwards
    // to m_MotionList[wIdx].SetMaxSprite(nMaxNum).
    // The cAni is R-12.x deferred, so the modern
    // port forwards to the host-injected callback.
    if (idx >= kMaxMotionSlots) return;
    if (m_motionList[idx] && m_setMaxSpriteCb) {
        m_setMaxSpriteCb(m_motionList[idx], nMaxNum, m_setMaxSpriteUser);
    }
}

void cHelper::AddSprite(std::uint16_t idx, void* sprite, std::uint16_t delay) {
    // 1:1 with legacy cHelper::AddSprite: forwards
    // to m_MotionList[wIdx].AddSprite(sprite, delay)
    // + Init(absPos, sprite size).  The cAni is
    // R-12.x deferred, so the modern port forwards
    // to the host-injected callback.  The legacy
    // Init(... absPos, sprite size, NULL) call is
    // deferred to the host as well.
    if (idx >= kMaxMotionSlots) return;
    if (m_motionList[idx] && m_addSpriteCb) {
        m_addSpriteCb(m_motionList[idx], sprite, delay, m_addSpriteUser);
    }
}

void cHelper::SetStartTime(std::uint32_t time) noexcept {
    // 1:1 with legacy cHelper::SetStartTime:
    //   m_dwStartTime = m_dwCurTime = time;
    //   m_bGreetCheck = TRUE;
    m_startTime = time;
    m_curTime = time;
    m_greetCheck = true;
}

void cHelper::SetGreetTime(std::uint32_t time) noexcept {
    // 1:1 with legacy cHelper::SetGreetTime:
    //   m_dwGreetTime = time;
    m_greetTime = time;
}

std::uint32_t cHelper::GetGreetTime() const noexcept {
    return m_greetTime;
}

bool cHelper::IsGreetCheck() const noexcept {
    return m_greetCheck;
}

void cHelper::StopGreetCheck() noexcept {
    // 1:1 with legacy cHelper::StopGreetCheck:
    //   m_bGreetCheck = FALSE;
    m_greetCheck = false;
}

void cHelper::SetMotionAniForTest(std::size_t slot, void* ani) noexcept {
    if (slot < kMaxMotionSlots) m_motionList[slot] = ani;
}

void* cHelper::GetMotionAniForTest(std::size_t slot) const noexcept {
    if (slot < kMaxMotionSlots) return m_motionList[slot];
    return nullptr;
}

void cHelper::SetRenderCallbackForTest(RenderCallback cb, void* user) noexcept {
    m_renderCb = cb; m_renderUser = user;
}

void cHelper::SetActionCallbackForTest(ActionCallback cb, void* user) noexcept {
    m_actionCb = cb; m_actionUser = user;
}

void cHelper::SetAddSpriteCallbackForTest(AddSpriteCallback cb, void* user) noexcept {
    m_addSpriteCb = cb; m_addSpriteUser = user;
}

void cHelper::SetSetMaxSpriteCallbackForTest(SetMaxSpriteCallback cb, void* user) noexcept {
    m_setMaxSpriteCb = cb; m_setMaxSpriteUser = user;
}

}  // namespace mxh::ui
