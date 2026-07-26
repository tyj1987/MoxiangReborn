#pragma once
namespace mxh::server {
struct GmToolManagerState { bool initialized=false; };
inline void gm_tool_init(GmToolManagerState&s){s.initialized=true;}
inline void gm_tool_release(GmToolManagerState&s){s.initialized=false;}
inline void gm_tool_process_permit(const GmToolManagerState&) noexcept {}
}