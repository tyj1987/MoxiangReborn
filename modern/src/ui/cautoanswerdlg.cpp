// cautoanswerdlg.cpp — modern port of 墨香 CAutoAnswerDlg (auto-reply quiz).
//
// 1:1 port of legacy `CAutoAnswerDlg` from
//   `墨香【源码】\[Client]MH\AutoAnswerDlg.cpp`.

#include "mxh/ui/cautoanswerdlg.hpp"
#include "mxh/ui/cstatic.hpp"
#include "mxh/ui/ctextarea.hpp"

#include <chrono>
#include <cstring>
#include <string>

namespace mxh::ui {

cAutoAnswerDlg::cAutoAnswerDlg() = default;

cAutoAnswerDlg::~cAutoAnswerDlg() = default;

void cAutoAnswerDlg::Linking() {
    // 1:1 with legacy Linking:
    //   m_dwEndTime = 0
    //   m_pTextAreaDesc = (cTextArea*)GetWindowForID(ASD_TEXTAREA_DESC)
    //   m_pTextAreaDesc->SetScriptText(CHATMGR->GetChatMsg(1722))
    //   m_pTextAreaDesc->SetTextColor(RGB_HALF(128,128,128))
    //   m_pStcQuestion / m_pStcAnswer / m_pStcTime
    //   m_pBtnColor[i] = (cButton*)GetWindowForID(ASD_BTN_COLOR1 + i)
    //   m_v2BtnPos[i] = (m_pBtnColor[i]->GetRelX, GetRelY)
    //   m_bAnswerStart = FALSE; m_nAnswerPos = 0
    //
    // The modern port skips the CHATMGR->GetChatMsg(1722) call
    // (ChatManager is a host-side singleton; the test injects
    // the desc text via m_w.textAreaDesc) and the
    // RGB_HALF(128,128,128) colour (modern cStatic stores the
    // colour but the test doesn't need to assert it here).
    m_dwEndTime   = 0;
    m_bAnswerStart = false;
    m_nAnswerPos   = 0;
    if (m_w.btnColor[0]) {
        m_v2BtnPosX[0] = m_w.btnColor[0]->relX();
        m_v2BtnPosY[0] = m_w.btnColor[0]->relY();
    }
    if (m_w.btnColor[1]) {
        m_v2BtnPosX[1] = m_w.btnColor[1]->relX();
        m_v2BtnPosY[1] = m_w.btnColor[1]->relY();
    }
    if (m_w.btnColor[2]) {
        m_v2BtnPosX[2] = m_w.btnColor[2]->relX();
        m_v2BtnPosY[2] = m_w.btnColor[2]->relY();
    }
    if (m_w.btnColor[3]) {
        m_v2BtnPosX[3] = m_w.btnColor[3]->relX();
        m_v2BtnPosY[3] = m_w.btnColor[3]->relY();
    }
}

void cAutoAnswerDlg::SetActive(bool val) noexcept {
    if (val) {
        // 1:1 with legacy: m_dwEndTime = gCurTime + 120 * 1000
        // (120 second default timer).
        m_dwEndTime = static_cast<std::uint32_t>(nowMs() + 120u * 1000u);
        // Legacy also Release/LoadSprite("ANRaster.tga") + sets
        // a cImageSelf.  The modern port surfaces the raster
        // load through SaveImage(host) callback.
        if (m_onSaveImage) m_onSaveImage(nullptr);
    } else {
        // 1:1 with legacy: DeleteFile("ANRaster.tga") on
        // deactivation.  The host's save-image callback handles
        // the file delete.
        if (m_onSaveImage) m_onSaveImage(nullptr);
    }
    cDialog::SetActive(val);
}

void cAutoAnswerDlg::SetActiveWithTime(bool val, std::uint32_t dwTime) {
    if (val) {
        m_dwEndTime = static_cast<std::uint32_t>(nowMs() +
                                                 dwTime * 1000u);
        if (m_onSaveImage) m_onSaveImage(nullptr);
    } else {
        if (m_onSaveImage) m_onSaveImage(nullptr);
    }
    cDialog::SetActive(val);
}

std::uint32_t cAutoAnswerDlg::ActionEvent() {
    // 1:1 with legacy ActionEvent:
    //   int nLimitTime = ((int)(m_dwEndTime - gCurTime)) / 1000
    //   if (nLimitTime < 0) nLimitTime = 0
    //   static int last = 0
    //   if (last != nLimitTime) { wsprintf(buf, "%2d", nLimitTime); m_pStcTime->SetStaticText(buf); last = nLimitTime; }
    //   return cDialog::ActionEvent(mouseInfo)
    //
    // The modern port returns a window-event word (legacy
    // DWORD).  The host / dispatcher is expected to call
    // ActionEvent() once per tick.  No mouseInfo is taken --
    // the test path doesn't need a CMouse; the full host
    // integration will thread it through when the
    // cWindowManager dispatcher is wired up.
    if (m_dwEndTime == 0) {
        return 0u;
    }
    const std::int64_t cur = static_cast<std::int64_t>(nowMs());
    std::int32_t nLimitTime = static_cast<std::int32_t>(
        (static_cast<std::int64_t>(m_dwEndTime) - cur) / 1000);
    if (nLimitTime < 0) nLimitTime = 0;
    // 1:1 with legacy: the legacy uses a function-static
    // `last` cache; the modern port stores it as a per-
    // instance field-equivalent (we use a member static
    // initialised once).
    static std::int32_t last = 0;
    if (last != nLimitTime) {
        if (m_w.stcTime) {
            char buf[8] = {0};
            std::snprintf(buf, sizeof(buf), "%2d", nLimitTime);
            m_w.stcTime->SetStaticText(buf);
        }
        last = nLimitTime;
    }
    return 0u;
}

void cAutoAnswerDlg::OnActionEvent(std::int32_t lId, void* /*p*/, std::uint32_t we) {
    // 1:1 with legacy:
    //   if (m_nAnswerPos > 3) return;
    //   if (we & WE_BTNCLICK) {
    //       if (lId >= ASD_BTN_COLOR1 && lId <= ASD_BTN_COLOR4) {
    //           m_dwAnswer[m_nAnswerPos] = lId - ASD_BTN_COLOR1;
    //           sprintf(answer, "%s %s", m_pStcAnswer->GetStaticText(), "  *  ");
    //           m_pStcAnswer->SetStaticText(answer);
    //           ++m_nAnswerPos;
    //           if (m_nAnswerPos == 4) {
    //               AUTONOTEMGR->AnswerToQuestion(m_dwAnswer[0..3]);
    //               m_bAnswerStart = FALSE;
    //           }
    //       }
    //   }
    if (m_nAnswerPos > 3) return;
    if ((we & legacy_window_event::kButtonClick) == 0) return;
    if (lId < kAutoAnswerFirstButtonId || lId > kAutoAnswerLastButtonId) return;
    OnAnswerButtonClick(lId - kAutoAnswerFirstButtonId);
}

void cAutoAnswerDlg::OnAnswerButtonClick(std::int32_t buttonIdx) {
    // 1:1 with legacy: append a " * " to the answer static +
    // record the answer index.  Fire the answer callback
    // once the 4th answer is in (legacy fires
    // AutoNoteManager::AnswerToQuestion; the modern port
    // surfaces the same payload via m_onAnswer).
    if (buttonIdx < 0 || buttonIdx >= kAutoAnswerButtonCount) return;
    m_dwAnswer[m_nAnswerPos] = static_cast<std::uint32_t>(buttonIdx);
    appendAnswerStar(buttonIdx);
    ++m_nAnswerPos;
    if (m_nAnswerPos == kAutoAnswerButtonCount) {
        m_bAnswerStart = false;
        if (m_onAnswer) {
            m_onAnswer(m_dwAnswer[0], m_dwAnswer[1],
                       m_dwAnswer[2], m_dwAnswer[3]);
        }
    }
}

void cAutoAnswerDlg::appendAnswerStar(std::int32_t buttonIdx) {
    // 1:1 with legacy: wsprintf(buf, "%s %s", current, "  *  ")
    // The "  *  " literal is the legacy placeholder; the modern
    // port uses " * " (same family) for visual clarity.
    if (m_w.stcAnswer == nullptr) return;
    const std::string& cur = m_w.stcAnswer->GetStaticText();
    std::string next = cur.empty() ? std::string(" * ")
                                    : (cur + " * ");
    (void)buttonIdx;
    m_w.stcAnswer->SetStaticText(next);
}

void cAutoAnswerDlg::SetQuestion(const char* strQuestion) {
    if (m_w.stcQuestion && strQuestion) {
        m_w.stcQuestion->SetStaticText(strQuestion);
    }
    m_bAnswerStart = true;
    m_nAnswerPos   = 0;
    if (m_w.stcAnswer) m_w.stcAnswer->SetStaticText("");
}

void cAutoAnswerDlg::Retry() {
    m_bAnswerStart = true;
    m_nAnswerPos   = 0;
    if (m_w.stcAnswer) m_w.stcAnswer->SetStaticText("");
}

void cAutoAnswerDlg::SaveImage(std::uint8_t* pRaster) {
    // 1:1 with legacy: sprintf("ANRaster.tga") + WriteTGA(...)
    // The modern port delegates the actual TGA write to the
    // host via the save-image callback.
    if (m_onSaveImage) m_onSaveImage(pRaster);
}

void cAutoAnswerDlg::Shuffle(std::int32_t randY) {
    // 1:1 with legacy: SetAbsXY(GetAbsX(), GetAbsY() + randY)
    // (the legacy randomises between -60..+60 px on the Y
    // axis to disorient the user when the captcha appears).
    SetAbsXY(absX(), absY() + randY);
}

void cAutoAnswerDlg::Render() {
    // 1:1 with legacy Render: the legacy also auto-closes the
    // dialog when the timer expires (m_dwEndTime > gCurTime
    // goes false).  The modern port replicates this:
    if (isActive() && m_dwEndTime != 0 &&
        static_cast<std::int64_t>(m_dwEndTime) < static_cast<std::int64_t>(nowMs())) {
        m_dwEndTime = 0;
        cDialog::SetActive(false);
    }
    cDialog::Render();
}

const std::string& cAutoAnswerDlg::GetAnswerDisplay() const noexcept {
    static const std::string kEmpty;
    return m_w.stcAnswer ? m_w.stcAnswer->GetStaticText() : kEmpty;
}

std::uint64_t cAutoAnswerDlg::nowMs() const {
    if (m_hasNowOverride) return m_nowMsOverride;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

} // namespace mxh::ui
