// mxh/ui/cResourceManager.cpp
// M-R1: 现代 cResourceManager 老版 cScriptManager InitScriptManager 等价物

#include "mxh/ui/cResourceManager.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/log/mlog.hpp"

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace mxh::ui {

namespace {

// 老版 7 张表文件路径
struct HardPathFile {
    PathFileType type;
    const char* filename;  // relative to <PlayDH>/Image/
};

constexpr std::array<HardPathFile, 7> kHardPathFiles = {{
    {PathFileType::HardPath,    "image_hard_path.bin"},
    {PathFileType::ItemPath,    "image_item_path.bin"},
    {PathFileType::MugongPath,  "image_mugong_path.bin"},
    {PathFileType::AbilityPath, "image_ability_path.bin"},
    {PathFileType::BuffPath,    "image_buff_path.bin"},
    {PathFileType::MiniMapPath, "image_minimap_path.bin"},
    {PathFileType::JackpotPath, "image_jackpot_path.bin"},
}};

constexpr const char* kPathTypeName(PathFileType t) noexcept {
    switch (t) {
    case PathFileType::HardPath:    return "HARDPATH";
    case PathFileType::ItemPath:    return "ITEMPATH";
    case PathFileType::MugongPath:  return "MUGONGPATH";
    case PathFileType::AbilityPath: return "ABILILTYPATH";
    case PathFileType::BuffPath:    return "BUFFPATH";
    case PathFileType::MiniMapPath: return "MINIMAPPATH";
    case PathFileType::JackpotPath: return "JACKPOTPATH";
    }
    return "UNKNOWN";
}

}  // namespace

cResourceManager& cResourceManager::getInstance() noexcept {
    static cResourceManager inst;
    return inst;
}

void cResourceManager::ReleaseScriptManager() noexcept {
    for (auto& t : m_tables) t.clear();
    m_reports.clear();
    m_loaded = false;
}

bool cResourceManager::InitScriptManager(const std::filesystem::path& path_root) {
    if (m_loaded) {
        MLOG_WARN("[cResourceManager] already loaded; releasing first");
        ReleaseScriptManager();
    }

    for (const auto& f : kHardPathFiles) {
        const auto bin_path = path_root / f.filename;
        if (!loadTable(f.type, bin_path)) {
            // 装载失败: 报告错误但不中断 — 老版即使有表缺失也继续
            MLOG_ERROR("[cResourceManager] failed to load %s from %s",
                       kPathTypeName(f.type), bin_path.string().c_str());
        }
    }

    // 7 张表都至少有一条 record 就算 loaded
    m_loaded = allLoaded();
    MLOG_INFO("[cResourceManager] InitScriptManager done: %zu/%zu tables loaded, %zu total records",
              std::count_if(m_reports.begin(), m_reports.end(),
                            [](const LoadReport& r) { return r.ok; }),
              kHardPathFiles.size(),
              sizeOf(PathFileType::HardPath) + sizeOf(PathFileType::ItemPath) +
              sizeOf(PathFileType::MugongPath) + sizeOf(PathFileType::AbilityPath) +
              sizeOf(PathFileType::BuffPath) + sizeOf(PathFileType::MiniMapPath) +
              sizeOf(PathFileType::JackpotPath));
    return m_loaded;
}

bool cResourceManager::loadTable(PathFileType type, const std::filesystem::path& bin_path) {
    LoadReport r;
    r.type = type;
    r.path = bin_path;

    // 1) 用 mxh::compat::read_mh_bin 解密 (现代等价物)
    auto res = mxh::compat::read_mh_bin(bin_path);
    if (!res.ok()) {
        r.error = "read_mh_bin failed (error code " + std::to_string(static_cast<int>(res.error)) + ")";
        m_reports.push_back(r);
        return false;
    }
    const std::string text(reinterpret_cast<const char*>(res.value.data.data()),
                            res.value.data.size());

    // 2) 解析文本
    //    老版每行 6 字段 (index, atlas_idx, left, top, right, bottom)
    //    用 tab 或空格分隔 (image_minimap_path.bin 用空格)
    //    行结束 \r\n 或 \n
    auto& table = m_tables[static_cast<std::size_t>(type)];
    std::istringstream iss(text);
    std::string line;
    std::size_t line_no = 0;
    std::size_t ok = 0;
    while (std::getline(iss, line)) {
        ++line_no;
        // 去掉 \r
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        // split by tab/space
        std::vector<int> toks;
        std::string cur;
        auto flush = [&]() {
            if (!cur.empty()) {
                try { toks.push_back(std::stoi(cur)); }
                catch (...) { toks.push_back(0); }
                cur.clear();
            }
        };
        for (char ch : line) {
            if (ch == '\t' || ch == ' ') flush();
            else cur.push_back(ch);
        }
        flush();

        if (toks.size() < 5) continue;  // 老版允许 5 字段（无 atlas_idx）
        ImageHardPath hp;
        std::int32_t dialog_idx = toks[0];
        if (toks.size() == 6) {
            hp.atlas_idx = toks[1];
            hp.left   = toks[2];
            hp.top    = toks[3];
            hp.right  = toks[4];
            hp.bottom = toks[5];
        } else {  // 5 字段: index, left, top, right, bottom
            hp.atlas_idx = 0;
            hp.left   = toks[1];
            hp.top    = toks[2];
            hp.right  = toks[3];
            hp.bottom = toks[4];
        }
        if (hp.right < hp.left || hp.bottom < hp.top) {
            // 老版不校验; 我们记录但仍装载
            MLOG_WARN("[cResourceManager] %s line %zu: invalid rect l=%d t=%d r=%d b=%d",
                      kPathTypeName(type), line_no, hp.left, hp.top, hp.right, hp.bottom);
        }
        if (table.find(dialog_idx) == table.end()) {
            table[dialog_idx] = hp;
            ++ok;
        }
    }

    r.records = ok;
    r.ok = true;
    r.error.clear();
    m_reports.push_back(r);
    MLOG_INFO("[cResourceManager] %s loaded from %s: %zu records (out of %zu lines)",
              kPathTypeName(type), bin_path.filename().string().c_str(),
              ok, line_no);
    return true;
}

std::optional<ImageHardPath>
cResourceManager::getHardPath(std::int32_t hard_idx, PathFileType type) const noexcept {
    if (hard_idx < 0) return std::nullopt;
    const auto idx = static_cast<std::size_t>(type);
    if (idx >= m_tables.size()) return std::nullopt;
    const auto& table = m_tables[idx];
    auto it = table.find(hard_idx);
    if (it == table.end()) return std::nullopt;
    return it->second;
}

std::size_t cResourceManager::sizeOf(PathFileType type) const noexcept {
    const auto idx = static_cast<std::size_t>(type);
    if (idx >= m_tables.size()) return 0;
    return m_tables[idx].size();
}

bool cResourceManager::allLoaded() const noexcept {
    if (m_reports.size() != kHardPathFiles.size()) return false;
    for (const auto& r : m_reports) {
        if (!r.ok) return false;
    }
    return true;
}

}  // namespace mxh::ui
