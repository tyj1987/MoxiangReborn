#pragma once

// Portable soak-flag parsing and character-state JSON for mxh_client_e2e.
// No sockets, no original game bytes. Linux unit tests include this header
// directly. A live login / map / peer / RST run needs Windows.

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mxh::e2e {

// Distinct from 0 (pass), 1 (spawn), 2 (protocol), and 3 (usage).
// --drop-socket-after closes with RST and returns this code.
inline constexpr int kSimulatedDisconnectExitCode = 4;

inline constexpr int kDefaultExpectPeerTimeoutSeconds = 30;
inline constexpr int kKeepAliveIntervalSeconds = 10;
inline constexpr int kMaxSoakSeconds = 7 * 24 * 60 * 60;
inline constexpr std::size_t kMaxCharacterNameBytes = 16;

struct SoakOptions {
    bool reuse = false;
    std::string reuse_character;
    bool hold = false;
    int hold_seconds = 0;
    bool expect_peer = false;
    std::string expect_peer_name;
    int expect_peer_timeout_s = kDefaultExpectPeerTimeoutSeconds;
    bool state_dump = false;
    std::string state_dump_path;
    bool drop = false;
    int drop_socket_after_s = 0;
};

inline bool in_map_soak_requested(const SoakOptions& opt) noexcept {
    return opt.hold || opt.expect_peer || opt.state_dump || opt.drop;
}

inline bool is_soak_flag(std::string_view flag) noexcept {
    return flag == "--reuse-character" || flag == "--hold-seconds" ||
           flag == "--expect-peer" || flag == "--expect-peer-timeout" ||
           flag == "--state-dump" || flag == "--drop-socket-after";
}

inline bool valid_character_name(std::string_view name, std::string& error) {
    if (name.empty()) {
        error = "character name is empty";
        return false;
    }
    if (name.size() > kMaxCharacterNameBytes) {
        error = "character name longer than 16 bytes";
        return false;
    }
    if (name.find_first_of("\r\n") != std::string_view::npos) {
        error = "character name contains a line break";
        return false;
    }
    return true;
}

inline bool parse_non_negative(std::string_view text, int& out, std::string& error,
                               std::string_view flag) {
    if (text.empty() || text == "+") {
        error = std::string(flag) + " requires a non-negative integer";
        return false;
    }
    std::size_t index = 0;
    if (text[0] == '+') index = 1;
    long value = 0;
    for (; index < text.size(); ++index) {
        const unsigned char digit = static_cast<unsigned char>(text[index]);
        if (digit < '0' || digit > '9') {
            error = std::string(flag) + " requires a non-negative integer";
            return false;
        }
        value = value * 10 + (digit - '0');
        if (value > kMaxSoakSeconds) {
            error = std::string(flag) + " is out of range";
            return false;
        }
    }
    out = static_cast<int>(value);
    return true;
}

// argv[index] is the flag. On success index is the last consumed argument.
inline bool consume_soak_flag(SoakOptions& opt, int argc, char** argv, int& index,
                              std::string& error) {
    const std::string_view flag = argv[index];
    auto take_value = [&](std::string& value) -> bool {
        if (index + 1 >= argc || argv[index + 1] == nullptr) {
            error = std::string(flag) + " requires a value";
            return false;
        }
        value = argv[++index];
        return true;
    };

    if (flag == "--reuse-character") {
        std::string value;
        if (!take_value(value)) return false;
        if (!valid_character_name(value, error)) return false;
        opt.reuse = true;
        opt.reuse_character = std::move(value);
        return true;
    }
    if (flag == "--expect-peer") {
        std::string value;
        if (!take_value(value)) return false;
        if (!valid_character_name(value, error)) return false;
        opt.expect_peer = true;
        opt.expect_peer_name = std::move(value);
        return true;
    }
    if (flag == "--state-dump") {
        std::string value;
        if (!take_value(value)) return false;
        if (value.empty()) {
            error = "--state-dump requires a path";
            return false;
        }
        opt.state_dump = true;
        opt.state_dump_path = std::move(value);
        return true;
    }
    if (flag == "--hold-seconds") {
        std::string value;
        if (!take_value(value)) return false;
        int seconds = 0;
        if (!parse_non_negative(value, seconds, error, flag)) return false;
        opt.hold = true;
        opt.hold_seconds = seconds;
        return true;
    }
    if (flag == "--drop-socket-after") {
        std::string value;
        if (!take_value(value)) return false;
        int seconds = 0;
        if (!parse_non_negative(value, seconds, error, flag)) return false;
        opt.drop = true;
        opt.drop_socket_after_s = seconds;
        return true;
    }
    if (flag == "--expect-peer-timeout") {
        std::string value;
        if (!take_value(value)) return false;
        int seconds = 0;
        if (!parse_non_negative(value, seconds, error, flag)) return false;
        if (seconds < 1) {
            error = "--expect-peer-timeout must be >= 1";
            return false;
        }
        opt.expect_peer_timeout_s = seconds;
        return true;
    }
    error = std::string("unknown arg: ") + std::string(flag);
    return false;
}

// Soak flags only. Any other token is a usage error so this parser cannot
// drift from the flags mxh_client_e2e accepts.
inline bool parse_soak_args(const std::vector<std::string>& args, SoakOptions& opt,
                            std::string& error) {
    std::vector<std::string> storage;
    storage.reserve(args.size() + 1);
    storage.emplace_back("mxh_client_e2e");
    storage.insert(storage.end(), args.begin(), args.end());
    std::vector<char*> argv;
    argv.reserve(storage.size());
    for (auto& token : storage) argv.push_back(token.data());

    SoakOptions parsed;
    const int argc = static_cast<int>(argv.size());
    for (int index = 1; index < argc; ++index) {
        if (!is_soak_flag(argv[index])) {
            error = std::string("unknown arg: ") + argv[index];
            return false;
        }
        if (!consume_soak_flag(parsed, argc, argv.data(), index, error)) return false;
    }
    opt = std::move(parsed);
    return true;
}

// One occupied inventory slot. The writer sorts by slot, then item_index,
// then count, so two dumps of the same items compare equal.
struct InventorySlot {
    int slot = 0;
    int item_index = 0;
    std::uint32_t count = 0;
};

// position.y is the protocol ground axis (position_z / local_z).
struct CharacterSnapshot {
    int x = 0;
    int y = 0;
    int map = 0;
    std::uint32_t hp = 0;
    std::uint32_t mp = 0;
    std::uint32_t shield = 0;
    std::uint32_t money = 0;
    std::vector<InventorySlot> inventory;
};

struct SessionDump {
    CharacterSnapshot entry;
    CharacterSnapshot exit;
};

inline void append_indent(std::string& out, int spaces) {
    out.append(static_cast<std::size_t>(spaces), ' ');
}

inline void append_snapshot_json(std::string& out, const CharacterSnapshot& snap) {
    auto items = snap.inventory;
    std::sort(items.begin(), items.end(), [](const InventorySlot& lhs, const InventorySlot& rhs) {
        if (lhs.slot != rhs.slot) return lhs.slot < rhs.slot;
        if (lhs.item_index != rhs.item_index) return lhs.item_index < rhs.item_index;
        return lhs.count < rhs.count;
    });

    out += "{\n";
    append_indent(out, 4);
    out += "\"hp\": " + std::to_string(snap.hp) + ",\n";
    append_indent(out, 4);
    out += "\"inventory\": ";
    if (items.empty()) {
        out += "[]";
    } else {
        out += "[\n";
        for (std::size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            append_indent(out, 6);
            out += "{\n";
            append_indent(out, 8);
            out += "\"count\": " + std::to_string(item.count) + ",\n";
            append_indent(out, 8);
            out += "\"item_index\": " + std::to_string(item.item_index) + ",\n";
            append_indent(out, 8);
            out += "\"slot\": " + std::to_string(item.slot) + "\n";
            append_indent(out, 6);
            out += "}";
            if (i + 1 != items.size()) out += ",";
            out += "\n";
        }
        append_indent(out, 4);
        out += "]";
    }
    out += ",\n";
    append_indent(out, 4);
    out += "\"money\": " + std::to_string(snap.money) + ",\n";
    append_indent(out, 4);
    out += "\"mp\": " + std::to_string(snap.mp) + ",\n";
    append_indent(out, 4);
    out += "\"position\": {\n";
    append_indent(out, 6);
    out += "\"map\": " + std::to_string(snap.map) + ",\n";
    append_indent(out, 6);
    out += "\"x\": " + std::to_string(snap.x) + ",\n";
    append_indent(out, 6);
    out += "\"y\": " + std::to_string(snap.y) + "\n";
    append_indent(out, 4);
    out += "},\n";
    append_indent(out, 4);
    out += "\"shield\": " + std::to_string(snap.shield) + "\n";
    append_indent(out, 2);
    out += "}";
}

// Keys are emitted in a fixed order (alphabetical within each object)
// so two dumps of the same state diff cleanly.
inline std::string format_session_dump_json(const SessionDump& dump) {
    std::string out;
    out.reserve(512);
    out += "{\n";
    append_indent(out, 2);
    out += "\"entry\": ";
    append_snapshot_json(out, dump.entry);
    out += ",\n";
    append_indent(out, 2);
    out += "\"exit\": ";
    append_snapshot_json(out, dump.exit);
    out += "\n}\n";
    return out;
}

inline bool write_session_dump_file(const std::filesystem::path& path,
                                    const SessionDump& dump, std::string& error) {
    std::error_code filesystem_error;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, filesystem_error);
        if (filesystem_error) {
            error = "cannot create " + parent.string() + ": " + filesystem_error.message();
            return false;
        }
    }
    const auto json = format_session_dump_json(dump);
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "cannot open " + path.string();
        return false;
    }
    output.write(json.data(), static_cast<std::streamsize>(json.size()));
    if (!output) {
        error = "cannot write " + path.string();
        return false;
    }
    return true;
}

}  // namespace mxh::e2e
