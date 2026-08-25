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

class EffectCatalog {
public:
    bool load(const std::filesystem::path& playdh_root,
              std::string* error = nullptr);

    const EffectAsset* find(std::string_view name) const noexcept;
    const std::vector<EffectAsset>& assets() const noexcept { return m_assets; }
    std::size_t beff_count() const noexcept { return m_beffCount; }
    std::size_t befl_count() const noexcept { return m_beflCount; }
    std::size_t packed_count() const noexcept { return m_packedCount; }
    std::size_t loose_count() const noexcept { return m_looseCount; }

private:
    std::vector<EffectAsset> m_assets;
    std::size_t m_beffCount = 0;
    std::size_t m_beflCount = 0;
    std::size_t m_packedCount = 0;
    std::size_t m_looseCount = 0;
};

} // namespace mxh::game
