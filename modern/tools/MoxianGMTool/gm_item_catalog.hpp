#pragma once
#include "mxh/game/item_list_types.hpp"
#include <cstddef>
#include <string>
#include <vector>
namespace mxh::gm {
class ItemCatalog final {
public:
    bool load(const std::string& path, std::string& error);
    const std::vector<mxh::game::ItemInfo>& items() const noexcept { return items_; }
    std::size_t parse_errors() const noexcept { return parse_errors_; }
private:
    std::vector<mxh::game::ItemInfo> items_;
    std::size_t parse_errors_ = 0;
};
std::string item_name_utf8(const mxh::game::ItemInfo& item);
} // namespace mxh::gm
