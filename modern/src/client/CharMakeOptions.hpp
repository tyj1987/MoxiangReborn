#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::client {

enum class CharMakeOptionCategory : std::uint8_t {
    Attribute,
    Sex,
    MaleHair,
    FemaleHair,
    MaleFace,
    FemaleFace,
    Cloth,
    Boot,
    Weapon,
    StartArea,
};

struct CharMakeOption {
    std::string label_bytes;
    std::uint32_t value = 0;
    std::uint32_t helper_value = 0;
};

struct CharMakeOptionGroup {
    CharMakeOptionCategory category = CharMakeOptionCategory::Sex;
    std::string legacy_control_id;
    std::vector<CharMakeOption> options;
};

class CharMakeOptionCatalog {
public:
    static std::optional<CharMakeOptionCatalog> parse(
        std::string_view decrypted_payload, std::string* error = nullptr);
    static std::optional<CharMakeOptionCatalog> load(
        const std::filesystem::path& playdh_root, std::string* error = nullptr);

    const CharMakeOptionGroup* find(CharMakeOptionCategory category) const noexcept;
    const std::vector<CharMakeOptionGroup>& groups() const noexcept { return m_groups; }
    bool hasChinaBaseline() const noexcept;

private:
    std::vector<CharMakeOptionGroup> m_groups;
};

} // namespace mxh::client
