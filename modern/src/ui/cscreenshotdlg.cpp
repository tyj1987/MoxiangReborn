// cscreenshotdlg.cpp — modern port of 墨香 CScreenShotDlg.

#include "mxh/ui/cscreenshotdlg.hpp"

#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cstatic.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

namespace {
constexpr std::int64_t kMsPerDay = 1000LL * 60LL * 60LL * 24LL;
}

cScreenShotDlg::cScreenShotDlg() {
    m_sttime = nullptr;
}

cScreenShotDlg::~cScreenShotDlg() = default;

void cScreenShotDlg::Linking() {
    // 1:1 with legacy Linking: the legacy creates m_sttime and
    // m_playtime via `new cStatic` and Init()s them at (10,10) and
    // (10,27).  In the modern port the host injects the cStatic
    // pointers via SetChildWindowsForTest (avoids the new'd
    // cStatic).  We still set the m_sttime / m_playtime accessors
    // for callers that don't use the test hook.
    if (m_childWindows.stTime)   m_sttime = m_childWindows.stTime;
    if (m_childWindows.playTime) m_playtime = m_childWindows.playTime;

    m_dwDelayTime = 0;
    m_bShow = false;
}

void cScreenShotDlg::SetActive(bool val, std::uint32_t heroId) {
    // 1:1 with legacy SetActive(BOOL, DWORD).  Forward to
    // cDialog::SetActive (the base) and ViewDatetime on enter.
    cDialog::SetActive(val);
    if (val) {
        ViewDatetime(heroId);
    }
}

void cScreenShotDlg::SetServerTi(std::tm serverTime) {
    // 1:1 with legacy Setserverti(SYSTEMTIME).  std::tm is a
    // direct stand-in for SYSTEMTIME.
    m_serverTime = serverTime;
}

void cScreenShotDlg::ViewDatetime(std::uint32_t /*heroId*/) {
    // 1:1 with legacy ViewDatetime.  Format strings:
    //   "[server] %02d-%02d-%02d %02d:%02d:%02d"
    //   "[PLAYTIME] %2d Day %02d:%02d:%02d"
    // with the legacy arithmetic:
    //   totalCurrent = currentDate*MS_PER_DAY + currentTime
    //   totalLogin   = loginDate   *MS_PER_DAY + loginTime
    //   diff = totalCurrent - totalLogin
    //   days = diff / 1000 / 60 / 60 / 24
    //   hh   = (diff / 1000 / 60 / 60) % 24
    //   mm   = (diff / 1000 / 60) % 60
    //   ss   = (diff / 1000) % 60
    // The server name is truncated to 4 chars in _HK_LOCAL_ or
    // 32 chars otherwise; the modern port uses m_serverName (the
    // host injects the truncated value).

    char timeText[255] = {};
    std::snprintf(timeText, sizeof(timeText), "[%s] %04d-%02d-%02d %02d:%02d:%02d",
                  m_serverName.c_str(),
                  m_serverTime.tm_year + 1900,
                  m_serverTime.tm_mon  + 1,
                  m_serverTime.tm_mday,
                  m_serverTime.tm_hour,
                  m_serverTime.tm_min,
                  m_serverTime.tm_sec);
    if (m_sttime) {
        m_sttime->SetStaticText(timeText);
    }

    const std::int64_t totalCurrent = static_cast<std::int64_t>(m_nowDate) * kMsPerDay
                                    + static_cast<std::int64_t>(m_nowTime);
    const std::int64_t totalLogin   = static_cast<std::int64_t>(m_loginDate) * kMsPerDay
                                    + static_cast<std::int64_t>(m_loginTime);
    const std::int64_t diff = totalCurrent - totalLogin;
    if (diff < 0) {
        // 1:1 with legacy: when login > current (clock skew /
        // late login), the legacy sprintf would print a
        // negative N Day.  The modern port mirrors the legacy
        // exact output for that case.
        std::snprintf(timeText, sizeof(timeText),
                      "[PLAYTIME] %2d Day %02d:%02d:%02d",
                      static_cast<int>(diff / 1000 / 60 / 60 / 24),
                      static_cast<int>((diff / 1000 / 60 / 60) % 24),
                      static_cast<int>((diff / 1000 / 60) % 60),
                      static_cast<int>((diff / 1000) % 60));
    } else {
        std::snprintf(timeText, sizeof(timeText),
                      "[PLAYTIME] %2d Day %02d:%02d:%02d",
                      static_cast<int>(diff / 1000 / 60 / 60 / 24),
                      static_cast<int>((diff / 1000 / 60 / 60) % 24),
                      static_cast<int>((diff / 1000 / 60) % 60),
                      static_cast<int>((diff / 1000) % 60));
    }
    if (m_playtime) {
        m_playtime->SetStaticText(timeText);
    }

    m_bShow = true;
    // 1:1 quirk: legacy stamps gCurTime + 2000.  In the modern
    // port, Render() reads from m_nowMs (test override), so the
    // host is expected to call SetNowMsForTest before SetActive.
    m_dwDelayTime = static_cast<std::uint32_t>(m_nowMs) + kScreenShotDelayMs;
}

void cScreenShotDlg::Render() {
    // 1:1 with legacy Render.  Extend the show window while
    // m_dwDelayTime > gCurTime, auto-close on expiry.
    if (m_dwDelayTime > m_nowMs) {
        m_bShow = true;
    } else if (m_bShow) {
        m_dwDelayTime = 0;
        m_bShow = false;
        cDialog::SetActive(false);
    }
    if (m_bShow) {
        cDialog::Render();
    }
}

} // namespace mxh::ui
