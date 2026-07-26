#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace mxh::server {
inline constexpr std::size_t mp_max=96u;
using MessageParser=std::function<void(std::uint32_t,const std::vector<std::uint8_t>&)>;
struct CommonParserTables { std::vector<MessageParser> server; std::vector<MessageParser> user; CommonParserTables(); bool set_server(std::size_t,MessageParser); bool set_user(std::size_t,MessageParser); bool invoke_server(std::size_t,std::uint32_t,const std::vector<std::uint8_t>&) const; bool invoke_user(std::size_t,std::uint32_t,const std::vector<std::uint8_t>&) const; };
}