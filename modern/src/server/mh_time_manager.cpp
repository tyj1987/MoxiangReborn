#include "mxh/server/mh_time_manager.hpp"
namespace mxh::server {
void mh_time_init(MhTimeState&s,std::uint32_t d,std::uint32_t t){s.mh_date=d;s.mh_time=t;s.last_tick=0;s.cur_time=0;s.tick_time=0;s.initialized=false;}
std::uint32_t mh_time_process(MhTimeState&s,std::uint32_t now){if(!s.initialized){s.last_tick=now;s.initialized=true;s.tick_time=0;return 0;}s.tick_time=now-s.last_tick;if(s.tick_time==0)return 0;s.last_tick=now;s.cur_time+=s.tick_time;s.mh_time+=s.tick_time;while(s.mh_time>=tick_per_day){++s.mh_date;s.mh_time-=tick_per_day;}return s.tick_time;}
std::uint32_t mh_new_calc_cur_time(const MhTimeState&s,std::uint32_t now){return s.cur_time+(now-s.last_tick);}
void mh_date_parts(const MhTimeState&s,std::uint8_t&y,std::uint8_t&m,std::uint8_t&d){y=static_cast<std::uint8_t>(s.mh_date/day_per_year)+1;m=static_cast<std::uint8_t>((s.mh_date-y)/day_per_month)+1;d=static_cast<std::uint8_t>(s.mh_date%day_per_month)+1;}
void mh_time_parts(const MhTimeState&s,std::uint8_t&h,std::uint8_t&m){h=static_cast<std::uint8_t>(s.mh_time/tick_per_hour);m=static_cast<std::uint8_t>((s.mh_time-h)/tick_per_minute);}
}
[[maybe_unused]] constexpr int mh_time_manager_translation_unit_anchor=0;