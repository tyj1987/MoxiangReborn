// cautoanswerdlg.hpp — modern port of 墨香 CAutoAnswerDlg (auto-reply quiz).
//
// 1:1 port of legacy `CAutoAnswerDlg` from
//   `墨香【源码】\[Client]MH\AutoAnswerDlg.{h,cpp}`.
//
// The auto-answer dialog is a 4-button "tap the colors in the
// right order" mini-game.  The legacy dialog keeps the answer
// order in m_dwAnswer[4] + an answer cursor (m_nAnswerPos), and
// fires the host's AutoNoteManager::AnswerToQuestion() once all
// 4 colors are tapped.
//
// 1:1 quirks preserved:
//   * SetActive(TRUE) / SetActiveWithTime(TRUE, dwTime) stamp
//     m_dwEndTime = gCurTime + (120 or dwTime) * 1000 — a 120s
//     timer (legacy default) that the host's per-tick Render()
//     reads to update the "time left" static.
//   * m_bAnswerStart + m_nAnswerPos gate the OnActionEvent
//     answer-entry state machine.
//   * SetQuestion() / Retry() reset the answer cursor but
//     keep m_dwEndTime intact (legacy).
//   * SaveImage() writes a 128x32 TGA raster of the captcha to
//     disk; the modern port keeps the API surface but defers
//     the actual TGA write (a WriteTGA helper is provided via
//     the host's image layer; the modern port just stores the
//     raster pointer for the host to consume).

#pragma once

#include "cDialog.hpp"
#include "cbutton.hpp"
#include "cstatic.hpp"
#include "ctextarea.hpp"
#include "legacy_window_event.hpp"

#include <cstdint>
#include <cstring>
#include <functional>

namespace mxh::ui {

// 1:1 with legacy CAutoAnswerDlg: 4 colored buttons.
inline constexpr std::int32_t kAutoAnswerButtonCount = 4;
inline constexpr std::int32_t kAutoAnswerFirstButtonId = 1695;
inline constexpr std::int32_t kAutoAnswerLastButtonId = 1698;

class cImageSelf;   // forward-declared; the modern port stores
                   // the image as void* (1:1 with cImageSelf*).

class cAutoAnswerDlg : public cDialog {
public:
    cAutoAnswerDlg();
    ~cAutoAnswerDlg() override;

    cAutoAnswerDlg(const cAutoAnswerDlg&) = delete;
    cAutoAnswerDlg& operator=(const cAutoAnswerDlg&) = delete;

    // 1:1 with legacy Linking.  Modern port reads the four
    // colored buttons + desc / question / answer / time statics
    // from the injected child windows (SetChildWindowsForTest).
    void Linking();

    // 1:1 with legacy SetActive(BOOL).  When activating the
    // dialog for the first time, stamps m_dwEndTime = gCurTime +
    // 120 * 1000 (120 second timer).  When deactivating, calls
    // the modern equivalent of DeleteFile("ANRaster.tga") via
    // a callback (the host is responsible for the file delete;
    // the modern port surfaces this through the same callback
    // used by SetActiveWithTime).
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy SetActiveWithTime(BOOL, DWORD).  Stamps
    // m_dwEndTime = gCurTime + dwTime * 1000.  Used by the host
    // when it wants a non-default timer (e.g. tests).
    void SetActiveWithTime(bool val, std::uint32_t dwTime);

    // 1:1 with legacy ActionEvent.  Updates the time-left
    // static once per second (gated by a static cache in the
    // legacy) then forwards to cDialog::ActionEvent.
    std::uint32_t ActionEvent();

    // 1:1 with legacy OnActionEvent.  Handles the
    // WE_BTNCLICK event on the four colored buttons: appends
    // the button index to m_dwAnswer[m_nAnswerPos], updates the
    // "answer" static with a " * " string per entry, and
    // invokes the host's AnswerToQuestion() callback once the
    // 4th answer is recorded.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // 1:1 with legacy SetQuestion.  Sets the question static
    // and resets the answer cursor (m_bAnswerStart = TRUE,
    // m_nAnswerPos = 0, m_pStcAnswer->SetStaticText("")).
    void SetQuestion(const char* strQuestion);

    // 1:1 with legacy Retry.  Like SetQuestion but does NOT
    // touch the question static (just resets the answer
    // cursor).
    void Retry();

