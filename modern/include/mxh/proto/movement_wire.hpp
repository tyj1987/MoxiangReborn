#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <vector>

// Modern timed-movement extension. Existing four-byte movement messages and
// original reference headers are unchanged. This codec does not enable routing
// or authorize a command: the server must bind epoch/sequence to the live player.
namespace mxh::proto::movement {
inline constexpr std::uint8_t version = 1;
inline constexpr std::size_t max_points = 15;
inline constexpr std::size_t command_header_size = 24;
inline constexpr std::size_t state_header_size = 52;
struct Point { std::uint16_t x = 0, z = 0; };
enum class CommandKind : std::uint8_t { Route = 1, Stop = 2 };
enum class StateKind : std::uint8_t { Started = 1, Stopped = 2, Corrected = 3, Snapshot = 4 };

struct Command {
    CommandKind kind = CommandKind::Route;
    std::uint64_t epoch = 0, sequence = 0;
    std::uint8_t count = 0;
    std::array<Point,max_points> points{};
};
struct State {
    StateKind kind = StateKind::Snapshot;
    std::uint64_t epoch = 0, command_sequence = 0, state_sequence = 0, server_time_ms = 0;
    float x = 0, z = 0, speed = 0;
    std::uint8_t count = 0;
    std::array<Point,max_points> points{};
};

namespace detail {
inline bool valid_points(std::span<const Point> points) noexcept {
    for (const auto p : points) if (p.x >= 51200 || p.z >= 51200) return false;
    return true;
}
inline void put(std::vector<std::uint8_t>& bytes, std::uint64_t value, std::size_t width) {
    for (std::size_t i=0;i<width;++i) bytes.push_back(static_cast<std::uint8_t>(value >> (8*i)));
}
inline std::uint64_t get(std::span<const std::uint8_t> bytes, std::size_t offset, std::size_t width) noexcept {
    std::uint64_t result=0;
    for (std::size_t i=0;i<width;++i) result |= std::uint64_t(bytes[offset+i]) << (8*i);
    return result;
}
inline void put_float(std::vector<std::uint8_t>& bytes, float value) {
    static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
    std::uint32_t bits=0; std::memcpy(&bits,&value,4); put(bytes,bits,4);
}
inline float get_float(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    const auto bits=static_cast<std::uint32_t>(get(bytes,offset,4));
    float value=0; std::memcpy(&value,&bits,4); return value;
}
inline bool header(std::span<const std::uint8_t> bytes, char type, std::size_t size) noexcept {
    return bytes.size() >= size && bytes[0]=='M' && bytes[1]=='X' && bytes[2]=='M' &&
        bytes[3]==static_cast<std::uint8_t>(type) && bytes[4]==version && bytes[7]==0;
}
inline void append_points(std::vector<std::uint8_t>& bytes,std::span<const Point> points) {
    for (const auto p : points) { put(bytes,p.x,2); put(bytes,p.z,2); }
}
inline void read_points(std::span<const std::uint8_t> bytes,std::size_t offset,std::span<Point> points) noexcept {
    for (auto& p : points) {
        p.x=static_cast<std::uint16_t>(get(bytes,offset,2));
        p.z=static_cast<std::uint16_t>(get(bytes,offset+2,2)); offset+=4;
    }
}
} // namespace detail

inline bool valid(const Command& command) noexcept {
    if (!command.epoch || !command.sequence || !command.count || command.count>max_points) return false;
    if (command.kind!=CommandKind::Route && command.kind!=CommandKind::Stop) return false;
    if (command.kind==CommandKind::Stop && command.count!=1) return false;
    return detail::valid_points({command.points.data(),command.count});
}
inline bool valid(const State& state) noexcept {
    if (!state.epoch || !state.state_sequence || state.count>max_points ||
        !std::isfinite(state.x) || !std::isfinite(state.z) ||
        !std::isfinite(state.speed) || state.x<0 || state.z<0 || state.x>51100 || state.z>51100) return false;
    switch (state.kind) {
    case StateKind::Started: if (!state.count || !state.command_sequence) return false; break;
    case StateKind::Stopped:
    case StateKind::Corrected: if (state.count || !state.command_sequence) return false; break;
    case StateKind::Snapshot: break;
    default: return false;
    }
    if (state.count ? state.speed<=0 : state.speed!=0) return false;
    return detail::valid_points({state.points.data(),state.count});
}

inline std::optional<std::vector<std::uint8_t>> encode(const Command& command) {
    if (!valid(command)) return std::nullopt;
    std::vector<std::uint8_t> bytes{'M','X','M','C',version,static_cast<std::uint8_t>(command.kind),command.count,0};
    bytes.reserve(command_header_size+4*command.count);
    detail::put(bytes,command.epoch,8); detail::put(bytes,command.sequence,8);
    detail::append_points(bytes,{command.points.data(),command.count});
    return bytes;
}
inline std::optional<Command> decode_command(std::span<const std::uint8_t> bytes) noexcept {
    if (!detail::header(bytes,'C',command_header_size) || bytes[6]>max_points ||
        bytes.size()!=command_header_size+4*std::size_t(bytes[6])) return std::nullopt;
    Command command;
    command.kind=static_cast<CommandKind>(bytes[5]); command.count=bytes[6];
    command.epoch=detail::get(bytes,8,8); command.sequence=detail::get(bytes,16,8);
    detail::read_points(bytes,command_header_size,{command.points.data(),command.count});
    return valid(command)?std::optional<Command>(command):std::nullopt;
}
inline std::optional<std::vector<std::uint8_t>> encode(const State& state) {
    if (!valid(state)) return std::nullopt;
    std::vector<std::uint8_t> bytes{'M','X','M','S',version,static_cast<std::uint8_t>(state.kind),state.count,0};
    bytes.reserve(state_header_size+4*state.count);
    detail::put(bytes,state.epoch,8); detail::put(bytes,state.command_sequence,8);
    detail::put(bytes,state.state_sequence,8); detail::put(bytes,state.server_time_ms,8);
    detail::put_float(bytes,state.x); detail::put_float(bytes,state.z); detail::put_float(bytes,state.speed);
    detail::append_points(bytes,{state.points.data(),state.count});
    return bytes;
}
inline std::optional<State> decode_state(std::span<const std::uint8_t> bytes) noexcept {
    if (!detail::header(bytes,'S',state_header_size) || bytes[6]>max_points ||
        bytes.size()!=state_header_size+4*std::size_t(bytes[6])) return std::nullopt;
    State state;
    state.kind=static_cast<StateKind>(bytes[5]); state.count=bytes[6];
    state.epoch=detail::get(bytes,8,8); state.command_sequence=detail::get(bytes,16,8);
    state.state_sequence=detail::get(bytes,24,8); state.server_time_ms=detail::get(bytes,32,8);
    state.x=detail::get_float(bytes,40); state.z=detail::get_float(bytes,44); state.speed=detail::get_float(bytes,48);
    detail::read_points(bytes,state_header_size,{state.points.data(),state.count});
    return valid(state)?std::optional<State>(state):std::nullopt;
}
} // namespace mxh::proto::movement
