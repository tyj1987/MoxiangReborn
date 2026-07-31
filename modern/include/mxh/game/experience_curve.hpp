#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <utility>
#include <vector>

namespace mxh::game {

inline constexpr std::uint16_t MAX_CHARACTER_LEVEL_NUM = 121;

struct ExperienceState {
    std::uint16_t level = 1;
    std::uint64_t exp_point = 0;
};

class ExperienceCurve {
public:
    static ExperienceCurve load_from_bin(const std::filesystem::path& path);
    static ExperienceCurve load_from_text(std::string_view text);

    std::size_t size() const noexcept { return required_exp_.size(); }
    std::uint64_t max_exp_point(std::uint16_t level) const;
    ExperienceState add_exp(ExperienceState state, std::uint64_t amount) const;

private:
    explicit ExperienceCurve(std::vector<std::uint64_t> required_exp)
        : required_exp_(std::move(required_exp)) {}

    std::vector<std::uint64_t> required_exp_;
};

}  // namespace mxh::game
