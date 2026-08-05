// T1 expansion - read_mh_bin coverage. Verifies modern can
// fully parse all standard .bin files (not just header sniff).
//
// Phase E1 / C-Batch-E1: targets all 59 deploy/Resource .bin files
// (the legacy server shipping set). Each file is opened via
// read_mh_bin, expected MhError::Ok, header sanity-checked, and
// payload size asserted against header.file_size.

#include "mxh/compat/mh_file_ex.hpp"

#define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

fs::path find_resource_dir() {
    fs::path cwd;
    try { cwd = fs::current_path(); } catch (...) { return {}; }
    // Try known candidate locations, walking up from cwd.
    const std::vector<std::wstring> sub_paths = {
        L"deploy/server/Distribute/Resource",
        L"deploy\\server\\Distribute\\Resource",
        L"../deploy/server/Distribute/Resource",
    };
    for (fs::path base = cwd; !base.empty(); base = base.parent_path()) {
        for (const auto& sub : sub_paths) {
            std::error_code ec;
            fs::path candidate = base / sub;
            if (fs::is_directory(candidate, ec)) return candidate;
        }
        if (base == base.root_path()) break;
    }
    return {};
}


// PlayDH/Resource helpers.
// PlayDH is the canonical full-resource tree; Distribute/Resource is the deploy subset.
// Tests for PlayDH-only files (TitanSpellCostPerMap.bin etc.) and Client/ subdir
// files (HairList_M.bin etc.) use find_playdh_dir / find_client_resource_dir.
static const char kPlayDH_u8[] = "\xE5\xA2\xA8\xE9\xA6\x99\xE3\x80\x90\xE6\xBA\x90\xE7\xA0\x81\xE9\x85\x8D\xE5\xA5\x97\xE8\xB5\x84\xE6\xBA\x90\xE3\x80\x91/PlayDH/Resource";
static const char kClient_u8[] = "Client";
fs::path find_playdh_dir() {
    fs::path cwd;
    try { cwd = fs::current_path(); } catch (...) { return {}; }
    for (fs::path base = cwd; !base.empty(); base = base.parent_path()) {
        std::error_code ec;
        fs::path candidate = base / fs::u8path(kPlayDH_u8);
        if (fs::is_directory(candidate, ec)) return candidate;
        if (base == base.root_path()) break;
    }
    return {};
}
fs::path find_client_resource_dir() {
    const fs::path playdh = find_playdh_dir();
    if (playdh.empty()) return {};
    const fs::path client = playdh / fs::u8path(kClient_u8);
    std::error_code ec;
    return fs::is_directory(client, ec) ? client : fs::path{};
}

}  // namespace

