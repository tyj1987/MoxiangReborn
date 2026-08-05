// T1 expansion - read_mh_bin coverage. Verifies modern can
// fully parse all standard .bin files (not just header sniff).
//
// Phase E1 / C-Batch-E1: targets all 59 deploy/Resource .bin files
// (the legacy server shipping set). Each file is opened via
// read_mh_bin, expected MhError::Ok, header sanity-checked, and
// payload size asserted against header.file_size.

#include "mxh/compat/mh_file_ex.hpp"

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

