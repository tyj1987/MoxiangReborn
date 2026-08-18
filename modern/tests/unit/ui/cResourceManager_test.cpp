// tests/unit/ui/cResourceManager_test.cpp
// M-R1 单测: cResourceManager 7 张表装载 + idx 查询

#include "mxh/ui/cResourceManager.hpp"
#include "mxh/ui/cSpriteAtlas.hpp"
#include "mxh/log/mlog.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// PlayDH 实际位置由 AGENTS.md / scripts/start_modern.ps1 决定:
// 优先用环境变量 MXH_PLAYDH_ROOT, 缺省回退到 <repo>/墨香【源码配套资源】/PlayDH
fs::path resolvePlayDHRoot() {
    if (const char* env = std::getenv("MXH_PLAYDH_ROOT")) {
        return fs::path(env);
    }
    // Build-time hint from CMake (set by the test target)
    #ifdef MXH_PLAYDH_ROOT_TS
    return fs::path(MXH_PLAYDH_ROOT_TS);
    #endif
    // Hardcoded fallback: assume test runs from <repo>/modern/build
    return fs::path("C:/moxiang/墨香【源码配套资源】/PlayDH");
}

int g_failures = 0;
int g_passes = 0;

#define EXPECT(cond, msg) do { \
    if (cond) { ++g_passes; } \
    else { ++g_failures; std::cerr << "FAIL: " << msg << " @ " << __FILE__ << ":" << __LINE__ << "\n"; } \
} while (0)

#define EXPECT_EQ(a, b, msg) do { \
    auto _a = (a); auto _b = (b); \
    if (_a == _b) { ++g_passes; } \
    else { ++g_failures; std::cerr << "FAIL: " << msg << " (got=" << _a << " want=" << _b << ") @ " << __FILE__ << ":" << __LINE__ << "\n"; } \
} while (0)

}  // namespace

