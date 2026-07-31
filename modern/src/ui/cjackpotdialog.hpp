// cjackpotdialog.hpp -- modern 1:1 port of Moxiang cJackpotDialog
//   (lottery jackpot animated digit display).
//
// 1:1 port of legacy cJackpotDialog from
//   [Client]MH\cJackpotDialog.{h,cpp}. The legacy source ships 100%
//   commented out -- the dialog was disabled before release but the
//   documented (commented) API and constants are the canonical 1:1
//   surface we lock here.
//
// 1:1 surface (legacy, line-by-line):
//   * Constants:
//       NUMIMAGE_W=8, NUMIMAGE_H=14
//       BASIC_ANI_TIMELENGTH=2000 (ms; total animation phase)
//       BETWEEN_ANI_TIMELENGTH=500 (ms; inter-digit settle)
//       NUM_CHANGE_TIMELENGTH=100 (ms; per-tick digit roll)
//       DEFAULT_IMAGE=99 (blank sentinel)
//       NUM_COUNT=10, CIPHER_NUM=9
//   * StNumImage  { void* pImage=nullptr; uint32 dwW=0,dwH=0; }
//   * StCipherNum { uint32 dwNumber=0,dwRealCipherNum=0; bool bIsAni=false; }
//   * m_stNumImage[NUM_COUNT]   per-digit sprite metadata
//   * m_vPos[CIPHER_NUM]        per-digit absolute screen pos (VECTOR2)
//   * m_pBtnClose               close button (cButton*, 1:1 type tag only)
//   * m_dwTotalMoney            current displayed amount
//   * m_dwOldTotalMoney         last settled amount (change detect)
//   * m_dwTempMoney             snapshot of old amount (sequence ani)
//   * m_stCipherNum[CIPHER_NUM] per-digit display state
//   * m_dwAniStartTime          animation phase start (ms)
//   * m_dwNumChangeTime         last digit roll tick (ms)
//   * m_dwIntervalAniTime       last inter-digit settle (ms)
//   * m_dwMaxCipher             highest non-blank digit index
//   * m_dwCipherCount           next digit to settle in sequence
//   * m_bIsAnimationing         per-digit roll in progress
//   * m_bDoSequenceAni          true when amount increased
//
// 1:1 quirks:
//   * InitNumImage/ReleaseNumImage/Linking/SetNumImagePos/Render are
//     empty stubs (require cImage/cButton/GPU layer).
//   * MONEY_PER_MON is undefined in legacy; we define it as constexpr 1.
//   * Time source: legacy used global gCurTime; modern takes an explicit
//     now_ms parameter + a Clock callable (default steady_clock ms).
//   * cButton*/cImage* are forward-declared; no link-time dep on
//     mxh_render (the GPU layer is wired at Linking() in legacy, which
//     is a no-op stub here).

#pragma once

#include "mxh/ui/cDialog.hpp"
#include "mxh/render/math.hpp"

#include <cstdint>
#include <chrono>
#include <functional>

namespace mxh::ui {

class cButton;

using mxh::gx::VECTOR2;

inline constexpr std::uint32_t kNumImageW            = 8;
inline constexpr std::uint32_t kNumImageH            = 14;
inline constexpr std::uint32_t kBasicAniTimelength   = 2000;
inline constexpr std::uint32_t kBetweenAniTimelength = 500;
inline constexpr std::uint32_t kNumChangeTimelength  = 100;
inline constexpr std::uint32_t kDefaultImage         = 99;
inline constexpr std::uint32_t kNumCount             = 10;
inline constexpr std::uint32_t kCipherNum            = 9;
inline constexpr std::uint32_t kMoneyPerMon          = 1;

struct StNumImage {
    void*          pImage = nullptr;
    std::uint32_t  dwW    = 0;
    std::uint32_t  dwH    = 0;
};

struct StCipherNum {
    std::uint32_t  dwNumber         = 0;
    std::uint32_t  dwRealCipherNum  = 0;
    bool           bIsAni           = false;
};

class cJackpotDialog final : public cDialog {
public:
    using Clock = std::function<std::uint32_t()>;

    cJackpotDialog();
    explicit cJackpotDialog(Clock clock);
    ~cJackpotDialog() override;

    cJackpotDialog(const cJackpotDialog&) = delete;
    cJackpotDialog& operator=(const cJackpotDialog&) = delete;

    std::uint32_t TotalMoney()        const noexcept { return m_dwTotalMoney; }
    std::uint32_t OldTotalMoney()     const noexcept { return m_dwOldTotalMoney; }
    std::uint32_t TempMoney()         const noexcept { return m_dwTempMoney; }
    std::uint32_t MaxCipher()         const noexcept { return m_dwMaxCipher; }
    std::uint32_t CipherCount()       const noexcept { return m_dwCipherCount; }
    std::uint32_t AniStartTime()      const noexcept { return m_dwAniStartTime; }
    std::uint32_t NumChangeTime()     const noexcept { return m_dwNumChangeTime; }
    std::uint32_t IntervalAniTime()   const noexcept { return m_dwIntervalAniTime; }
    bool          IsAnimating()       const noexcept { return m_bIsAnimationing; }
    bool          DoSequenceAniFlag() const noexcept { return m_bDoSequenceAni; }

    const StCipherNum& CipherAt(std::uint32_t i) const noexcept { return m_stCipherNum[i]; }
    const VECTOR2&     CipherPos(std::uint32_t i) const noexcept { return m_vPos[i]; }
    const StNumImage&  NumImage(std::uint32_t i) const noexcept { return m_stNumImage[i]; }
    cButton*           CloseButton() const noexcept { return m_pBtnClose; }
    const Clock&       TimeSource()  const noexcept { return m_clock; }

    void SetTotalMoney(std::uint32_t v) noexcept     { m_dwTotalMoney = v; }
    void SetOldTotalMoney(std::uint32_t v) noexcept { m_dwOldTotalMoney = v; }
    void SetTempMoney(std::uint32_t v) noexcept      { m_dwTempMoney = v; }

    void Init();
    void Release();
    void ConvertCipherNum();
    bool IsNumChanged() noexcept;
    void InitForAni(std::uint32_t now_ms) noexcept;
    void InitForSequenceAni(std::uint32_t now_ms) noexcept;
    void DoAni(std::uint32_t now_ms) noexcept;
    void DoSequenceAni(std::uint32_t now_ms) noexcept;
    void Process(std::uint32_t now_ms) noexcept;

    void InitNumImage() noexcept;
    void ReleaseNumImage() noexcept;
    void Linking() noexcept;
    void SetNumImagePos() noexcept;
    void Render() override;

private:
    StNumImage  m_stNumImage[kNumCount]      {};
    VECTOR2     m_vPos[kCipherNum]           {};
    cButton*    m_pBtnClose                  = nullptr;

    std::uint32_t m_dwTotalMoney             = 0;
    std::uint32_t m_dwOldTotalMoney          = 0;
    std::uint32_t m_dwTempMoney              = 0;

    StCipherNum  m_stCipherNum[kCipherNum]   {};
    std::uint32_t m_dwAniStartTime           = 0;
    std::uint32_t m_dwNumChangeTime          = 0;
    std::uint32_t m_dwIntervalAniTime        = 0;
    std::uint32_t m_dwMaxCipher              = 0;
    std::uint32_t m_dwCipherCount            = 0;
    bool         m_bIsAnimationing           = false;
    bool         m_bDoSequenceAni            = false;

    Clock        m_clock;
};

}  // namespace mxh::ui
