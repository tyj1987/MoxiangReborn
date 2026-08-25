#include "mxh/compat/map_change_catalog.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace {

std::filesystem::path playdh() {
    auto cursor = std::filesystem::current_path();
    for (int depth = 0; depth < 8 && !cursor.empty(); ++depth) {
        const auto candidate = cursor / "modern" / "data" / "PlayDH";
        if (std::filesystem::is_directory(candidate)) return candidate;
        const auto parent = cursor.parent_path();
        if (parent == cursor) break;
        cursor = parent;
    }
    return {};
}

}  // namespace

TEST(MapChangeCatalog, ParsesTextFieldsAndFindsRoute) {
    const auto parsed = mxh::compat::parse_map_change_text(
        "1002\tCur\tNpc\t10\t12\t1.5\t2.5\t3.5\t4.5\t31\r\n");
    ASSERT_TRUE(parsed);
    ASSERT_EQ(parsed->entries.size(), 1u);
    const auto* entry = parsed->find_destination(10, 12);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->kind, 1002u);
    EXPECT_FLOAT_EQ(entry->move_x, 3.5f);
    ASSERT_NE(parsed->find_object_destination(10, "Npc"), nullptr);
    EXPECT_EQ(parsed->find_object_destination(10, "Npc")->move_map_num, 12u);
    EXPECT_EQ(parsed->find_object_destination(12, "Npc"), nullptr);
}

TEST(MapChangeCatalog, LoadsCanonicalPlayDhResource) {
    const auto root = playdh();
    if (root.empty()) GTEST_SKIP() << "source root unavailable";
    const auto catalog = mxh::compat::load_map_change_bin(
        root / "Resource" / "MapChange.bin");
    ASSERT_TRUE(catalog);
    EXPECT_EQ(catalog->entries.size(), 180u);
    ASSERT_NE(catalog->find_destination(10, 2), nullptr);
    EXPECT_EQ(catalog->find_destination(10, 2)->chx_num, 31u);
}
