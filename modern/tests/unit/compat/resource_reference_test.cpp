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
    std::error_code ec;
    std::vector<fs::path> candidates;
    for (auto it = fs::recursive_directory_iterator(
             root, fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec) || it->path().filename() != L"SkillList.bin") continue;
        auto pathText = it->path().parent_path().parent_path().wstring();
        if (pathText.find(L"PlayDH") != std::wstring::npos) return it->path().parent_path();
        candidates.push_back(it->path().parent_path());
    }
    return candidates.empty() ? fs::path{} : candidates.front();
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

// These are the immutable PlayDH reference bytes used by the 1:1 build.
// Keep deploy/server variants in the separate manifest below; they are not
// interchangeable with the original client resource set.
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

TEST(MxhResourceReference, DeployServerFilesMatchTheirReferenceManifest) {
    const auto root = repo_root();
    ASSERT_FALSE(root.empty());
    struct DeployRef { const char* name; const char* hash; };
    const DeployRef deploy_refs[] = {
        {"SkillList.bin", "6727903837346c783d9c6833d2bd9ab94d6fcba4e7e16d07d8bf75c280a2d280"},
        {"MonsterList.bin", "c4a6174487c4407277710504199da0113580029d4a12c38bc88fc3b4bad3a96e"},
        {"ItemList.bin", "07d25fb98ee7f02aae3b5950ab4472847742d989775078985576c7c94a3957bd"},
        {"AvatarEquip.bin", "55c5e8d3c23d24f314ba1a8f703091a9b13f7ed949cd62ebf9ea660040206c08"},
        {"CharacterExpPoint.bin", "ed581cbdd5ebf33e95c566c25355343a52ad3673066abc1b0118fb15c76cf90a"},
    };
    for (const auto& ref : deploy_refs) {
        const auto path = root / "deploy" / "server" / "Distribute" / "Resource" / ref.name;
        ASSERT_TRUE(fs::exists(path)) << path.string();
        EXPECT_EQ(sha256(path), ref.hash) << path.string();
    }
}

TEST(MxhResourceReference, KnownPlayDhAndDeployVariantsAreNotSilentlyMixed) {
    const auto root = repo_root();
    ASSERT_FALSE(root.empty());
    const auto play = resource_root(root) / L"SkillList.bin";
    const auto deploy = root / "deploy" / "server" / "Distribute" / "Resource" / "SkillList.bin";
    ASSERT_TRUE(fs::exists(play));
    ASSERT_TRUE(fs::exists(deploy));
    EXPECT_NE(sha256(play), sha256(deploy));
}
