#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mxh::game {
struct KyungGongInfo {
    std::uint16_t id=0, need_mp=0, move_type=0, change_time=0;
    std::uint16_t start_effect=0, ongoing_effect=0, end_effect=0;
    float speed=0;
    std::string source_name; // Original encoded bytes, not assumed UTF-8.
};
class KyungGongCatalog {
public:
    static std::optional<KyungGongCatalog> parse(std::string_view decoded, std::string& error);
    const KyungGongInfo* find(std::uint16_t id) const noexcept;
    std::size_t size() const noexcept { return m_entries.size(); }
private:
    std::unordered_map<std::uint16_t,KyungGongInfo> m_entries;
};
}
