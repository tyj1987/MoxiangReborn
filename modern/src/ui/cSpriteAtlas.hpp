// mxh/ui/cSpriteAtlas.hpp
// M-R2: 现代 sprite atlas 池 (老版 [Client]MH/interface/cResourceManager.cpp Init 等价物)
//
// 老版机制 (实地查证 cResourceManager.cpp):
//   1. Init(szImageListPath, szMsgListPath) 读 image list (.bin):
//        - header(12B) + CRC
//        - 第 1 行: 总数 N
//        - 接下来 N 行: id, filename, width, height, layer
//   2. GetImageInfo(idx) lazy 装载:
//        m_pImageInfoArray[idx].pSpriteObject = renderer->CreateSpriteObject(filename)
//   3. GetInfo(idx) 拿 IMAGE_NODE (含 szFileName + size + layer)
//
// M-R2 范围: 装 image list + 查文件名 + 索引对应 cResourceManager hard path 的 atlas_idx。
// GetSpriteObject 留给 MoxianClient runtime 调 (需要 ID3D11Device)。

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace mxh::ui {

struct ImageInfo {
    std::int32_t idx = 0;
    std::string  filename;  // e.g. "./image/2D/1.tif"
    std::int32_t width  = 0;
    std::int32_t height = 0;
    std::int32_t layer  = 0;
};

class cSpriteAtlas {
public:
    static cSpriteAtlas& getInstance() noexcept;
    cSpriteAtlas(const cSpriteAtlas&) = delete;
    cSpriteAtlas& operator=(const cSpriteAtlas&) = delete;

    // 装载 image_path.bin
    // path_root = <PlayDH> (路径会自动加 /Image/image_path.bin)
    bool Init(const std::filesystem::path& path_root);
    void Release() noexcept;

    // 老版 GetInfo(idx) - 1:1
    [[nodiscard]] std::optional<ImageInfo> getInfo(std::int32_t idx) const noexcept;

    // 实际绝对路径 = path_root / filename (strip "./" prefix)
    [[nodiscard]] std::filesystem::path resolvePath(const ImageInfo& info) const;

    [[nodiscard]] std::size_t size() const noexcept { return m_entries.size(); }
    [[nodiscard]] bool loaded() const noexcept { return m_loaded; }
    [[nodiscard]] const std::filesystem::path& pathRoot() const noexcept { return m_pathRoot; }

private:
    cSpriteAtlas() = default;
    ~cSpriteAtlas() = default;

    std::vector<ImageInfo> m_entries;
    std::filesystem::path m_pathRoot;
    bool m_loaded = false;
};

}  // namespace mxh::ui