TEST(MxhResourceParse, ReadMhBin_AbilityBaseInfo_bin) {
    static const char* kName = "AbilityBaseInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_AbilityCalcInfo_bin) {
    static const char* kName = "AbilityCalcInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_AvatarEquip_bin) {
    static const char* kName = "AvatarEquip.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_BobusangInfo_bin) {
    static const char* kName = "BobusangInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_CastleGateList_bin) {
    static const char* kName = "CastleGateList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_CharacterExpPoint_bin) {
    static const char* kName = "CharacterExpPoint.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_CostumeSkinItemList_bin) {
    static const char* kName = "CostumeSkinItemList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_Dealitem_bin) {
    static const char* kName = "Dealitem.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_FilterWord_bin) {
    static const char* kName = "FilterWord.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_FlagNpcInfo_bin) {
    static const char* kName = "FlagNpcInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_GuildLevel_bin) {
    static const char* kName = "GuildLevel.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_GuildPointPlustimeList_bin) {
    static const char* kName = "GuildPointPlustimeList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_ItemList_bin) {
    static const char* kName = "ItemList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_ItemMixList_bin) {
    static const char* kName = "ItemMixList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_Item_RareItemInfo_bin) {
    static const char* kName = "Item_RareItemInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_ItemdupOption_bin) {
    static const char* kName = "ItemdupOption.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_JobSkillList_bin) {
    static const char* kName = "JobSkillList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_Jobskill_bin) {
    static const char* kName = "Jobskill.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_KyungGongInfo_bin) {
    static const char* kName = "KyungGongInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_MapChange_bin) {
    static const char* kName = "MapChange.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_MapKindInfo_bin) {
    static const char* kName = "MapKindInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_MonsterDropItemList_bin) {
    static const char* kName = "MonsterDropItemList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_MonsterList_bin) {
    static const char* kName = "MonsterList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_NpcList_bin) {
    static const char* kName = "NpcList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_PaneltyTime_bin) {
    static const char* kName = "PaneltyTime.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_PartyPlustimeInfo_bin) {
    static const char* kName = "PartyPlustimeInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_PenaltyTime_bin) {
    static const char* kName = "PenaltyTime.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_PetBuffList_bin) {
    static const char* kName = "PetBuffList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_PetList_bin) {
    static const char* kName = "PetList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_PetRule_bin) {
    static const char* kName = "PetRule.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_PyogukListInfo_bin) {
    static const char* kName = "PyogukListInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_SiegeWarMapInfo_bin) {
    static const char* kName = "SiegeWarMapInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_SkillAreaList_bin) {
    static const char* kName = "SkillAreaList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_SkillList_bin) {
    static const char* kName = "SkillList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_SkillOptionList_bin) {
    static const char* kName = "SkillOptionList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_SkillTree_bin) {
    static const char* kName = "SkillTree.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_SkillchangeList_bin) {
    static const char* kName = "SkillchangeList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_SkinSelectItemList_bin) {
    static const char* kName = "SkinSelectItemList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_StateInfo_bin) {
    static const char* kName = "StateInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_StaticNpc_bin) {
    static const char* kName = "StaticNpc.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_SuryunLevelChange_bin) {
    static const char* kName = "SuryunLevelChange.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_SuryunMonster_bin) {
    static const char* kName = "SuryunMonster.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_Suryundata_bin) {
    static const char* kName = "Suryundata.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_TitanBreakList_bin) {
    static const char* kName = "TitanBreakList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_TitanList_bin) {
    static const char* kName = "TitanList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_TitanMapDropInfo_bin) {
    static const char* kName = "TitanMapDropInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_TitanMixList_bin) {
    static const char* kName = "TitanMixList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_TitanPartsKind_bin) {
    static const char* kName = "TitanPartsKind.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_TitanRule_bin) {
    static const char* kName = "TitanRule.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_TitanUpgradeInfo_bin) {
    static const char* kName = "TitanUpgradeInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_UniqueItemMixList_bin) {
    static const char* kName = "UniqueItemMixList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_UniqueItemOptionList_bin) {
    static const char* kName = "UniqueItemOptionList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_hideitemlock_bin) {
    static const char* kName = "hideitemlock.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_itemReinforceList_bin) {
    static const char* kName = "itemReinforceList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_item_RareReinforceList_bin) {
    static const char* kName = "item_RareReinforceList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_itembreak_bin) {
    static const char* kName = "itembreak.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_sat_bin) {
    static const char* kName = "sat.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_setitem_opt_bin) {
    static const char* kName = "setitem_opt.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}

TEST(MxhResourceParse, ReadMhBin_tacticstartinfo_bin) {
    static const char* kName = "tacticstartinfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "deploy/Resource not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}



TEST(MxhResourceParseClient, ReadMhBin_AvatarItemException_bin) {
    static const char* kName = "AvatarItemException.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_BRList_bin) {
    static const char* kName = "BRList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_BRTList_bin) {
    static const char* kName = "BRTList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_CharMake_SelectOption_bin) {
    static const char* kName = "CharMake_SelectOption.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_default_bin) {
    static const char* kName = "default.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_EffectList_bin) {
    static const char* kName = "EffectList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_FaceList_M_bin) {
    static const char* kName = "FaceList_M.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_FaceList_W_bin) {
    static const char* kName = "FaceList_W.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_HairList_M_bin) {
    static const char* kName = "HairList_M.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_HairList_W_bin) {
    static const char* kName = "HairList_W.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_IllusionMaterial_bin) {
    static const char* kName = "IllusionMaterial.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_ItemChxList_bin) {
    static const char* kName = "ItemChxList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_ModList_M_bin) {
    static const char* kName = "ModList_M.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_ModList_T_bin) {
    static const char* kName = "ModList_T.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_ModList_W_bin) {
    static const char* kName = "ModList_W.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_MonsterInfoInMap_bin) {
    static const char* kName = "MonsterInfoInMap.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_MonsterSpeechList_bin) {
    static const char* kName = "MonsterSpeechList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_motionList_bin) {
    static const char* kName = "motionList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_NpcChxList_bin) {
    static const char* kName = "NpcChxList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_PetSpeechList_bin) {
    static const char* kName = "PetSpeechList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_PhysiqueHairList_bin) {
    static const char* kName = "PhysiqueHairList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_PlusItemEffect_bin) {
    static const char* kName = "PlusItemEffect.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_PreLoadData_bin) {
    static const char* kName = "PreLoadData.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_PremierSkill_bin) {
    static const char* kName = "PremierSkill.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_SFList_bin) {
    static const char* kName = "SFList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_shadowGhost_bin) {
    static const char* kName = "shadowGhost.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_Tooltipinfo_bin) {
    static const char* kName = "Tooltipinfo.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_UserCommandList_bin) {
    static const char* kName = "UserCommandList.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseClient, ReadMhBin_WeatherEffect_bin) {
    static const char* kName = "WeatherEffect.bin";
    const auto dir = find_client_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH/Resource/Client not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParse, ReadMhBin_TitanSpellCostPerMap_bin) {
    static const char* kName = "TitanSpellCostPerMap.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "PlayDH not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size)
        << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}
