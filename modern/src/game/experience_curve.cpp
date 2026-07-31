#include "mxh/game/experience_curve.hpp"

#include "mxh/compat/mh_file_ex.hpp"

#include <charconv>
#include <stdexcept>
#include <string>

namespace mxh::game {

namespace {

std::uint64_t parse_exp(std::string_view token) {
    std::uint64_t value = 0;
    const auto* first = token.data();
    const auto* last = first + token.size();
    const auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc{} || result.ptr != last) {
        throw std::runtime_error("invalid CharacterExpPoint value");
    }
    return value;
}

std::vector<std::uint64_t> parse_entries(std::string_view text) {
    std::vector<std::uint64_t> entries;
    std::size_t cursor = 0;
    while (entries.size() < MAX_CHARACTER_LEVEL_NUM) {
        while (cursor < text.size() && text[cursor] <= ' ') ++cursor;
        if (cursor == text.size()) break;

        const auto level_begin = cursor;
        while (cursor < text.size() && text[cursor] > ' ') ++cursor;
        const auto level = parse_exp(text.substr(level_begin, cursor - level_begin));

        while (cursor < text.size() && text[cursor] <= ' ') ++cursor;
        const auto exp_begin = cursor;
        while (cursor < text.size() && text[cursor] > ' ') ++cursor;
        if (exp_begin == cursor) throw std::runtime_error("missing CharacterExpPoint value");
        const auto exp = parse_exp(text.substr(exp_begin, cursor - exp_begin));

        if (level != entries.size() + 1) {
            throw std::runtime_error("CharacterExpPoint levels are not sequential");
        }
        entries.push_back(exp);
    }
    if (entries.size() != MAX_CHARACTER_LEVEL_NUM) {
        throw std::runtime_error("CharacterExpPoint has too few levels");
    }
    return entries;
}

}  // namespace

ExperienceCurve ExperienceCurve::load_from_bin(const std::filesystem::path& path) {
    const auto result = mxh::compat::read_mh_bin(path);
    if (!result) throw std::runtime_error("cannot read CharacterExpPoint.bin");
    const std::string text(result.value.data.begin(), result.value.data.end());
    return ExperienceCurve(parse_entries(text));
}

ExperienceCurve ExperienceCurve::load_from_text(std::string_view text) {
    return ExperienceCurve(parse_entries(text));
}

std::uint64_t ExperienceCurve::max_exp_point(std::uint16_t level) const {
    if (level == 0) return required_exp_.at(0);
    return required_exp_.at(static_cast<std::size_t>(level - 1));
}

ExperienceState ExperienceCurve::add_exp(ExperienceState state, std::uint64_t amount) const {
    if (state.level >= MAX_CHARACTER_LEVEL_NUM || amount == 0) return state;
    const auto new_exp = state.exp_point + amount;
    const auto threshold = max_exp_point(state.level);
    if (new_exp >= threshold) {
        ++state.level;
        state.exp_point = new_exp - threshold;
    } else {
        state.exp_point = new_exp;
    }
    return state;
}

}  // namespace mxh::game