    // 1:1 with legacy SaveImage.  The modern port stores the
    // raster pointer and surfaces a write callback for the
    // host (the actual TGA encoder is host-side).
    void SaveImage(std::uint8_t* pRaster);
    using SaveImageCallback = std::function<void(std::uint8_t* raster)>;
    void SetOnSaveImage(SaveImageCallback cb) noexcept { m_onSaveImage = std::move(cb); }

    // 1:1 with legacy Shuffle.  The legacy Shuffle jitters the
    // dialog's Y position to nudge the user into paying
    // attention; the modern port preserves this as
    // Shuffle(randY) and exposes the absolute-Y delta through
    // SetAbsXY.
    void Shuffle(std::int32_t randY);

    // 1:1 with legacy Render.  Decrements the timer + auto-
    // closes the dialog when the timer expires.  The modern
    // port uses a host-provided clock (default: steady_clock
    // in ms).  Override with SetNowMsForTest.
    void Render() override;
    void SetNowMsForTest(std::uint64_t now_ms) noexcept { m_nowMsOverride = now_ms; m_hasNowOverride = true; }
    std::uint64_t GetNowMsForTest() const noexcept      { return m_nowMsOverride; }

    // 1:1 with legacy SetActiveTestClient (a no-op in the
    // legacy engine; kept for 1:1 source compatibility).
    void SetActiveTestClient() noexcept { /* no-op */ }

    // 1:1 with legacy OnActionEvent: 4 answer-tap buttons map
    // to the 4 cPushupButton/cButton children.  Modern port
    // enumerates the answers through a callback so the host
    // can forward to AutoNoteManager::AnswerToQuestion().
    using AnswerCallback = std::function<void(std::uint32_t a0,
                                              std::uint32_t a1,
                                              std::uint32_t a2,
                                              std::uint32_t a3)>;
    void SetOnAnswer(AnswerCallback cb) noexcept { m_onAnswer = std::move(cb); }

    // 1:1 with legacy OnActionEvent WE_BTNCLICK mapping.
    // The button ids are legacy WindowIDs.h values 1695..1698
    // (ASD_BTN_COLOR1..4).  The host (or tests) forward
    // button-click events to OnActionEvent; the dialog does
    // the answer-entry bookkeeping and fires the callback.
    void OnAnswerButtonClick(std::int32_t buttonIdx);

    // Test accessors.
    bool IsAnswerStart() const noexcept      { return m_bAnswerStart; }
    std::int32_t GetAnswerPos() const noexcept { return m_nAnswerPos; }
    std::uint32_t GetAnswer(std::int32_t idx) const noexcept {
        return (idx >= 0 && idx < kAutoAnswerButtonCount) ? m_dwAnswer[idx] : 0u;
    }
    std::uint64_t GetEndTime() const noexcept { return m_dwEndTime; }
    const std::string& GetAnswerDisplay() const noexcept;

    // Test hook -- inject child windows.
    struct ChildWindows {
        cTextArea*  textAreaDesc = nullptr;
        cStatic*    stcQuestion  = nullptr;
        cStatic*    stcAnswer    = nullptr;
        cStatic*    stcTime      = nullptr;
        cButton*    btnColor[kAutoAnswerButtonCount] = {};
    };
    void SetChildWindowsForTest(const ChildWindows& w) noexcept { m_w = w; }

private:
    std::uint32_t m_dwEndTime   = 0;     // ms timestamp; 0 = inactive
    std::int32_t  m_v2BtnPosX[kAutoAnswerButtonCount] = {};
    std::int32_t  m_v2BtnPosY[kAutoAnswerButtonCount] = {};
    std::uint32_t m_dwOriginalPos[kAutoAnswerButtonCount] = {};
    bool          m_bAnswerStart = false;
    std::uint32_t m_dwAnswer[kAutoAnswerButtonCount] = {};
    std::int32_t  m_nAnswerPos   = 0;
    ChildWindows  m_w{};

    AnswerCallback    m_onAnswer;
    SaveImageCallback m_onSaveImage;

    // Test clock override (default: get the real wall clock
    // in ms).  Set by SetNowMsForTest / used by Render's
    // timer-expiry check.
    std::uint64_t m_nowMsOverride = 0;
    bool          m_hasNowOverride = false;

    std::uint64_t nowMs() const;
    // Resets the visible "answer" static by appending " * "
    // for the new entry.
    void appendAnswerStar(std::int32_t buttonIdx);
};

} // namespace mxh::ui
