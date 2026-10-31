// Host tool. Prints the compiled Item protocol as Name=Value lines in
// MXH_ITEM_PROTOCOL_EACH order. CMake hashes that text; this file does not.
#include "mxh/proto/protocol.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>

namespace {

constexpr std::size_t kNameCount =
    sizeof(mxh::proto::kItemProtocolNames) / sizeof(mxh::proto::kItemProtocolNames[0]);
constexpr std::size_t kValueCount =
    sizeof(mxh::proto::kItemProtocolValues) / sizeof(mxh::proto::kItemProtocolValues[0]);

static_assert(kNameCount != 0, "Item protocol canonical list is empty");
static_assert(kValueCount != 0, "Item protocol canonical list is empty");
static_assert(mxh::proto::kItemProtocolCount != 0,
              "Item protocol canonical list is empty");
static_assert(kNameCount == mxh::proto::kItemProtocolCount,
              "canonical entries must equal kItemProtocolCount");
static_assert(kValueCount == mxh::proto::kItemProtocolCount,
              "canonical entries must equal kItemProtocolCount");

void Usage() {
    std::cerr << "usage: mxh_item_protocol_canon --list PATH --count PATH\n";
}

}  // namespace

int main(int argc, char** argv) {
    const char* list_path = nullptr;
    const char* count_path = nullptr;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] == nullptr ? std::string() : std::string(argv[i]);
        if (arg == "--list" && (i + 1) < argc) {
            list_path = argv[++i];
        } else if (arg == "--count" && (i + 1) < argc) {
            count_path = argv[++i];
        } else {
            Usage();
            return 2;
        }
    }
    if (list_path == nullptr || count_path == nullptr) {
        Usage();
        return 2;
    }

    std::ofstream list(list_path, std::ios::binary | std::ios::trunc);
    if (!list) {
        std::cerr << "failed to write item protocol canonical list\n";
        return 1;
    }
    std::size_t emitted = 0;
    for (std::size_t i = 0; i < mxh::proto::kItemProtocolCount; ++i) {
        const auto value = static_cast<unsigned>(mxh::proto::kItemProtocolValues[i]);
        list << mxh::proto::kItemProtocolNames[i] << '=' << value << '\n';
        ++emitted;
    }
    list.close();
    if (!list || emitted != mxh::proto::kItemProtocolCount) {
        std::cerr << "item protocol canonical list was not fully written\n";
        return 1;
    }

    std::ofstream count(count_path, std::ios::binary | std::ios::trunc);
    if (!count) {
        std::cerr << "failed to write item protocol canonical count\n";
        return 1;
    }
    count << mxh::proto::kItemProtocolCount << '\n';
    count.close();
    if (!count) {
        std::cerr << "failed to flush item protocol canonical count\n";
        return 1;
    }
    return 0;
}
