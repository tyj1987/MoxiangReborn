#include "gm_item_catalog.hpp"
#include "mxh/game/item_list_parser.hpp"
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif
namespace mxh::gm {
bool ItemCatalog::load(const std::string& path, std::string& error) {
    const auto parsed = mxh::game::load_item_list(path);
    if (parsed.items.empty()) {
        error = parsed.error_message.empty() ? "ItemList.bin parse failed" : parsed.error_message;
        return false;
    }
    items_ = parsed.items;
    parse_errors_ = parsed.parse_errors;
    error.clear();
    return true;
}
std::string item_name_utf8(const mxh::game::ItemInfo& item) {
    const std::string raw(item.ItemName, strnlen(item.ItemName, sizeof(item.ItemName)));
#ifdef _WIN32
    if (raw.empty()) return {};
    // This shipped ItemList.bin is the Traditional Chinese service asset
    // and stores ItemName in Big5 (Windows code page 950).
    constexpr UINT kItemNameCodePage = 950;
    const int wide_size = MultiByteToWideChar(kItemNameCodePage, 0, raw.data(), static_cast<int>(raw.size()), nullptr, 0);
    if (wide_size <= 0) return raw;
    std::wstring wide(static_cast<std::size_t>(wide_size), L'\0');
    MultiByteToWideChar(kItemNameCodePage, 0, raw.data(), static_cast<int>(raw.size()), wide.data(), wide_size);
    const int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_size, nullptr, 0, nullptr, nullptr);
    if (utf8_size <= 0) return raw;
    std::string utf8(static_cast<std::size_t>(utf8_size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_size, utf8.data(), utf8_size, nullptr, nullptr);
    return utf8;
#else
    return raw;
#endif
}
} // namespace mxh::gm
