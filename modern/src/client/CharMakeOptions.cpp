#include "CharMakeOptions.hpp"

#include <sstream>
#include "mxh/compat/mh_file_ex.hpp"

namespace mxh::client {
namespace {

std::optional<CharMakeOptionCategory> category_for_id(
    std::string_view id) noexcept {
    using Category = CharMakeOptionCategory;
    if (id == "CMID_AttribType") return Category::Attribute;
    if (id == "CMID_SexType") return Category::Sex;
    if (id == "CMID_ManHairType") return Category::MaleHair;
    if (id == "CMID_WomanHairType") return Category::FemaleHair;
    if (id == "CMID_ManFaceType") return Category::MaleFace;
    if (id == "CMID_WomanFaceType") return Category::FemaleFace;
    if (id == "CMID_ClothType") return Category::Cloth;
    if (id == "CMID_BootType") return Category::Boot;
    if (id == "CMID_WeaponType") return Category::Weapon;
    if (id == "CMID_StartArea") return Category::StartArea;
    return std::nullopt;
}

bool parse_u32(const std::string& token, std::uint32_t& value) noexcept {
    try {
        std::size_t used = 0;
        const auto parsed = std::stoul(token, &used, 10);
        if (used != token.size()) return false;
        value = static_cast<std::uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

std::optional<CharMakeOptionCatalog> CharMakeOptionCatalog::parse(
    std::string_view decrypted_payload, std::string* error) {
    CharMakeOptionCatalog catalog;
    std::istringstream input{std::string(decrypted_payload)};
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::istringstream row(line);
        std::size_t option_count = 0;
        std::string control_id;
        if (!(row >> option_count >> control_id)) {
            if (error) *error = "invalid group header at line " +
                                std::to_string(line_number);
            return std::nullopt;
        }
        const auto category = category_for_id(control_id);
        if (!category || option_count == 0 || option_count > 64) {
            if (error) *error = "unknown or invalid option group at line " +
                                std::to_string(line_number);
            return std::nullopt;
        }

        CharMakeOptionGroup group;
        group.category = *category;
        group.legacy_control_id = std::move(control_id);
        group.options.reserve(option_count);
        for (std::size_t index = 0; index < option_count; ++index) {
            CharMakeOption option;
            std::string value;
            std::string helper;
            if (!(row >> option.label_bytes >> value >> helper) ||
                !parse_u32(value, option.value) ||
                !parse_u32(helper, option.helper_value)) {
                if (error) *error = "invalid option at line " +
                                    std::to_string(line_number);
                return std::nullopt;
            }
            group.options.push_back(std::move(option));
        }
        std::string trailing;
        if (row >> trailing) {
            if (error) *error = "trailing tokens at line " +
                                std::to_string(line_number);
            return std::nullopt;
        }
        catalog.m_groups.push_back(std::move(group));
    }
    if (catalog.m_groups.empty()) {
        if (error) *error = "character creation option payload is empty";
        return std::nullopt;
    }
    return catalog;
}

std::optional<CharMakeOptionCatalog> CharMakeOptionCatalog::load(
    const std::filesystem::path& playdh_root, std::string* error) {
    const auto path = playdh_root / "Resource" / "Client" /
                      "CharMake_SelectOption.bin";
    const auto decoded = mxh::compat::read_mh_bin(path);
    if (!decoded.ok()) {
        if (error) *error = "read_mh_bin failed for " + path.string();
        return std::nullopt;
    }
    const std::string_view payload(
        reinterpret_cast<const char*>(decoded.value.data.data()),
        decoded.value.data.size());
    return parse(payload, error);
}

const CharMakeOptionGroup* CharMakeOptionCatalog::find(
    CharMakeOptionCategory category) const noexcept {
    for (const auto& group : m_groups) {
        if (group.category == category) return &group;
    }
    return nullptr;
}

bool CharMakeOptionCatalog::hasChinaBaseline() const noexcept {
    using Category = CharMakeOptionCategory;
    static constexpr Category required[] = {
        Category::Sex, Category::MaleHair, Category::FemaleHair,
        Category::MaleFace, Category::FemaleFace, Category::Cloth,
        Category::Boot, Category::Weapon, Category::StartArea};
    for (const auto category : required) {
        const auto* group = find(category);
        if (!group || group->options.empty()) return false;
    }
    return true;
}

} // namespace mxh::client
