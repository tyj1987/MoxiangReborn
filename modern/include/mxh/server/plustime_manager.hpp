#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace mxh::server {
struct PlusTimeInfo { std::uint16_t index=0,event_index=0,start_day=0,start_hour=0,start_minute=0,end_day=0,end_hour=0,end_minute=0;std::uint32_t rate=0;std::string title,context; };
enum class PlusTimeActionKind : std::uint8_t { on, off, notify_on, notify_off, reset };
struct PlusTimeAction { PlusTimeActionKind kind{}; std::uint16_t event_index=0; std::uint16_t rate=0; std::string title,context; };
struct PlusTimeState { std::vector<PlusTimeInfo> entries; std::vector<std::uint16_t> applied; bool toggle_on=true; std::string context; };
void plustime_reset(PlusTimeState&);
std::vector<PlusTimeAction> plustime_process(PlusTimeState&,std::uint16_t day,std::uint16_t hour,std::uint16_t minute);
std::vector<PlusTimeAction> plustime_off(PlusTimeState&);
std::vector<PlusTimeAction> plustime_connecting(const PlusTimeState&);
}