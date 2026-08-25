#include "mxh/game/effect_catalog.hpp"

#include <gtest/gtest.h>

#include <windows.h>

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
    EXPECT_GT(catalog.decoded_beff_count(), 1000u);
    const auto* script = catalog.script("become_h.beff");
    ASSERT_NE(script, nullptr);
    EXPECT_TRUE(script->decoded);
    EXPECT_GT(script->effect_unit_count, 0u);
    EXPECT_GT(script->trigger_count, 0u);
}

TEST(EffectCatalog, RejectsMissingRoot) {
    mxh::game::EffectCatalog catalog;
    std::string error;
    EXPECT_FALSE(catalog.load(L"C:/does-not-exist/moxian-effects", &error));
    EXPECT_EQ(error, "effect resource root does not exist");
}
