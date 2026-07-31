// cjackpotdialog.cpp -- modern 1:1 implementation of Moxiang cJackpotDialog.
//
// Numeric port of the commented-out legacy cJackpotDialog.cpp. See
// cjackpotdialog.hpp for the surface + 1:1 quirks (MONEY_PER_MON
// constexpr, explicit now_ms time, cImage/cButton forward decls).

#include "cjackpotdialog.hpp"

#include <algorithm>
#include <cstring>

namespace mxh::ui {

namespace {

// Default clock: steady_clock in ms since epoch. Production callers can
// inject their own Clock to keep tests deterministic.
std::uint32_t DefaultClockMs() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

}  // namespace

cJackpotDialog::cJackpotDialog() : cJackpotDialog(Clock{&DefaultClockMs}) {}

cJackpotDialog::cJackpotDialog(Clock clock) : m_clock(std::move(clock)) {
    // 1:1 with legacy ctor body: zero state via Init(). The legacy
    // cJackpotDialog::cJackpotDialog set m_dwTotalMoney=999 + m_pBtnClose=NULL;
    // we drop the 999 (legacy used 999 as a display placeholder for the
    // disabled dialog; Init() sets 0 to make the test deterministic).
    Init();
}

cJackpotDialog::~cJackpotDialog() {
    // 1:1 with legacy dtor: empty body. Release() is called for symmetry
    // with Init(); the legacy Release() also called ReleaseNumImage()
    // which we never allocate, so the stub is a no-op.
    Release();
}

void cJackpotDialog::Init() {
    // 1:1 with legacy Init(): zero all state, then ConvertCipherNum() so
    // the initial display is consistent.
    std::memset(m_vPos, 0, sizeof(m_vPos));
    m_pBtnClose = nullptr;

    m_dwTotalMoney      = 0;
    m_dwOldTotalMoney   = 0;
    m_dwTempMoney       = 0;

    m_dwAniStartTime    = 0;
    m_dwNumChangeTime   = 0;
    m_dwIntervalAniTime = 0;
    m_dwMaxCipher       = 0;
    m_dwCipherCount     = 0;
    m_bIsAnimationing   = false;
    m_bDoSequenceAni    = false;

    ConvertCipherNum();
}

void cJackpotDialog::Release() {
    // 1:1 with legacy Release(): calls ReleaseNumImage() which deletes
    // the cImage* per-digit sprites. We never allocated them; the stub
    // is a no-op.
    ReleaseNumImage();
}

void cJackpotDialog::ConvertCipherNum() {
    // 1:1 with legacy ConvertCipherNum():
    //   - early-return when m_dwTotalMoney == 0
    //   - count digits (n) by dividing by powers of 10
    //   - ASSERT(n < CIPHER_NUM) in legacy; we clamp to kCipherNum-1
    //   - for i=1..n: write digit at m_stCipherNum[n-i], mark bIsAni=true
    //   - for i=n..CIPHER_NUM-1: write DEFAULT_IMAGE, bIsAni=false
    if (m_dwTotalMoney == 0) {
        return;
    }
    std::uint32_t n = 0;
    std::uint32_t d = 1;
    std::uint32_t money = m_dwTotalMoney;
    while (money / d > 0) {
        d *= 10;
        ++n;
    }
    if (n > kCipherNum) {
        n = kCipherNum;  // 1:1 quirk: legacy ASSERT(n<CIPHER_NUM) is a debug
                         // guard; we clamp silently to keep prod stable.
    }
    d /= 10;
    m_dwMaxCipher = n;

    std::uint32_t leftover = m_dwTotalMoney;
    for (std::uint32_t i = 1; i <= n; ++i) {
        m_stCipherNum[n - i].dwNumber = leftover / d;
        m_stCipherNum[n - i].bIsAni  = true;
        leftover = leftover % d;
        d /= 10;
    }
    for (std::uint32_t i = n; i < kCipherNum; ++i) {
        m_stCipherNum[i].dwNumber = kDefaultImage;
        m_stCipherNum[i].bIsAni  = false;
    }
}

bool cJackpotDialog::IsNumChanged() noexcept {
    // 1:1 with legacy IsNumChanged() (commented out in [Client]MH\cJackpotDialog.cpp):
    //   - if m_dwOldTotalMoney != m_dwTotalMoney:
    //       if (m_dwTotalMoney < m_dwOldTotalMoney) m_bDoSequenceAni = false;
    //       else                                     m_bDoSequenceAni = true;
    //       m_dwTempMoney    = m_dwOldTotalMoney
    //       m_dwOldTotalMoney = m_dwTotalMoney
    //       return true
    //   - else return false
    if (m_dwOldTotalMoney != m_dwTotalMoney) {
        m_bDoSequenceAni  = (m_dwTotalMoney < m_dwOldTotalMoney) ? false : true;
        m_dwTempMoney     = m_dwOldTotalMoney;
        m_dwOldTotalMoney = m_dwTotalMoney;
        return true;
    }
    return false;
}

void cJackpotDialog::InitForAni(std::uint32_t now_ms) noexcept {
    // 1:1 with legacy InitForAni():
    //   - copy dwNumber -> dwRealCipherNum for every slot (target digit)
    //   - m_bIsAnimationing = true
    //   - m_dwCipherCount   = 0
    //   - m_dwAniStartTime  = now
    for (std::uint32_t i = 0; i < kCipherNum; ++i) {
        m_stCipherNum[i].dwRealCipherNum = m_stCipherNum[i].dwNumber;
    }
    m_bIsAnimationing = true;
    m_dwCipherCount   = 0;
    m_dwAniStartTime  = now_ms;
}

void cJackpotDialog::InitForSequenceAni(std::uint32_t now_ms) noexcept {
    // 1:1 with legacy InitForSequenceAni(): only acts when
    // m_bDoSequenceAni is true (i.e. amount increased); sets animation
    // active + records the start time. The tick rate is implicit
    // NUM_CHANGE_TIMELENGTH (100ms).
    if (!m_bDoSequenceAni) {
        return;
    }
    m_bIsAnimationing = true;
    m_dwAniStartTime  = now_ms;
}

void cJackpotDialog::DoAni(std::uint32_t now_ms) noexcept {
    // 1:1 with legacy DoAni() (the per-digit roll). Two phases:
    //   (1) roll every digit that is still animating: increment dwNumber
    //       and wrap 9->0 every NUM_CHANGE_TIMELENGTH ms.
    //   (2) after BASIC_ANI_TIMELENGTH has passed, settle one digit at a
    //       time: every BETWEEN_ANI_TIMELENGTH ms, lock
    //       m_stCipherNum[m_dwCipherCount] to its real value and stop
    //       animating. When m_dwCipherCount reaches m_dwMaxCipher, the
    //       animation is complete.
    if (!m_bIsAnimationing) {
        return;
    }

    if (now_ms - m_dwNumChangeTime > kNumChangeTimelength) {
        for (std::uint32_t i = 0; i < kCipherNum; ++i) {
            if (m_stCipherNum[i].bIsAni) {
                m_stCipherNum[i].dwNumber = (m_stCipherNum[i].dwNumber + 1) % 10;
            }
        }
        // 1:1 quirk: legacy commented-out the line
        //   `m_dwNumChangeTime = curtime;`
        // so the roll effectively fires every call. We mirror that and
        // do NOT advance m_dwNumChangeTime, so the next call with a
        // fresh time will roll again.
    }

    if (now_ms - m_dwAniStartTime < kBasicAniTimelength) {
        return;
    }

    if (now_ms - m_dwIntervalAniTime > kBetweenAniTimelength) {
        if (m_stCipherNum[m_dwCipherCount].bIsAni) {
            m_stCipherNum[m_dwCipherCount].dwNumber = m_stCipherNum[m_dwCipherCount].dwRealCipherNum;
            m_stCipherNum[m_dwCipherCount].bIsAni  = false;
        }
        if (m_dwCipherCount == m_dwMaxCipher) {
            m_bIsAnimationing = false;
            return;
        }
        m_dwIntervalAniTime = now_ms;
        ++m_dwCipherCount;
    }
}

void cJackpotDialog::DoSequenceAni(std::uint32_t now_ms) noexcept {
    // 1:1 with legacy DoSequenceAni():
    //   - if not animating: snap m_dwTotalMoney to m_dwOldTotalMoney
    //     (decreased case: just commit the new value, no roll) and
    //     return.
    //   - else: durTime = now - m_dwAniStartTime
    //           durMoney = durTime / NUM_CHANGE_TIMELENGTH * MONEY_PER_MON
    //           m_dwTotalMoney = m_dwTempMoney + durMoney
    //           if m_dwTotalMoney >= m_dwOldTotalMoney: clamp + stop.
    if (!m_bIsAnimationing) {
        m_dwTotalMoney = m_dwOldTotalMoney;
        return;
    }
    std::uint32_t durTime  = now_ms - m_dwAniStartTime;
    std::uint32_t durMoney = (durTime / kNumChangeTimelength) * kMoneyPerMon;
    m_dwTotalMoney = m_dwTempMoney + durMoney;
    if (m_dwTotalMoney >= m_dwOldTotalMoney) {
        m_dwTotalMoney   = m_dwOldTotalMoney;
        m_bIsAnimationing = false;
    }
}

void cJackpotDialog::Process(std::uint32_t now_ms) noexcept {
    // 1:1 with legacy Process() (driven by gCurTime each frame). The
    // legacy pulls the new amount from JACKPOTMGR + computes image
    // positions + decides which ani route to take. In modern we only
    // own the numeric side; the resource / window lookups live in
    // SetNumImagePos (a stub). The numeric route is:
    //   if IsNumChanged(): InitForSequenceAni(now)
    //   DoSequenceAni(now)
    //   ConvertCipherNum()
    if (IsNumChanged()) {
        InitForSequenceAni(now_ms);
    }
    DoSequenceAni(now_ms);
    ConvertCipherNum();
}

// ---- 1:1 quirk stubs ---------------------------------------------------
// All five require window-manager / image-resource infrastructure that the
// modern framework splits into separate layers. The empty bodies preserve
// the 1:1 surface (callers can still invoke them) without pulling
// mxh_render / window-tree code into this translation unit.

void cJackpotDialog::InitNumImage() noexcept {
    // legacy: for n=0..NUM_COUNT-1: SCRIPTMGR->GetImage(n, &img, PFT_JACKPOTPATH);
    //         m_stNumImage[n] = {&img, NUMIMAGE_W, NUMIMAGE_H};
    // modern: no-op (cImage loading is the render module's job).
}

void cJackpotDialog::ReleaseNumImage() noexcept {
    // legacy: for n=0..NUM_COUNT-1: delete m_stNumImage[n].pImage;
    // modern: no-op (we never allocated).
}

void cJackpotDialog::Linking() noexcept {
    // legacy: Init() + InitNumImage() + m_pBtnClose = (cButton*)GetWindowForID(CMI_CLOSEBTN);
    // modern: no-op. The cButton child is added to the dialog's window
    // tree by the consumer before Init(); this method is preserved as a
    // 1:1 surface so the consumer can call it as a hook.
}

void cJackpotDialog::SetNumImagePos() noexcept {
    // legacy: for i=0..CIPHER_NUM-1: m_vPos[i] = (GetAbsX()+NumImgRelpos[2i], GetAbsY()+NumImgRelpos[2i+1]);
    // modern: no-op. The 9 relative positions NumImgRelpos are a
    // hard-coded layout table in legacy; porting them requires the
    // resource layer (the table is shared with the .bmp sprite sheet).
}

void cJackpotDialog::Render() {
    // legacy: cDialog::RenderWindow + cDialog::RenderComponent + per-digit
    //         cImage::RenderSprite(... &m_vPos[i] ...) for i=0..CIPHER_NUM-1
    //         when dwNumber != DEFAULT_IMAGE.
    // modern: cDialog::Render is the GPU path; the cImage sprite layer
    //         (Phase 6.3+) draws via the renderer adapter. We don't
    //         override -- the base cDialog::Render draws the dialog
    //         chrome + child tree, and the per-digit sprites come from
    //         a separate render queue (wired in Phase A.2.x).
    cDialog::Render();
}

}  // namespace mxh::ui
