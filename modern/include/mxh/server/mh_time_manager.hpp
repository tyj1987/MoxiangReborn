#pragma once
#include <cstdint>
namespace mxh::server {
inline constexpr std::uint32_t tick_per_day=86400000u, tick_per_hour=3600000u, tick_per_minute=60000u;
inline constexpr std::uint32_t day_per_year=360u, day_per_month=30u;
struct MhTimeState { std::uint32_t mh_date=0, mh_time=0, last_tick=0, cur_time=0, tick_time=0; bool initialized=false; };
void mh_time_init(MhTimeState&,std::uint32_t date,std::uint32_t time);
std::uint32_t mh_time_process(MhTimeState&,std::uint32_t platform_tick);
std::uint32_t mh_new_calc_cur_time(const MhTimeState&,std::uint32_t platform_tick);
void mh_date_parts(const MhTimeState&,std::uint8_t& year,std::uint8_t& month,std::uint8_t& day);
void mh_time_parts(const MhTimeState&,std::uint8_t& hour,std::uint8_t& minute);
}