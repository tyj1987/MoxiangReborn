#include "mxh/game/effect_catalog.hpp"

#include <gtest/gtest.h>

#include <windows.h>
#include <set>

namespace {
std::filesystem::path find_playdh_root() {
    wchar_t buf[MAX_PATH]{};
    const auto n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0) return {};
    auto p = std::filesystem::path(buf);
    for (int i = 0; i < 7; ++i) {
        p = p.parent_path();
        const auto candidate = p / L"modern" / L"data" / L"PlayDH";
        if (std::filesystem::exists(candidate)) return candidate;
    }
    return {};
}
}

TEST(EffectCatalog, IndexesShippedEffectAssets) {
    const auto root = find_playdh_root();
    if (root.empty()) GTEST_SKIP() << "PlayDH root not found";
    mxh::game::EffectCatalog catalog;
    std::string error;
    ASSERT_TRUE(catalog.load(root, &error)) << error;
    EXPECT_GT(catalog.beff_count(), 1000u);
    EXPECT_GT(catalog.packed_count(), 1000u);
    EXPECT_NE(catalog.find("m_combo_gum01.beff"), nullptr);
    ASSERT_NE(catalog.effect_name(0), nullptr);
    EXPECT_EQ(*catalog.effect_name(0), "shadow.beff");
    ASSERT_NE(catalog.effect_name(1, true), nullptr);
    EXPECT_GT(catalog.decoded_beff_count(), 1000u);
    const auto* script = catalog.script("become_h.beff");
    ASSERT_NE(script, nullptr);
    EXPECT_TRUE(script->decoded);
    EXPECT_GT(script->effect_unit_count, 0u);
    EXPECT_GT(script->trigger_count, 0u);
    ASSERT_EQ(script->trigger_count, script->triggers.size());
    ASSERT_EQ(script->effect_unit_count, script->units.size());
    EXPECT_EQ(script->triggers.front().time_token, "f0");
    EXPECT_EQ(script->triggers.front().kind, "ON");
    EXPECT_FALSE(script->units.front().kind.empty());
    EXPECT_FLOAT_EQ(script->units.front().radius, 120.0f);
    EXPECT_EQ(script->units.front().color_index, 0u);
    EXPECT_EQ(script->units.front().coordinate, 0u);
    EXPECT_EQ(script->units[2].sound_id, 405u);
    std::set<std::string> kinds;
    for (const auto& summary : catalog.scripts())
        for (const auto& unit : summary.units) kinds.insert(unit.kind);
    EXPECT_TRUE(kinds.contains("ANIMATION"));
    EXPECT_TRUE(kinds.contains("DAMAGE"));
    EXPECT_TRUE(kinds.contains("LIGHT"));
    EXPECT_TRUE(kinds.contains("MOVE"));
    EXPECT_TRUE(kinds.contains("OBJECT"));
    EXPECT_TRUE(kinds.contains("SOUND"));
}

TEST(EffectCatalog, RejectsMissingRoot) {
    mxh::game::EffectCatalog catalog;
    std::string error;
    EXPECT_FALSE(catalog.load(L"C:/does-not-exist/moxian-effects", &error));
    EXPECT_EQ(error, "effect resource root does not exist");
}
