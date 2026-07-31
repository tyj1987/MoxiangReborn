#pragma once
#include <cstdint>
namespace mxh::server {
// 1:1 port of legacy [Server]MurimNet/MHTimeManager.cpp numeric constants.
// DAY_PER_YEAR=360 / DAY_PER_MONTH=30 is the in-game calendar where each
// year is 360 days and each month is 30 days, all 24 hours.
inline constexpr std::uint32_t MXH_MH_TICK_PER_DAY    = 8640000u;   // legacy value: 8640000 ms (one in-game day)
inline constexpr std::uint32_t MXH_MH_TICK_PER_HOUR   = 3600000u;   // 60 * 60 * 1000
inline constexpr std::uint32_t MXH_MH_TICK_PER_MINUTE = 60000u;     // 60 * 1000
inline constexpr std::uint32_t MXH_MH_DAY_PER_YEAR    = 360u;
inline constexpr std::uint32_t MXH_MH_DAY_PER_MONTH   = 30u;
// Murim in-game time manager. Tracks MHDate (whole days) and MHTime
// (milliseconds within the current day). The Process(now_ms) entry point
// accepts a steady-clock-style tick in milliseconds; on the first call it
// seeds the baseline, on subsequent calls it advances MHTime by the delta
// and rolls over into a new day whenever MHTime crosses TICK_PER_DAY.
class MhTimeManager final {
public:
    void init(std::uint32_t mh_date, std::uint32_t mh_time) noexcept;
    void process(std::uint32_t now_ms) noexcept;
    std::uint32_t mh_date() const noexcept { return m_mhDate; }
    std::uint32_t mh_time() const noexcept { return m_mhTime; }
    void mh_date(std::uint8_t& year, std::uint8_t& month, std::uint8_t& day) const noexcept;
    void mh_time(std::uint8_t& hour, std::uint8_t& minute) const noexcept;
    void reset_for_test() noexcept { m_mhDate = 0; m_mhTime = 0; m_lastMs = 0; m_first = true; }
private:
    std::uint32_t m_mhDate = 0;
    std::uint32_t m_mhTime = 0;
    std::uint32_t m_lastMs = 0;
    bool m_first = true;
};
}