// Auto-generated by gen_subdir_addendum.py 2026-08-06
// Coverage: deploy Server/ + QuestScript/ subdirs + PlayDH non-Client (top-level + subdirs)
// Excludes 14-byte placeholder stubs (header-only, no payload) since EXPECT_GT(file_size,0) would fail.
// Aligns with resource_payload_manifest_*.json SHA-256 lock-in (commit 6210ea2e).

TEST(MxhResourceParseServer, ReadMhBin_Server_AttribItemChangeRato) {
    static const char* kName = "Server/AttribItemChangeRato.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/AttribItemChangeRato.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_BobusangInfo) {
    static const char* kName = "Server/BobusangInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/BobusangInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_BossMonsterfileList) {
    static const char* kName = "Server/BossMonsterfileList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/BossMonsterfileList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_BossReward) {
    static const char* kName = "Server/BossReward.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/BossReward.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Boss_CowI) {
    static const char* kName = "Server/Boss_CowI.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Boss_CowI.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Boss_Cowf) {
    static const char* kName = "Server/Boss_Cowf.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Boss_Cowf.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Boss_Cowking) {
    static const char* kName = "Server/Boss_Cowking.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Boss_Cowking.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Boss_Emperor) {
    static const char* kName = "Server/Boss_Emperor.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Boss_Emperor.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Boss_EmperorJr) {
    static const char* kName = "Server/Boss_EmperorJr.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Boss_EmperorJr.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_DropRate) {
    static const char* kName = "Server/DropRate.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/DropRate.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_ExpPenalty) {
    static const char* kName = "Server/ExpPenalty.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/ExpPenalty.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_FieldBossDropItemList) {
    static const char* kName = "Server/FieldBossDropItemList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/FieldBossDropItemList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_FieldBossList) {
    static const char* kName = "Server/FieldBossList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/FieldBossList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_FortWarInfo) {
    static const char* kName = "Server/FortWarInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/FortWarInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_GameEventInfo) {
    static const char* kName = "Server/GameEventInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/GameEventInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_GuildPointRule) {
    static const char* kName = "Server/GuildPointRule.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/GuildPointRule.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_GuildTournamentInfo) {
    static const char* kName = "Server/GuildTournamentInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/GuildTournamentInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_HiLevelItemMixRate) {
    static const char* kName = "Server/HiLevelItemMixRate.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/HiLevelItemMixRate.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_HideNpcList) {
    static const char* kName = "Server/HideNpcList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/HideNpcList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_ItemChangeList) {
    static const char* kName = "Server/ItemChangeList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/ItemChangeList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_ItemChangeListMulti) {
    static const char* kName = "Server/ItemChangeListMulti.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/ItemChangeListMulti.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_ItemChangeRateofLv) {
    static const char* kName = "Server/ItemChangeRateofLv.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/ItemChangeRateofLv.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_ItemLimitInfo) {
    static const char* kName = "Server/ItemLimitInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/ItemLimitInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_ItemMixList) {
    static const char* kName = "Server/ItemMixList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/ItemMixList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Item_RareOptionInfo) {
    static const char* kName = "Server/Item_RareOptionInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Item_RareOptionInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Item_RareStatSetRate) {
    static const char* kName = "Server/Item_RareStatSetRate.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Item_RareStatSetRate.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Jackpot_Info) {
    static const char* kName = "Server/Jackpot_Info.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Jackpot_Info.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_LoginPoint) {
    static const char* kName = "Server/LoginPoint.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/LoginPoint.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_MapDropItemList) {
    static const char* kName = "Server/MapDropItemList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/MapDropItemList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_MapDropListInitDate) {
    static const char* kName = "Server/MapDropListInitDate.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/MapDropListInitDate.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_MonsterDropItemList) {
    static const char* kName = "Server/MonsterDropItemList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/MonsterDropItemList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_MonsterSpeechInfoList) {
    static const char* kName = "Server/MonsterSpeechInfoList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/MonsterSpeechInfoList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_02) {
    static const char* kName = "Server/Monster_02.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_02.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_04) {
    static const char* kName = "Server/Monster_04.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_04.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_08) {
    static const char* kName = "Server/Monster_08.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_08.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_09) {
    static const char* kName = "Server/Monster_09.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_09.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_10) {
    static const char* kName = "Server/Monster_10.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_10.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_101) {
    static const char* kName = "Server/Monster_101.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_101.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_102) {
    static const char* kName = "Server/Monster_102.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_102.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_104) {
    static const char* kName = "Server/Monster_104.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_104.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_105) {
    static const char* kName = "Server/Monster_105.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_105.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_106) {
    static const char* kName = "Server/Monster_106.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_106.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_108) {
    static const char* kName = "Server/Monster_108.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_108.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_11) {
    static const char* kName = "Server/Monster_11.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_11.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_13) {
    static const char* kName = "Server/Monster_13.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_13.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_14) {
    static const char* kName = "Server/Monster_14.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_14.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_15) {
    static const char* kName = "Server/Monster_15.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_15.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_17) {
    static const char* kName = "Server/Monster_17.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_17.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_18) {
    static const char* kName = "Server/Monster_18.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_18.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_19) {
    static const char* kName = "Server/Monster_19.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_19.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_20) {
    static const char* kName = "Server/Monster_20.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_20.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_21) {
    static const char* kName = "Server/Monster_21.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_21.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_22) {
    static const char* kName = "Server/Monster_22.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_22.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_24) {
    static const char* kName = "Server/Monster_24.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_24.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_25) {
    static const char* kName = "Server/Monster_25.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_25.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_26) {
    static const char* kName = "Server/Monster_26.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_26.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_32) {
    static const char* kName = "Server/Monster_32.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_32.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_34) {
    static const char* kName = "Server/Monster_34.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_34.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_35) {
    static const char* kName = "Server/Monster_35.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_35.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_39) {
    static const char* kName = "Server/Monster_39.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_39.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_40) {
    static const char* kName = "Server/Monster_40.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_40.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_59) {
    static const char* kName = "Server/Monster_59.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_59.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_60) {
    static const char* kName = "Server/Monster_60.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_60.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_61) {
    static const char* kName = "Server/Monster_61.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_61.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_65) {
    static const char* kName = "Server/Monster_65.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_65.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_66) {
    static const char* kName = "Server/Monster_66.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_66.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_67) {
    static const char* kName = "Server/Monster_67.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_67.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_68) {
    static const char* kName = "Server/Monster_68.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_68.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_69) {
    static const char* kName = "Server/Monster_69.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_69.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_70) {
    static const char* kName = "Server/Monster_70.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_70.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_72) {
    static const char* kName = "Server/Monster_72.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_72.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Monster_75) {
    static const char* kName = "Server/Monster_75.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Monster_75.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Mooun) {
    static const char* kName = "Server/Mooun.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Mooun.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_PaneltyTime) {
    static const char* kName = "Server/PaneltyTime.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/PaneltyTime.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_PetSpeechInfoList) {
    static const char* kName = "Server/PetSpeechInfoList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/PetSpeechInfoList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_PlayerxMonsterPoint) {
    static const char* kName = "Server/PlayerxMonsterPoint.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/PlayerxMonsterPoint.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_PlustimeInfo_N) {
    static const char* kName = "Server/PlustimeInfo_N.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/PlustimeInfo_N.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_PlustimeInfo__) {
    static const char* kName = "Server/PlustimeInfo__.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/PlustimeInfo__.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType1) {
    static const char* kName = "Server/RegenType1.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType1.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType11) {
    static const char* kName = "Server/RegenType11.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType11.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType12) {
    static const char* kName = "Server/RegenType12.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType12.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType13) {
    static const char* kName = "Server/RegenType13.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType13.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType14) {
    static const char* kName = "Server/RegenType14.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType14.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType15) {
    static const char* kName = "Server/RegenType15.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType15.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType16) {
    static const char* kName = "Server/RegenType16.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType16.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType17) {
    static const char* kName = "Server/RegenType17.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType17.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType18) {
    static const char* kName = "Server/RegenType18.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType18.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType2) {
    static const char* kName = "Server/RegenType2.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType2.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType3) {
    static const char* kName = "Server/RegenType3.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType3.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType4) {
    static const char* kName = "Server/RegenType4.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType4.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType5) {
    static const char* kName = "Server/RegenType5.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType5.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType6) {
    static const char* kName = "Server/RegenType6.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType6.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType7) {
    static const char* kName = "Server/RegenType7.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType7.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType8) {
    static const char* kName = "Server/RegenType8.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType8.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_RegenType9) {
    static const char* kName = "Server/RegenType9.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/RegenType9.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_SWRelationMap) {
    static const char* kName = "Server/SWRelationMap.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/SWRelationMap.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_SiegeWarStartTime) {
    static const char* kName = "Server/SiegeWarStartTime.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/SiegeWarStartTime.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Summon1) {
    static const char* kName = "Server/Summon1.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Summon1.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Summon2) {
    static const char* kName = "Server/Summon2.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Summon2.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Summon3) {
    static const char* kName = "Server/Summon3.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Summon3.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Summon4) {
    static const char* kName = "Server/Summon4.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Summon4.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_SummonList) {
    static const char* kName = "Server/SummonList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/SummonList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_SuryunRegenList) {
    static const char* kName = "Server/SuryunRegenList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/SuryunRegenList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_TacticAbilityInfo) {
    static const char* kName = "Server/TacticAbilityInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/TacticAbilityInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_TitanMapDropInfo) {
    static const char* kName = "Server/TitanMapDropInfo.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/TitanMapDropInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_TitanServer) {
    static const char* kName = "Server/TitanServer.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/TitanServer.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_TitanSpellCostPerMap) {
    static const char* kName = "Server/TitanSpellCostPerMap.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/TitanSpellCostPerMap.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseServer, ReadMhBin_Server_Weather) {
    static const char* kName = "Server/Weather.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "Server/Weather.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseQuestScript, ReadMhBin_QuestScript_NewbieGuide) {
    static const char* kName = "QuestScript/NewbieGuide.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/NewbieGuide.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseQuestScript, ReadMhBin_QuestScript_QuestItemList) {
    static const char* kName = "QuestScript/QuestItemList.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/QuestItemList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseQuestScript, ReadMhBin_QuestScript_QuestRegen) {
    static const char* kName = "QuestScript/QuestRegen.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/QuestRegen.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseQuestScript, ReadMhBin_QuestScript_QuestScript) {
    static const char* kName = "QuestScript/QuestScript.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/QuestScript.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseQuestScript, ReadMhBin_QuestScript_QuestString) {
    static const char* kName = "QuestScript/QuestString.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/QuestString.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParseQuestScript, ReadMhBin_QuestScript_questnpclist) {
    static const char* kName = "QuestScript/questnpclist.bin";
    const auto dir = find_resource_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/questnpclist.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_AbilityBaseInfo) {
    static const char* kName = "AbilityBaseInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "AbilityBaseInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_AbilityCalcInfo) {
    static const char* kName = "AbilityCalcInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "AbilityCalcInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_AvatarEquip) {
    static const char* kName = "AvatarEquip.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "AvatarEquip.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_CastleGateList) {
    static const char* kName = "CastleGateList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "CastleGateList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_CharacterExpPoint) {
    static const char* kName = "CharacterExpPoint.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "CharacterExpPoint.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_CostumeSkinItemList) {
    static const char* kName = "CostumeSkinItemList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "CostumeSkinItemList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_Dealitem) {
    static const char* kName = "Dealitem.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "Dealitem.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_FilterWord) {
    static const char* kName = "FilterWord.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "FilterWord.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_FlagNpcInfo) {
    static const char* kName = "FlagNpcInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "FlagNpcInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_GuildLevel) {
    static const char* kName = "GuildLevel.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "GuildLevel.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_GuildPointPlustimeList) {
    static const char* kName = "GuildPointPlustimeList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "GuildPointPlustimeList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_ItemList) {
    static const char* kName = "ItemList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "ItemList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_ItemMixList) {
    static const char* kName = "ItemMixList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "ItemMixList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_Item_RareItemInfo) {
    static const char* kName = "Item_RareItemInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "Item_RareItemInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_ItemdupOption) {
    static const char* kName = "ItemdupOption.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "ItemdupOption.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_JobSkillList) {
    static const char* kName = "JobSkillList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "JobSkillList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_Jobskill) {
    static const char* kName = "Jobskill.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "Jobskill.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_KyungGongInfo) {
    static const char* kName = "KyungGongInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "KyungGongInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_MapChange) {
    static const char* kName = "MapChange.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "MapChange.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_MapKindInfo) {
    static const char* kName = "MapKindInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "MapKindInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_MonsterDropItemList) {
    static const char* kName = "MonsterDropItemList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "MonsterDropItemList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_MonsterList) {
    static const char* kName = "MonsterList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "MonsterList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_NpcList) {
    static const char* kName = "NpcList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "NpcList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_PaneltyTime) {
    static const char* kName = "PaneltyTime.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "PaneltyTime.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_PartyPlustimeInfo) {
    static const char* kName = "PartyPlustimeInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "PartyPlustimeInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_PetBuffList) {
    static const char* kName = "PetBuffList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "PetBuffList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_PetList) {
    static const char* kName = "PetList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "PetList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_PetRule) {
    static const char* kName = "PetRule.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "PetRule.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_PyogukListInfo) {
    static const char* kName = "PyogukListInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "PyogukListInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_Setitem_Opt) {
    static const char* kName = "Setitem_Opt.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "Setitem_Opt.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_SiegeWarMapInfo) {
    static const char* kName = "SiegeWarMapInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "SiegeWarMapInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_SkillAreaList) {
    static const char* kName = "SkillAreaList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "SkillAreaList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_SkillList) {
    static const char* kName = "SkillList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "SkillList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_SkillOptionList) {
    static const char* kName = "SkillOptionList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "SkillOptionList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_SkillchangeList) {
    static const char* kName = "SkillchangeList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "SkillchangeList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_SkinSelectItemList) {
    static const char* kName = "SkinSelectItemList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "SkinSelectItemList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_StateInfo) {
    static const char* kName = "StateInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "StateInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_StaticNpc) {
    static const char* kName = "StaticNpc.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "StaticNpc.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_SuryunLevelChange) {
    static const char* kName = "SuryunLevelChange.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "SuryunLevelChange.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_SuryunMonster) {
    static const char* kName = "SuryunMonster.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "SuryunMonster.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_Suryundata) {
    static const char* kName = "Suryundata.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "Suryundata.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_TacticStartInfo) {
    static const char* kName = "TacticStartInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "TacticStartInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_TitanBreakList) {
    static const char* kName = "TitanBreakList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "TitanBreakList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_TitanList) {
    static const char* kName = "TitanList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "TitanList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_TitanMapDropInfo) {
    static const char* kName = "TitanMapDropInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "TitanMapDropInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_TitanMixList) {
    static const char* kName = "TitanMixList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "TitanMixList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_TitanPartsKind) {
    static const char* kName = "TitanPartsKind.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "TitanPartsKind.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_TitanRule) {
    static const char* kName = "TitanRule.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "TitanRule.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_TitanSpellCostPerMap) {
    static const char* kName = "TitanSpellCostPerMap.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "TitanSpellCostPerMap.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_TitanUpgradeInfo) {
    static const char* kName = "TitanUpgradeInfo.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "TitanUpgradeInfo.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_UniqueItemMixList) {
    static const char* kName = "UniqueItemMixList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "UniqueItemMixList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_UniqueItemOptionList) {
    static const char* kName = "UniqueItemOptionList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "UniqueItemOptionList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_hideitemlock) {
    static const char* kName = "hideitemlock.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "hideitemlock.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_itemReinforceList) {
    static const char* kName = "itemReinforceList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "itemReinforceList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_item_RareReinforceList) {
    static const char* kName = "item_RareReinforceList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "item_RareReinforceList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_itembreak) {
    static const char* kName = "itembreak.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "itembreak.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_sat) {
    static const char* kName = "sat.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "sat.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_skillTree) {
    static const char* kName = "skillTree.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "skillTree.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_QuestScript_NewbieGuide) {
    static const char* kName = "QuestScript/NewbieGuide.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/NewbieGuide.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_QuestScript_QuestItemList) {
    static const char* kName = "QuestScript/QuestItemList.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/QuestItemList.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_QuestScript_QuestRegen) {
    static const char* kName = "QuestScript/QuestRegen.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/QuestRegen.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_QuestScript_QuestScript) {
    static const char* kName = "QuestScript/QuestScript.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/QuestScript.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_QuestScript_QuestScript_) {
    static const char* kName = "QuestScript/QuestScript_.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/QuestScript_.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_QuestScript_QuestString) {
    static const char* kName = "QuestScript/QuestString.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/QuestString.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


TEST(MxhResourceParsePlayDh, ReadMhBin_QuestScript_questnpclist) {
    static const char* kName = "QuestScript/questnpclist.bin";
    const auto dir = find_playdh_dir();
    if (dir.empty()) GTEST_SKIP() << "QuestScript/questnpclist.bin base not available";
    const auto p = dir / kName;
    if (!fs::exists(p)) GTEST_SKIP() << kName << " not present";
    const auto r = mxh::compat::read_mh_bin(p);
    ASSERT_TRUE(r.ok()) << kName << " err=" << static_cast<int>(r.error);
    EXPECT_EQ(r.value.data.size(), r.value.header.file_size) << kName << " payload size mismatch";
    EXPECT_GT(r.value.header.file_size, 0u) << kName;
    EXPECT_LE(r.value.header.file_size, 256u * 1024u * 1024u) << kName;
}


