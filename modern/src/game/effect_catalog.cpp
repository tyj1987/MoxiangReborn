#include "mxh/game/effect_catalog.hpp"

#include "mxh/compat/pack_file.hpp"

#include <algorithm>
#include <cctype>

namespace mxh::game {
namespace {

std::string lower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

bool is_effect_name(std::string_view name, bool& beff, bool& befl) {
    const auto dot = name.find_last_of('.');
    if (dot == std::string_view::npos) return false;
    const auto ext = lower(name.substr(dot));
    beff = ext == ".beff";
    befl = ext == ".befl";
    // Effect.pak stores the compiled effect dependency closure as MOD/CHX/
    // ANM beside any BEFF/BEFL scripts.  Keep those entries in the same
    // catalog so a runtime resolver can load the complete closure.
    return beff || befl || ext == ".mod" || ext == ".chx" || ext == ".anm";
}

} // namespace

bool EffectCatalog::load(const std::filesystem::path& root, std::string* error) {
    m_assets.clear();
    m_beffCount = m_beflCount = m_packedCount = m_looseCount = 0;
    if (!std::filesystem::exists(root)) {
        if (error) *error = "effect resource root does not exist";
        return false;
    }

    const auto pakPath = root / "Effect.pak";
    if (std::filesystem::exists(pakPath)) {
        const auto pack = mxh::compat::PackFile::open(pakPath);
        if (!pack) {
            if (error) *error = "Effect.pak is not a valid PackFile";
            return false;
        }
        for (const auto& entry : pack->entries()) {
            bool beff = false;
            bool befl = false;
            if (!is_effect_name(entry.name, beff, befl)) continue;
            m_assets.push_back({entry.name, pakPath, entry.size, true});
            if (beff) ++m_beffCount;
            if (befl) ++m_beflCount;
            ++m_packedCount;
        }
    }

    std::error_code ec;
    for (std::filesystem::recursive_directory_iterator it(root, ec), end;
         it != end && !ec; it.increment(ec)) {
        if (!it->is_regular_file()) continue;
        const auto path = it->path();
        if (lower(path.extension().string()) != ".beff" &&
            lower(path.extension().string()) != ".befl") continue;
        bool beff = false;
        bool befl = false;
        const auto name = path.filename().string();
        if (!is_effect_name(name, beff, befl)) continue;
        m_assets.push_back({name, path, static_cast<std::size_t>(it->file_size()), false});
        if (beff) ++m_beffCount;
        if (befl) ++m_beflCount;
        ++m_looseCount;
    }

    if (m_assets.empty()) {
        if (error) *error = "no BEFF or BEFL effect assets found";
        return false;
    }
    return true;
}

const EffectAsset* EffectCatalog::find(std::string_view name) const noexcept {
    const auto wanted = lower(name);
    for (const auto& asset : m_assets) {
        if (lower(asset.name) == wanted) return &asset;
    }
    return nullptr;
}

} // namespace mxh::game
