#include "mxh/game/effect_catalog.hpp"

#include <gtest/gtest.h>

#include <windows.h>
#include <chrono>
#include <fstream>
#include <set>
#include "../../support/playdh_env.hpp"

namespace {
std::filesystem::path find_playdh_root() {
    if (const auto playdh_override = mxh::test::playdh_dir_override()) return *playdh_override;
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
    bool hasAnimationMotion = false;
    bool hasDamageKind = false;
    bool hasMoveDuration = false;
    for (const auto& summary : catalog.scripts()) {
        for (const auto& unit : summary.units) {
            hasAnimationMotion = hasAnimationMotion ||
                (unit.kind == "ANIMATION" && unit.motion_index != 0u);
            hasDamageKind = hasDamageKind ||
                (unit.kind == "DAMAGE" && !unit.damage_kind.empty());
            hasMoveDuration = hasMoveDuration ||
                (unit.kind == "MOVE" && unit.duration_ms != 0u);
        }
    }
    EXPECT_TRUE(hasAnimationMotion);
    EXPECT_TRUE(hasDamageKind);
    EXPECT_TRUE(hasMoveDuration);
}

TEST(EffectCatalog, WarnsWhenEffectPakOpenFails) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() /
        ("mxh-effect-pak-" + std::to_string(static_cast<long long>(stamp)));
    std::filesystem::create_directories(root);
    {
        std::vector<unsigned char> header(96, 0);
        header[0] = 1;
        std::ofstream out(root / "Effect.pak", std::ios::binary);
        ASSERT_TRUE(out);
        out.write(reinterpret_cast<const char*>(header.data()),
                  static_cast<std::streamsize>(header.size()));
    }

    testing::internal::CaptureStderr();
    mxh::game::EffectCatalog catalog;
    std::string error;
    const bool opened = catalog.load(root, &error);
    const std::string logged = testing::internal::GetCapturedStderr();

    EXPECT_FALSE(opened);
    EXPECT_EQ(error, "Effect.pak is not a valid PackFile");
    EXPECT_NE(logged.find("Effect.pak"), std::string::npos) << logged;
    EXPECT_NE(logged.find("unsupported header"), std::string::npos) << logged;
    std::filesystem::remove_all(root);
}

TEST(EffectCatalog, RejectsMissingRoot) {
    mxh::game::EffectCatalog catalog;
    std::string error;
    EXPECT_FALSE(catalog.load(L"C:/does-not-exist/moxian-effects", &error));
    EXPECT_EQ(error, "effect resource root does not exist");
}
