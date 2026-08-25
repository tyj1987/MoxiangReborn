#include "mxh/server/drop_item.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace {

std::filesystem::path find_playdh_drop_file() {
    auto base = std::filesystem::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        const auto candidate = base / "modern" / "data" / "PlayDH" /
            "Resource" / "Server" / "MonsterDropItemList.bin";
        if (std::filesystem::exists(candidate)) return candidate;
        const auto parent = base.parent_path();
        if (parent == base) break;
        base = parent;
    }
    return {};
}

}  // namespace

TEST(DropItemLoaderTest, RejectsMislabeledCanonicalPayloadFailClosed) {
    const auto path = find_playdh_drop_file();
    if (path.empty()) GTEST_SKIP() << "canonical PlayDH not available";
    std::string error;
    const auto tables = mxh::server::load_drop_item_tables(
        path, "playdh-current", &error);
    // The current playdh-current file is byte-valid but its decoded payload
    // starts with the AIGroup grammar ($Group), not the legacy drop grammar.
    // Keep this explicit until the profile supplies the correct variant;
    // silently treating it as a drop table would corrupt gameplay.
    EXPECT_TRUE(tables.empty());
    EXPECT_NE(error.find("$Group"), std::string::npos);
}
