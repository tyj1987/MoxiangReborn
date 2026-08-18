// mxh/ui/cSpriteAtlas.cpp
// M-R2: image list 装载 (老版 cResourceManager::Init 文本格式等价物)

#include "mxh/ui/cSpriteAtlas.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/log/mlog.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace mxh::ui {

namespace {

std::vector<std::string> splitTab(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : line) {
        if (ch == '\t') { out.push_back(cur); cur.clear(); }
        else cur.push_back(ch);
    }
    out.push_back(cur);
    return out;
}

}  // namespace

cSpriteAtlas& cSpriteAtlas::getInstance() noexcept {
    static cSpriteAtlas inst;
    return inst;
}

void cSpriteAtlas::Release() noexcept {
    m_entries.clear();
    m_pathRoot.clear();
    m_loaded = false;
}

bool cSpriteAtlas::Init(const std::filesystem::path& path_root) {
    Release();
    m_pathRoot = path_root;

    const auto bin_path = path_root / "Image" / "image_path.bin";
    auto res = mxh::compat::read_mh_bin(bin_path);
    if (!res.ok()) {
        MLOG_ERROR("[cSpriteAtlas] read_mh_bin failed: %s", bin_path.string().c_str());
        return false;
    }
    const std::string text(reinterpret_cast<const char*>(res.value.data.data()),
                            res.value.data.size());

    std::istringstream iss(text);
    std::string line;
    std::size_t line_no = 0;
    int total = -1;
    while (std::getline(iss, line)) {
        ++line_no;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        if (total < 0) {
            try { total = std::stoi(line); }
            catch (...) {
                MLOG_ERROR("[cSpriteAtlas] first line not a number: %s", line.c_str());
                return false;
            }
            m_entries.reserve(static_cast<std::size_t>(total));
            continue;
        }

        auto toks = splitTab(line);
        if (toks.size() < 5) {
            MLOG_WARN("[cSpriteAtlas] %s line %zu: %zu fields, skip", bin_path.filename().string().c_str(), line_no, toks.size());
            continue;
        }
        ImageInfo e;
        try {
            e.idx = std::stoi(toks[0]);
            e.filename = toks[1];
            e.width  = static_cast<std::int32_t>(std::stoi(toks[2]));
            e.height = static_cast<std::int32_t>(std::stoi(toks[3]));
            e.layer  = std::stoi(toks[4]);
        } catch (...) {
            MLOG_WARN("[cSpriteAtlas] parse fail line %zu: %s", line_no, line.c_str());
            continue;
        }
        m_entries.push_back(e);
    }

    if (total >= 0 && static_cast<int>(m_entries.size()) != total) {
        MLOG_WARN("[cSpriteAtlas] header said %d entries, parsed %zu", total, m_entries.size());
    }
    m_loaded = true;
    MLOG_INFO("[cSpriteAtlas] Init done: %zu entries loaded from %s",
              m_entries.size(), bin_path.string().c_str());
    return true;
}

std::optional<ImageInfo> cSpriteAtlas::getInfo(std::int32_t idx) const noexcept {
    if (idx < 0 || static_cast<std::size_t>(idx) >= m_entries.size()) {
        return std::nullopt;
    }
    return m_entries[static_cast<std::size_t>(idx)];
}

std::filesystem::path cSpriteAtlas::resolvePath(const ImageInfo& info) const {
    std::string f = info.filename;
    if (f.rfind("./", 0) == 0) f = f.substr(2);
    if (f.rfind(".\\", 0) == 0) f = f.substr(2);
    return m_pathRoot / f;
}

}  // namespace mxh::ui
