#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::game {

// Runtime index for the shipped BEFF/BEFL effect assets.  This deliberately
// indexes names and raw sizes only: effect bytes remain owned by PackFile or
// the canonical resource tree and are never rewritten or heuristically
// decoded.  Consumers can use the returned source path/name to build the
// verified BEFF timeline decoder without inventing effect IDs.
struct EffectAsset {
    std::string name;
    std::filesystem::path source;
    std::size_t size = 0;
    bool packed = false;
};

struct EffectScriptSummary {
    std::string name;
    std::size_t decoded_size = 0;
    std::size_t effect_unit_count = 0;
    std::size_t trigger_count = 0;
    std::vector<std::string> dependencies;
    struct Unit {
        std::size_t index = 0;
        std::string kind;
    };
    struct Trigger {
        std::string time_token;
        std::size_t unit = 0;
        std::string kind;
    };
    std::vector<Unit> units;
    std::vector<Trigger> triggers;
    bool decoded = false;
};

class EffectCatalog {
public:
    bool load(const std::filesystem::path& playdh_root,
              std::string* error = nullptr);

    const EffectAsset* find(std::string_view name) const noexcept;
    const EffectScriptSummary* script(std::string_view name) const noexcept;
    const std::vector<EffectAsset>& assets() const noexcept { return m_assets; }
    std::size_t beff_count() const noexcept { return m_beffCount; }
    std::size_t befl_count() const noexcept { return m_beflCount; }
    std::size_t packed_count() const noexcept { return m_packedCount; }
    std::size_t loose_count() const noexcept { return m_looseCount; }
    std::size_t decoded_beff_count() const noexcept { return m_decodedBeffCount; }
    std::size_t invalid_beff_count() const noexcept { return m_invalidBeffCount; }

private:
    std::vector<EffectAsset> m_assets;
    std::size_t m_beffCount = 0;
    std::size_t m_beflCount = 0;
    std::size_t m_packedCount = 0;
    std::size_t m_looseCount = 0;
    std::size_t m_decodedBeffCount = 0;
    std::size_t m_invalidBeffCount = 0;
    std::vector<EffectScriptSummary> m_scripts;
};

} // namespace mxh::game