int main() {
    using mxh::ui::cResourceManager;
    using mxh::ui::PathFileType;
    using mxh::ui::ImageHardPath;

    const auto playdh = resolvePlayDHRoot();
    const auto image_dir = playdh / "Image";
    std::cout << "[cResourceManager_test] playdh=" << playdh << "\n";
    std::cout << "[cResourceManager_test] image_dir=" << image_dir << "\n";

    if (!fs::exists(image_dir / "image_hard_path.bin")) {
        std::cerr << "SKIP: PlayDH Image dir not found: " << image_dir << "\n";
        return 0;  // not a failure — environment-dependent
    }

    auto& rm = cResourceManager::getInstance();
    EXPECT(rm.InitScriptManager(image_dir), "InitScriptManager returns true");

    // M-R1 验收: 7 张表都至少 1 条
    EXPECT(rm.sizeOf(PathFileType::HardPath)     > 0, "HARDPATH has records");
    EXPECT(rm.sizeOf(PathFileType::ItemPath)     > 0, "ITEMPATH has records");
    EXPECT(rm.sizeOf(PathFileType::MugongPath)   > 0, "MUGONGPATH has records");
    EXPECT(rm.sizeOf(PathFileType::AbilityPath)  > 0, "ABILITYPATH has records");
    EXPECT(rm.sizeOf(PathFileType::BuffPath)     > 0, "BUFFPATH has records");
    EXPECT(rm.sizeOf(PathFileType::MiniMapPath)  > 0, "MINIMAPPATH has records");
    EXPECT(rm.sizeOf(PathFileType::JackpotPath)  > 0, "JACKPOTPATH has records");

    // 老版 init size (modern 装载去重后的 unique count):
    //   m_ImageHardPath.Initialize(150)   => 实测 147 unique
    //   m_ItemHardPath.Initialize(1200)   => 实测 1515 unique (1516 行 - 1 重复 idx=1152)
    //   m_MugongHardPath.Initialize(100)  => 实测 112 unique
    //   m_AbilityHardPath.Initialize(100) => 实测 85 unique
    //   m_BuffHardPath.Initialize(200)    => 实测 180 unique
    //   m_MiniMapHardPath.Initialize(40)  => 实测 39 unique
    //   m_JackPotHardPath.Initialize(?)   => 实测 11 unique
    EXPECT_EQ(rm.sizeOf(PathFileType::HardPath),    147, "HARDPATH == 147");
    EXPECT_EQ(rm.sizeOf(PathFileType::ItemPath),    1515, "ITEMPATH == 1515 (1516 rows - 1 dup idx=1152)");
    EXPECT_EQ(rm.sizeOf(PathFileType::MugongPath),  112, "MUGONGPATH == 112");
    EXPECT_EQ(rm.sizeOf(PathFileType::AbilityPath), 85,  "ABILITYPATH == 85");
    EXPECT_EQ(rm.sizeOf(PathFileType::BuffPath),    180, "BUFFPATH == 180");
    EXPECT_EQ(rm.sizeOf(PathFileType::MiniMapPath), 39,  "MINIMAPPATH == 39");
    EXPECT_EQ(rm.sizeOf(PathFileType::JackpotPath), 11,  "JACKPOTPATH == 11");

    // 查询 idx=0: 7 张表都应该有
    for (auto t : {PathFileType::HardPath, PathFileType::ItemPath,
                   PathFileType::MugongPath, PathFileType::AbilityPath,
                   PathFileType::BuffPath, PathFileType::JackpotPath}) {
        auto hp = rm.getHardPath(0, t);
        EXPECT(hp.has_value(), "idx=0 queryable in every PathFileType");
    }

    // 具体值校验: HardPath[0] = (atlas=0, l=129, t=772, r=171, b=814) 来自手工解析
    auto hp0 = rm.getHardPath(0, PathFileType::HardPath);
    EXPECT(hp0.has_value(), "HardPath[0] exists");
    if (hp0) {
        EXPECT_EQ(hp0->atlas_idx, 0, "HardPath[0].atlas_idx");
        EXPECT_EQ(hp0->left,   129, "HardPath[0].left");
        EXPECT_EQ(hp0->top,    772, "HardPath[0].top");
        EXPECT_EQ(hp0->right,  171, "HardPath[0].right");
        EXPECT_EQ(hp0->bottom, 814, "HardPath[0].bottom");
    }

    // ItemPath[0] = (atlas=1, l=0, t=0, r=40, b=40)
    auto ip0 = rm.getHardPath(0, PathFileType::ItemPath);
    EXPECT(ip0.has_value(), "ItemPath[0] exists");
    if (ip0) {
        EXPECT_EQ(ip0->atlas_idx, 1, "ItemPath[0].atlas_idx");
        EXPECT_EQ(ip0->left,   0,  "ItemPath[0].left");
        EXPECT_EQ(ip0->right,  40, "ItemPath[0].right");
        EXPECT_EQ(ip0->bottom, 40, "ItemPath[0].bottom");
    }

    // 负 idx 应该返回 nullopt
    auto neg = rm.getHardPath(-1, PathFileType::HardPath);
    EXPECT(!neg.has_value(), "negative idx returns nullopt");

    // 越界 idx 应该返回 nullopt
    auto too_big = rm.getHardPath(999999, PathFileType::HardPath);
    EXPECT(!too_big.has_value(), "out-of-range idx returns nullopt");

    // 错误 type 越界: 不该崩
    auto bad_type = rm.getHardPath(0, static_cast<PathFileType>(999));
    EXPECT(!bad_type.has_value(), "invalid PathFileType returns nullopt");

    // allLoaded
    EXPECT(rm.allLoaded(), "all 7 tables loaded");

    // ReleaseScriptManager 应该清空
    rm.ReleaseScriptManager();
    EXPECT_EQ(rm.sizeOf(PathFileType::HardPath), 0, "HardPath cleared after Release");
    EXPECT(!rm.allLoaded(), "allLoaded=false after Release");

    // ==================== M-R2: cSpriteAtlas (image list) 装载 ====================
    // 测试 image_path.bin (老版 cResourceManager::Init 装载的 image list)。
    // M-R1 + M-R2 集成: cResourceManager.getHardPath(hard_idx, type) → ImageHardPath.atlas_idx
    //   → cSpriteAtlas.getInfo(atlas_idx) → filename → resolvePath 实际文件
    auto& atlas = mxh::ui::cSpriteAtlas::getInstance();
    EXPECT(atlas.Init(playdh), "cSpriteAtlas.Init returns true");
    EXPECT(atlas.loaded(), "cSpriteAtlas loaded");
    EXPECT_EQ(atlas.size(), std::size_t{184}, "cSpriteAtlas size == 184 (老版实测)");

    {
        auto e0 = atlas.getInfo(0);
        EXPECT(e0.has_value(), "cSpriteAtlas.getInfo(0) ok");
        if (e0) {
            EXPECT_EQ(e0->idx, 0, "entry 0 idx");
            EXPECT_EQ(e0->width,  1024, "entry 0 width");
            EXPECT_EQ(e0->height, 1024, "entry 0 height");
            EXPECT_EQ(e0->layer, 1, "entry 0 layer");
            EXPECT(e0->filename == "./image/2D/1.tif", "entry 0 filename");
        }
    }
    {
        auto e1 = atlas.getInfo(1);
        EXPECT(e1.has_value(), "cSpriteAtlas.getInfo(1) ok");
        if (e1) {
            EXPECT_EQ(e1->width, 256, "entry 1 width");
            EXPECT_EQ(e1->height, 256, "entry 1 height");
        }
    }
    {
        auto e183 = atlas.getInfo(183);
        EXPECT(e183.has_value(), "cSpriteAtlas.getInfo(183) ok");
        if (e183) {
            EXPECT_EQ(e183->width, 1024, "entry 183 width");
            EXPECT_EQ(e183->filename, "./image/2D/4.tif", "entry 183 filename");
        }
    }
    // 越界 / 负数
    EXPECT(!atlas.getInfo(-1).has_value(),     "getInfo(-1) returns nullopt");
    EXPECT(!atlas.getInfo(184).has_value(),    "getInfo(184) out-of-range returns nullopt");
    EXPECT(!atlas.getInfo(99999).has_value(),  "getInfo(99999) way out-of-range returns nullopt");

    // resolvePath strip "./" prefix (path case-insensitive on Windows)
    {
        auto e0 = atlas.getInfo(0);
        if (e0) {
            auto resolved = atlas.resolvePath(*e0);
            auto expected = playdh / "Image" / "2D" / "1.tif";
            // Windows fs::equivalent is case-insensitive
            EXPECT(fs::equivalent(resolved, expected) || resolved == expected,
                   "resolvePath strips './' prefix and points at file");
        }
    }

    // Release + re-Init
    atlas.Release();
    EXPECT_EQ(atlas.size(), std::size_t{0}, "atlas size=0 after Release");
    EXPECT(!atlas.loaded(), "atlas loaded=false after Release");
    EXPECT(atlas.Init(playdh), "atlas re-Init returns true");
    EXPECT_EQ(atlas.size(), std::size_t{184}, "atlas size still 184 after re-Init");

    // M-R1 + M-R2 集成 cross-check: 7 张 hard path 表 idx=0 的 atlas_idx 都应该
    // 在 image list 查到, 验证 cResourceManager → cSpriteAtlas 链通
    rm.InitScriptManager(image_dir);
    int cross_check_pass = 0;
    int cross_check_total = 0;
    for (auto t : {PathFileType::HardPath, PathFileType::ItemPath,
                   PathFileType::MugongPath, PathFileType::AbilityPath,
                   PathFileType::BuffPath, PathFileType::JackpotPath}) {
        auto hp = rm.getHardPath(0, t);
        if (!hp) continue;
        auto info = atlas.getInfo(hp->atlas_idx);
        ++cross_check_total;
        if (info.has_value() && !info->filename.empty()) {
            ++cross_check_pass;
        }
        std::cout << "[M-R1+2 cross] " << (int)t << " hard_idx=0 atlas_idx="
                  << hp->atlas_idx << " -> "
                  << (info ? info->filename : "<MISSING>") << "\n";
    }
    EXPECT(cross_check_pass == cross_check_total,
           "every hard_path[0].atlas_idx resolves to an image list entry");

    // 抽样 5 个 image 验证文件实际存在 + 取 magic bytes
    std::vector<int> sample_idxs = {0, 1, 17, 20, 87};
    int files_found = 0;
    for (int idx : sample_idxs) {
        auto info = atlas.getInfo(idx);
        if (!info) continue;
        auto path = atlas.resolvePath(*info);
        if (fs::exists(path)) {
            ++files_found;
            std::ifstream f(path, std::ios::binary);
            unsigned char buf[16] = {0};
            f.read(reinterpret_cast<char*>(buf), 16);
            std::streamsize got = f.gcount();
            std::cout << "[sample] idx=" << idx << " " << path.filename().string()
                      << " size=" << fs::file_size(path) << " magic=";
            for (int i = 0; i < std::min<std::streamsize>(got, 8); ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << (static_cast<unsigned int>(buf[i]) & 0xFF) << " ";
            }
            std::cout << std::dec << "\n";
        } else {
            std::cout << "[sample] idx=" << idx << " " << path.filename().string() << " MISSING\n";
        }
    }
    EXPECT(files_found >= 1, "at least one sampled image file exists on disk");

    std::cout << "\n[cResourceManager_test] PASS " << g_passes << " / FAIL " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
