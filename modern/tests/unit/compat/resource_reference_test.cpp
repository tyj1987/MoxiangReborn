#include <gtest/gtest.h>

#include <windows.h>
#include <bcrypt.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace fs = std::filesystem;

namespace {
std::string sha256(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
        return {};
    DWORD objectLength = 0;
    DWORD resultLength = 0;
    if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength),
                          &resultLength, 0) != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0); return {};
    }
    std::vector<UCHAR> object(objectLength);
    std::vector<UCHAR> digest(32);
    if (BCryptCreateHash(algorithm, &hash, object.data(), objectLength, nullptr, 0, 0) != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0); return {};
    }
    std::vector<char> buffer(64 * 1024);
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = in.gcount();
        if (count > 0 && BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()),
                                         static_cast<ULONG>(count), 0) != 0) {
            BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(algorithm, 0); return {};
        }
    }
    const bool ok = BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) == 0;
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!ok) return {};
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto byte : digest) out << std::setw(2) << static_cast<unsigned>(byte);
    return out.str();
}

fs::path resource_root(const fs::path& root) {
    return root / "modern" / "data" / "PlayDH" / "Resource";
}

fs::path recovered_server_root(const fs::path& root) {
    return root / "reference" / "legacy-source" / "4dddd9a6" /
           "SWorking" / "Resource" / "Server";
}

fs::path repo_root() {
    auto p = fs::current_path();
    for (int i = 0; i < 8 && !p.empty(); ++i, p = p.parent_path())
        if (fs::exists(p / "modern") && fs::exists(p / "deploy")) return p;
    return {};
}

struct Ref {
    const wchar_t* relative_name;
    const char* hash;
    std::uintmax_t size;
};

// These are the immutable canonical PlayDH bytes used by the 1:1 build.
const Ref refs[] = {
    {L"SkillList.bin",  "9b5d1fac408c610252e419f6c55b12e3bc38f9ed436fdebbdc01ec664da906b7", 769649},
    {L"MonsterList.bin", "fb7ee93e66ea9321577fe4a5031e98689d346b6fdbd5852e05d69ad7a952eebe", 142234},
    {L"ItemList.bin",   "07d25fb98ee7f02aae3b5950ab4472847742d989775078985576c7c94a3957bd", 1510488},
    {L"ItemMixList.bin", "4ead8cd9f4665ab97b1540df1dab75c2a0375f42ac748b4a6e4321b76acd634a", 514261},
    {L"MapKindInfo.bin", "9af05426844b08bc8c624b04da2413a7ef23ec5025a66af0f7679b0c378f55f8", 1650},
    {L"AbilityBaseInfo.bin", "900d97e6afdaf2c1dbb0bad91264966c7e8ad28b3ded33ff7b3577d815455f62", 12684},
    {L"JobSkillList.bin", "7b306270acc1a67d2025c80b11582dfa125afc1442310354636133ef41ab8864", 278},
    {L"CharacterExpPoint.bin", "8010160a2ed9a7e0bac91c73eb31efa43d25d101200d1cb149be5e8083305f59", 3519},
    {L"AvatarEquip.bin", "32413ec55572bd58cb687ec6a5089ca7934107e06aa7112a20a176e3cb757605", 26751},
    {L"Dealitem.bin", "5d5e8023d0071da3e1da0161c75cc98ffcd9c0540b782422f3fb426a8ba04a2a", 166939},
    {L"MonsterDropItemList.bin", "a65baf6ec5ffae8e3af8ec2c9fd9a4878a8773f69624aea1cafa594fc07e858e", 196253},
    {L"TacticStartInfo.bin", "2fa652d7429be2ae860f46beeaf0212ad109b203fa4e123a9350d458c67d7000", 1514},
    {L"TitanList.bin", "8a7ff80ec1cafcdbf343d30bedb866715e4391293c1667bec388a6cae8f07a8d", 362},
    {L"MapChange.bin", "66aca3ccca86469e4f8ec1f0dada451fd2422d5c4b0c2698408e56c0101b7048", 9692},
    {L"QuestScript/QuestScript.bin", "82bcdf96770bdf09e8b56e57d48bcffded0169e301967a115cb6336d5e97ba1e", 238478},
};
}

TEST(MxhResourceReference, PlayDhFilesMatchReferenceManifest) {
    const auto root = repo_root();
    ASSERT_FALSE(root.empty());
    const auto resource = resource_root(root);
    ASSERT_FALSE(resource.empty());
    for (const auto& ref : refs) {
        const auto path = resource / ref.relative_name;
        ASSERT_TRUE(fs::exists(path)) << path.string();
        ASSERT_EQ(fs::file_size(path), ref.size) << path.string();
        EXPECT_EQ(sha256(path), ref.hash) << path.string();
    }
}

TEST(MxhResourceReference, PlayDhServerArchiveFilesMatchReferenceManifest) {
    const auto root = repo_root();
    ASSERT_FALSE(root.empty());
    struct ServerRef { const char* name; const char* hash; std::uintmax_t size; };
    const ServerRef server_refs[] = {
        {"Monster_10.bin", "50033323336100da141443f3b563c62312586d149edf5cfdd78262b26d15cc01", 22766},
        {"Monster_12.bin", "5be52f14930a81743b1596694c5d5d3eb39f5fd990371f5cf5cb6642b598d966", 14},
        {"PlayerxMonsterPoint.bin", "7560dd9f486a3d93f1e134f3eaf45ce2264443424526b8b0239928dd53936148", 16403},
    };
    for (const auto& ref : server_refs) {
        const auto path = resource_root(root) / "Server" / ref.name;
        ASSERT_TRUE(fs::exists(path)) << path.string();
        ASSERT_EQ(fs::file_size(path), ref.size) << path.string();
        EXPECT_EQ(sha256(path), ref.hash) << path.string();
    }
}

TEST(MxhResourceReference, RecoveredRuntimeServerFilesMatchReferenceManifest) {
    const auto root = repo_root();
    ASSERT_FALSE(root.empty());
    struct RuntimeRef { const char* name; const char* hash; std::uintmax_t size; };
    const RuntimeRef runtime_refs[] = {
        {"Monster_10.bin", "a029bd886b503a22af9032a9ae78ac1c37aaa06e058a9ce457caf163a2f01153", 22763},
        {"Monster_12.bin", "5be52f14930a81743b1596694c5d5d3eb39f5fd990371f5cf5cb6642b598d966", 14},
        {"PlayerxMonsterPoint.bin", "32b11dc34ae0b16091c37ce31f0115e4f7aee518801957a4e12a1c1a80c9350d", 6644},
    };
    for (const auto& ref : runtime_refs) {
        const auto path = recovered_server_root(root) / ref.name;
        ASSERT_TRUE(fs::exists(path)) << "required VHD-recovered runtime resource missing: " << path.string();
        ASSERT_EQ(fs::file_size(path), ref.size) << path.string();
        EXPECT_EQ(sha256(path), ref.hash) << path.string();
    }
}

TEST(MxhResourceReference, CanonicalRootIsExplicitAndDoesNotDependOnTreeOrder) {
    const auto root = repo_root();
    ASSERT_FALSE(root.empty());
    const auto expected = root / "modern" / "data" / "PlayDH" / "Resource";
    EXPECT_EQ(fs::weakly_canonical(resource_root(root)), fs::weakly_canonical(expected));
    EXPECT_TRUE(fs::exists(expected / "SkillList.bin"));
}
