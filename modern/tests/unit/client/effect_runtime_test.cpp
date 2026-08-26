#include "EffectRuntime.hpp"

#include <gtest/gtest.h>

#include <windows.h>

#include <algorithm>

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

TEST(EffectRuntime, RejectsUnknownEffectWithoutFallback) {
    mxh::client::EffectRuntime runtime;
    EXPECT_FALSE(runtime.start("missing.beff", 1, 2, 0, 16));
    EXPECT_EQ(runtime.active_count(), 0u);
}

TEST(EffectRuntime, EmptyRuntimeDoesNotEmitEvents) {
    mxh::client::EffectRuntime runtime;
    std::size_t emitted = 0;
    runtime.advance(100, [&](const auto&) { ++emitted; });
    EXPECT_EQ(emitted, 0u);
    EXPECT_EQ(runtime.active_count(), 0u);
}

TEST(EffectRuntime, RunsDecodedRealEffectWithObjectContext) {
    const auto root = find_playdh_root();
    if (root.empty()) GTEST_SKIP() << "PlayDH root not found";
    mxh::client::EffectRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(root, &error)) << error;
    ASSERT_TRUE(runtime.start("become_h.beff", 101, 202, 1000, 16));
    std::vector<mxh::client::RuntimeEffectEvent> emitted;
    runtime.advance(1000, [&](const auto& event) { emitted.push_back(event); });
    ASSERT_FALSE(emitted.empty());
    EXPECT_EQ(emitted.front().effect_name, "become_h.beff");
    EXPECT_EQ(emitted.front().source_object_id, 101u);
    EXPECT_EQ(emitted.front().target_object_id, 202u);
    const auto sound = std::find_if(emitted.begin(), emitted.end(),
        [](const auto& event) { return event.unit_kind == "SOUND"; });
    ASSERT_NE(sound, emitted.end());
    EXPECT_TRUE(sound->sound_id == 405u || sound->sound_id == 409u ||
                sound->sound_id == 407u || sound->sound_id == 408u);
}

TEST(EffectRuntime, RunsAuthoritativeCombatEffectFromSkillList) {
    const auto root = find_playdh_root();
    if (root.empty()) GTEST_SKIP() << "PlayDH root not found";
    mxh::client::EffectRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(root, &error)) << error;
    ASSERT_TRUE(runtime.start("m_combo_gum01.beff", 101, 202, 1000, 16));
    EXPECT_GT(runtime.active_count(), 0u);
}

TEST(EffectRuntime, StopObjectRemovesMatchingSourceAndTargetInstances) {
    const auto root = find_playdh_root();
    if (root.empty()) GTEST_SKIP() << "PlayDH root not found";
    mxh::client::EffectRuntime runtime;
    std::string error;
    ASSERT_TRUE(runtime.load(root, &error)) << error;
    ASSERT_TRUE(runtime.start("m_combo_gum01.beff", 101, 202, 1000, 16));
    ASSERT_TRUE(runtime.start("m_combo_gum01.beff", 303, 404, 1000, 16));
    EXPECT_EQ(runtime.stop_object(202), 1u);
    EXPECT_EQ(runtime.active_count(), 1u);
    EXPECT_EQ(runtime.stop_object(303), 1u);
    EXPECT_EQ(runtime.active_count(), 0u);
    EXPECT_EQ(runtime.stop_object(0), 0u);
}

TEST(EffectRuntime, MissingIdFailsWithoutGuessingAnotherEffect) {
    mxh::client::EffectRuntime runtime;
    EXPECT_FALSE(runtime.start_by_id(999999u, false, 1, 2, 0, 16));
    EXPECT_EQ(runtime.active_count(), 0u);
}
