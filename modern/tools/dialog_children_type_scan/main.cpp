// dialog_children_type_scan.cpp — 一次性调试: 枚举所有 *.bin 解析 children,
// 输出 type 字符串 + 出现频次, 用于 M-R4.5 决策. 头less, 不调 GPU/sprite.
#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/ui/interface_script.hpp"

#include "mxh/tools/cli_playdh_root.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <string>

int main(int argc, char** argv) {
    namespace fs = std::filesystem;
    std::string explicit_root;
    bool resolve_only = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--resource-root") {
            if (i + 1 >= argc) {
                std::cerr << "--resource-root requires an argument\n";
                return 1;
            }
            explicit_root = argv[++i];
        } else if (arg == "--resolve-only") {
            resolve_only = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "usage: mxh_dialog_children_type_scan [--resource-root DIR] [--resolve-only]\n"
                         "  MXH_PLAYDH_ROOT is used when --resource-root is omitted.\n";
            return 0;
        } else if (!arg.empty() && arg[0] != '-') {
            explicit_root = arg;
        } else {
            std::cerr << "unknown arg: " << arg << "\n";
            return 1;
        }
    }
    const auto resolved = mxh::tools::resolve_cli_playdh_root(
        explicit_root, std::getenv("MXH_PLAYDH_ROOT"));
    if (!resolved.error.empty()) {
        std::cerr << resolved.error << "\n";
        return 1;
    }
    if (resolve_only) {
        std::cout << "resource_root=" << resolved.path.string() << "\n";
        return 0;
    }
    const fs::path playdh_root = resolved.path;
    const auto dir = playdh_root / "Image" / "InterfaceScript";
    if (!fs::is_directory(dir)) {
        std::cerr << "missing: " << dir << "\n";
        return 1;
    }
    std::vector<fs::path> bins;
    for (auto& e : fs::directory_iterator(dir)) {
        if (e.is_regular_file() && e.path().extension() == ".bin") {
            bins.push_back(e.path());
        }
    }
    std::sort(bins.begin(), bins.end());
    std::cout << "scanning " << bins.size() << " .bin files\n";

    std::map<std::string, std::size_t> type_count;
    std::map<std::string, std::size_t> root_type_count;
    std::size_t total_children = 0;
    std::size_t total_roots = 0;
    for (const auto& bin : bins) {
        auto read = mxh::compat::read_mh_bin(bin);
        if (!read.ok() || read.value.data.empty()) continue;
        std::string_view payload(
            reinterpret_cast<const char*>(read.value.data.data()),
            read.value.data.size());
        try {
            auto parsed = mxh::ui::parse_interface_script(payload);
            for (const auto& root : parsed.roots) {
                ++root_type_count[root->type];
                ++total_roots;
                std::function<void(const mxh::ui::InterfaceNode&)> walk =
                    [&](const mxh::ui::InterfaceNode& n) {
                        for (const auto& c : n.children) {
                            ++type_count[c->type];
                            ++total_children;
                            walk(*c);
                        }
                    };
                walk(*root);
            }
        } catch (...) {
            // skip parse errors
        }
    }
    std::cout << "\nroot types (" << total_roots << " total):\n";
    for (const auto& [t, c] : root_type_count) {
        std::cout << "  " << c << "\t" << t << "\n";
    }
    std::cout << "\nchildren types (" << total_children << " total):\n";
    std::vector<std::pair<std::string, std::size_t>> v(type_count.begin(),
                                                       type_count.end());
    std::sort(v.begin(), v.end(),
              [](auto& a, auto& b) { return a.second > b.second; });
    for (const auto& [t, c] : v) {
        std::cout << "  " << c << "\t" << t << "\n";
    }
    return 0;
}
