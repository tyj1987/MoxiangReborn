#include "mxh/game/effect_catalog.hpp"

#include "mxh/compat/pack_file.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace mxh::game {
namespace {

std::string lower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

void scan_script(EffectScriptSummary& summary,
                 std::span<const std::uint8_t> bytes) {
    summary.decoded_size = bytes.size();
    const std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const auto trim = [&line]() {
            const auto first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos) return std::string{};
            const auto last = line.find_last_not_of(" \t\r");
            return line.substr(first, last - first + 1);
        }();
        if (trim.rfind("#MAXEFFECTUNIT", 0) == 0) {
            std::istringstream fields(trim.substr(14));
            fields >> summary.effect_unit_count;
        } else if (trim.rfind("#MAXTRIGGER", 0) == 0) {
            std::istringstream fields(trim.substr(12));
            fields >> summary.trigger_count;
        } else if (trim.rfind("#NEWEFFECTUNIT", 0) == 0) {
            std::istringstream fields(trim);
            std::string directive;
            EffectScriptSummary::Unit unit;
            std::string ignored_bool;
            fields >> directive >> unit.index >> unit.kind >> ignored_bool;
            if (!unit.kind.empty()) summary.units.push_back(std::move(unit));
        } else if (trim.rfind("#TRIGGER", 0) == 0) {
            std::istringstream fields(trim);
            std::string directive;
            EffectScriptSummary::Trigger trigger;
            fields >> directive >> trigger.time_token >> trigger.unit >> trigger.kind;
            if (!trigger.kind.empty()) summary.triggers.push_back(std::move(trigger));
        } else if (trim.rfind("#OBJECTNAME", 0) == 0 ||
                   trim.rfind("#SOUNDNAME", 0) == 0 ||
                   trim.rfind("#TEXTURE", 0) == 0) {
            std::istringstream fields(trim);
            std::string directive;
            std::string dependency;
            fields >> directive >> dependency;
            if (!dependency.empty()) summary.dependencies.push_back(dependency);
        }
    }
    summary.decoded = summary.effect_unit_count != 0 || summary.trigger_count != 0;
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
    m_decodedBeffCount = m_invalidBeffCount = 0;
    m_scripts.clear();
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

    // BEFF files are classic MHFileEx resources. Decode only the loose
    // scripts, where the canonical bytes and profile are unambiguous; packed
    // MOD/CHX/ANM entries remain dependency assets for the renderer.
    for (const auto& asset : m_assets) {
        if (asset.packed || lower(std::filesystem::path(asset.name).extension().string()) != ".beff") {
            continue;
        }
        EffectScriptSummary summary;
        summary.name = asset.name;
        const auto decoded = mxh::compat::read_mh_bin(asset.source);
        if (!decoded.ok()) {
            ++m_invalidBeffCount;
            m_scripts.push_back(std::move(summary));
            continue;
        }
        scan_script(summary, decoded.value.data);
        if (summary.decoded) ++m_decodedBeffCount;
        else ++m_invalidBeffCount;
        m_scripts.push_back(std::move(summary));
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

const EffectScriptSummary* EffectCatalog::script(std::string_view name) const noexcept {
    const auto wanted = lower(name);
    for (const auto& summary : m_scripts) {
        if (lower(summary.name) == wanted) return &summary;
    }
    return nullptr;
}

} // namespace mxh::game
