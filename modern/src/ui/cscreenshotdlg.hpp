// cscreenshotdlg.hpp — modern port of 墨香 CScreenShotDlg.
//
// 1:1 port of legacy `CScreenShotDlg` from
//   `墨香【源码】\[Client]MH\ScreenShotDlg.{h,cpp}`.
//
// The dialog shows two static text lines (server login time + play
// time) when a screenshot is taken, then auto-closes after a 2-second
// delay.  The modern port keeps that surface verbatim:
//
//   * Linking() builds two cStatic children
//     (m_sttime at (10,10), m_playtime at (10,27))
//   * SetActive(true, heroId) triggers ViewDatetime
//   * ViewDatetime formats "[server] YYYY-MM-DD HH:MM:SS" and
//     "[PLAYTIME] N Day HH:MM:SS" with seconds computed from
//     m_serverLoginDate / m_serverLoginTime
//   * SetServerTi stores the server SYSTEMTIME
//   * Render() auto-closes after 2 seconds
//
// The legacy code uses a couple of 1:1 quirks worth preserving:
//   * ViewDatetime takes a heroId (legacy: passed to GAMEIN->GetLoginTime)
//   * m_dwDelayTime stamps gCurTime + 2000 on ViewDatetime, and
//     Render() checks `if(m_dwDelayTime > gCurTime)` to extend the
//     show window indefinitely (the comment in the legacy file
//     explains: screenshots taken in quick succession extend the
//     delay).  When the timer expires the dialog auto-closes.

#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstdint>
#include <ctime>
#include <string>

namespace mxh::ui {

class cStatic;

// 1:1 with legacy eDelayTime = 2000.
inline constexpr std::int32_t kScreenShotDelayMs = 2000;

class cScreenShotDlg : public cDialog {
public:
    cScreenShotDlg();
    ~cScreenShotDlg() override;

    cScreenShotDlg(const cScreenShotDlg&) = delete;
    cScreenShotDlg& operator=(const cScreenShotDlg&) = delete;

    // 1:1 with legacy SetActive(BOOL, DWORD HeroID).
    void SetActive(bool val, std::uint32_t heroId = 0);

    // 1:1 with legacy Render.  Extends the show window while
    // m_dwDelayTime is in the future, auto-closes the dialog on
    // expiry, and forwards to cDialog::Render when m_bShow is true.
    void Render();

    // 1:1 with legacy Linking.  Creates the m_sttime + m_playtime
    // cStatic children.  Tests inject cStatic pointers via
    // SetChildWindowsForTest (avoiding the new'd cStatic path).
    void Linking();

    // 1:1 with legacy Setserverti.
    void SetServerTi(std::tm serverTime);

    // 1:1 with legacy ViewDatetime(DWORD HeroID).  Format strings:
    //   "[server] YYYY-MM-DD HH:MM:SS"
    //   "[PLAYTIME] N Day HH:MM:SS"
    // where:
    //   * N    = (currentTotal - loginTotal) / 1000 / 60 / 60 / 24
    //   * HH   = (currentTotal - loginTotal) / 1000 / 60 / 60 % 24
    //   * MM   = (currentTotal - loginTotal) / 1000 / 60 % 60
    //   * SS   = (currentTotal - loginTotal) / 1000 % 60
    // and `currentTotal = currentDate * MS_PER_DAY + currentTime`
    // with MS_PER_DAY = 1000*60*60*24 (legacy 1:1 quirk).
    void ViewDatetime(std::uint32_t heroId = 0);

    // Test hook -- inject cStatic pointers + server/login times +
    // a server name (max 4 chars per _HK_LOCAL_, max 32 chars
    // otherwise).
    struct ChildWindows {
        cStatic* stTime    = nullptr;
        cStatic* playTime  = nullptr;
    };
    void SetChildWindowsForTest(const ChildWindows& w) { m_childWindows = w; }

    // Test hook -- override the "current date / time" the dialog
    // reads from `MHTIMEMGR->GetMHDate() / GetMHTime()`.  The
    // legacy code uses seconds-since-epoch style ints; we model
    // them as separate `date` (days) and `time` (seconds) ints to
    // match the legacy arithmetic.
    void SetNowForTest(std::int32_t date, std::int32_t time) {
        m_nowDate = date;
        m_nowTime = time;
    }

    // Test hook -- override the "client login date / time" (legacy
    // GAMEIN->GetClientLoginTime(dateOut, timeOut)).
    void SetClientLoginForTest(std::int32_t date, std::int32_t time) {
        m_loginDate = date;
        m_loginTime = time;
    }

    // Test hook -- set the server set name (legacy g_ServerSetName
    // / GAMERESRCMNGR->GetServerSetName()).
    void SetServerNameForTest(const std::string& name) { m_serverName = name; }

    // Test introspection.
    bool             isShow()       const noexcept { return m_bShow; }
    std::uint32_t    delayTime()    const noexcept { return m_dwDelayTime; }
    const std::tm&   serverTime()   const noexcept { return m_serverTime; }
    cStatic*         stTime()       const noexcept { return m_sttime; }
    cStatic*         playTime()     const noexcept { return m_playtime; }

    // Test hook -- override the global now-millis (legacy gCurTime)
    // so Render()'s m_dwDelayTime > gCurTime check is testable.
    void SetNowMsForTest(std::uint64_t now) { m_nowMs = now; }
    std::uint64_t nowMsForTest() const noexcept { return m_nowMs; }

private:
    ChildWindows m_childWindows;
    cStatic*     m_sttime    = nullptr;  // 1:1 with legacy m_sttime
    cStatic*     m_playtime  = nullptr;  // 1:1 with legacy m_playtime
    bool         m_bShow       = false;
    std::uint32_t m_dwDelayTime = 0;
    std::tm       m_serverTime{};

    std::int32_t m_nowDate  = 0;
    std::int32_t m_nowTime  = 0;
    std::int32_t m_loginDate = 0;
    std::int32_t m_loginTime = 0;
    std::string  m_serverName;

    std::uint64_t m_nowMs = 0;
};

} // namespace mxh::ui
