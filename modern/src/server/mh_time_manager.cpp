#include "mxh/server/mh_time_manager.hpp"
namespace mxh::server {
void MhTimeManager::init(std::uint32_t mh_date, std::uint32_t mh_time) noexcept {
    m_mhDate = mh_date;
    m_mhTime = mh_time;
    m_first = true;
}
void MhTimeManager::process(std::uint32_t now_ms) noexcept {
    if(m_first) {
        m_lastMs = now_ms;
        m_first = false;
        return;
    }
    std::uint32_t delta = now_ms >= m_lastMs ? now_ms - m_lastMs : (now_ms + (0xFFFFFFFFu - m_lastMs) + 1u);
    if(delta == 0) return;
    m_lastMs = now_ms;
    m_mhTime += delta;
    while(m_mhTime >= MXH_MH_TICK_PER_DAY) {
        ++m_mhDate;
        m_mhTime -= MXH_MH_TICK_PER_DAY;
    }
}
void MhTimeManager::mh_date(std::uint8_t& year, std::uint8_t& month, std::uint8_t& day) const noexcept {
    year = static_cast<std::uint8_t>(m_mhDate / MXH_MH_DAY_PER_YEAR) + 1;
    std::uint32_t day_of_year = m_mhDate % MXH_MH_DAY_PER_YEAR;
    month = static_cast<std::uint8_t>(day_of_year / MXH_MH_DAY_PER_MONTH) + 1;
    day = static_cast<std::uint8_t>(day_of_year % MXH_MH_DAY_PER_MONTH) + 1;
}
void MhTimeManager::mh_time(std::uint8_t& hour, std::uint8_t& minute) const noexcept {
    hour = static_cast<std::uint8_t>(m_mhTime / MXH_MH_TICK_PER_HOUR);
    minute = static_cast<std::uint8_t>((m_mhTime % MXH_MH_TICK_PER_HOUR) / MXH_MH_TICK_PER_MINUTE);
}
}
