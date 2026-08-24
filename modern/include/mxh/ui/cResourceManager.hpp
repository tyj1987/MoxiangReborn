// mxh/ui/cResourceManager.hpp
// 现代 cResourceManager: 老版 [Client]MH/interface/cScriptManager.cpp
// InitScriptManager() + GetImage() 的现代等价物。
//
// 老版机制（实地查证 cScriptManager.cpp）：
//   1. 启动时打开 7 个 .bin (image_hard_path / image_item_path / image_mugong_path /
//      image_ability_path / image_buff_path / image_minimap_path / image_jackpot_path)
//   2. 每张表解密 (MHFile byte-level -= i, if i%type==0 -= type)
//   3. 解析文本格式 (6 字段 / 行: index, atlas_idx, left, top, right, bottom)
//   4. 装到 7 个 CYHHashTable<sIMAGHARDPATH> 里
//   5. dialog Init 时调 GetImage(hard_idx, cImage*, ePATH_FILE_TYPE) →
//      RESRCMGR->GetImageInfo(idx) → pImage->SetSpriteObject(atlas[idx])
//
// M-R1 范围: 实现装载 + 查询。等 M-R2 (sprite atlas pool) 接入 GetImageInfo()。

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mxh::ui {

// 1:1 镜像老版 sIMAGHARDPATH (cScriptManager.h:38-44)
// atlas_idx = 硬路径在 IMAGE 表里的索引
// left/top/right/bottom = 切片在 atlas sprite 里的像素矩形
struct ImageHardPath {
    std::int32_t atlas_idx = 0;  // 老版 sIMAGHARDPATH.idx
    std::int32_t left   = 0;
    std::int32_t top    = 0;
    std::int32_t right  = 0;
    std::int32_t bottom = 0;

    [[nodiscard]] std::int32_t width()  const noexcept { return right - left; }
    [[nodiscard]] std::int32_t height() const noexcept { return bottom - top; }
};

// 老版 ePATH_FILE_TYPE (cScriptManager.h:26-35)
enum class PathFileType : std::int32_t {
    HardPath   = 0,  // image_hard_path.bin  (dialog 节点基本图)
    ItemPath   = 1,  // image_item_path.bin  (物品 icon)
    MugongPath = 2,  // image_mugong_path.bin
    AbilityPath= 3,
    BuffPath   = 4,
    MiniMapPath= 5,
    JackpotPath= 6,
};

// 1 张装载错误
struct LoadReport {
    PathFileType type;
    std::filesystem::path path;
    std::size_t records = 0;
    bool ok = false;
    std::string error;
};

class cResourceManager {
public:
    static cResourceManager& getInstance() noexcept;
    cResourceManager(const cResourceManager&) = delete;
    cResourceManager& operator=(const cResourceManager&) = delete;

    // 启动时一次调用: 装载 7 张 hard path .bin
    // path_root 通常 = <PlayDH>/Image/
    bool InitScriptManager(const std::filesystem::path& path_root);

    // 老版 ReleaseScriptManager
    void ReleaseScriptManager() noexcept;

    // 老版 GetImage(hard_idx, cImage*, ePATH_FILE_TYPE)
    // 查 hard path 表 → 出 ImageHardPath (atlas_idx + rect)
    // 1:1 with cScriptManager.cpp:501
    std::optional<ImageHardPath> getHardPath(std::int32_t hard_idx,
                                            PathFileType type) const noexcept;

    // 单表大小（用于单元测试 + 报告）
    [[nodiscard]] std::size_t sizeOf(PathFileType type) const noexcept;

    // 总装载统计 (供 visual-baseline / commit message 用)
    [[nodiscard]] const std::vector<LoadReport>& reports() const noexcept { return m_reports; }

    // 7 张表都装载成功的检查
    [[nodiscard]] bool allLoaded() const noexcept;

private:
    cResourceManager() = default;
    ~cResourceManager() = default;

    // 单张表装载
    bool loadTable(PathFileType type, const std::filesystem::path& bin_path);

    // 7 张表各自存
    using Table = std::unordered_map<std::int32_t, ImageHardPath>;
    std::array<Table, 7> m_tables{};
    std::vector<LoadReport> m_reports;
    bool m_loaded = false;
};

}  // namespace mxh::ui
