#include <gtest/gtest.h>

#include "mxh/compat/character_appearance_catalog.hpp"

#include <filesystem>
#include <fstream>

namespace {
std::filesystem::path findClientResource() {
    auto root = std::filesystem::current_path();
    for (int depth = 0; depth < 8 && !root.empty(); ++depth, root = root.parent_path()) {
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (!entry.is_directory()) continue;
            const auto nested = entry.path() / L"PlayDH" / L"Resource" / L"Client";
            const auto direct = entry.path() / L"Resource" / L"Client";
            if (std::filesystem::is_regular_file(nested / L"ModList_M.bin")) return nested;
            if (std::filesystem::is_regular_file(direct / L"ModList_M.bin")) return direct;
        }
    }
    return {};
}

std::vector<std::uint8_t> read(const std::filesystem::path& file) {
    std::ifstream input(file, std::ios::binary | std::ios::ate);
    if (!input) return {};
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(input.tellg()));
    input.seekg(0); input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    return bytes;
}
}

TEST(CharacterAppearanceCatalog, ParsesRealMaleAndFemaleLists) {
    const auto root = findClientResource();
    if (root.empty()) GTEST_SKIP() << "PlayDH client appearance tables unavailable";
    for (const auto gender : {L'M', L'W'}) {
        const auto suffix = std::wstring(1, gender) + L".bin";
        const auto body = mxh::compat::CharacterAppearanceCatalog::parse_mod_list_bin(
            read(root / (L"ModList_" + suffix)));
        const auto faces = mxh::compat::CharacterAppearanceCatalog::parse_part_list_bin(
            read(root / (L"FaceList_" + suffix)));
        const auto hairs = mxh::compat::CharacterAppearanceCatalog::parse_part_list_bin(
            read(root / (L"HairList_" + suffix)));
        ASSERT_TRUE(body.has_value());
        ASSERT_TRUE(faces.has_value());
        ASSERT_TRUE(hairs.has_value());
        EXPECT_EQ(body->base_object, gender == L'M' ? "man.chx" : "woman.chx");
        EXPECT_GT(body->mod_files.size(), 100u);
        EXPECT_EQ(faces->size(), 5u);
        EXPECT_EQ(hairs->size(), 5u);
        for (const auto& file : *faces) EXPECT_TRUE(file.ends_with(".MOD"));
        for (const auto& file : *hairs) EXPECT_TRUE(file.ends_with(".MOD"));
    }
}
