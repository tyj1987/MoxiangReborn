#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace mxh::server {

struct ItemLimitInfo {
    std::uint32_t dwItmeIdx = 0;
    int nItemLimitCount = 0;
    int nItemCurrentCount = 0;
};

struct ItemLimitManagerState {
    std::unordered_map<std::uint32_t, ItemLimitInfo> m_ItemLimitTable;
};

inline ItemLimitManagerState make_item_limit_manager() {
    return ItemLimitManagerState{};
}

inline void item_limit_manager_init(ItemLimitManagerState& state) {
    state.m_ItemLimitTable.clear();
    state.m_ItemLimitTable.reserve(20u);
}

inline void item_limit_manager_release(ItemLimitManagerState& state) {
    state.m_ItemLimitTable.clear();
}

inline ItemLimitInfo* get_item_limit_info(ItemLimitManagerState& state,
                                          std::uint32_t itemIdx) {
    const auto it = state.m_ItemLimitTable.find(itemIdx);
    return it == state.m_ItemLimitTable.end() ? nullptr : &it->second;
}

inline const ItemLimitInfo* get_item_limit_info(const ItemLimitManagerState& state,
                                                std::uint32_t itemIdx) {
    const auto it = state.m_ItemLimitTable.find(itemIdx);
    return it == state.m_ItemLimitTable.end() ? nullptr : &it->second;
}

inline bool load_item_limit_record(ItemLimitManagerState& state,
                                   std::uint32_t itemIdx,
                                   int limitCount) {
    if (itemIdx == 0u) return false;
    state.m_ItemLimitTable.insert_or_assign(
        itemIdx, ItemLimitInfo{itemIdx, limitCount, 0});
    return true;
}

inline int check_item_limit_info(const ItemLimitManagerState& state,
                                 std::uint32_t itemIdx) {
    const auto* info = get_item_limit_info(state, itemIdx);
    if (info == nullptr) return 1;
    if (info->nItemLimitCount > info->nItemCurrentCount)
        return info->nItemLimitCount - info->nItemCurrentCount;
    return 0;
}

inline bool set_item_limit_info_from_db(ItemLimitManagerState& state,
                                        std::uint32_t itemIdx,
                                        int limitCount,
                                        int currentCount) {
    auto* info = get_item_limit_info(state, itemIdx);
    if (info == nullptr) return false;
    info->nItemLimitCount = limitCount;
    info->nItemCurrentCount = currentCount;
    return true;
}

inline bool add_current_item_count(ItemLimitManagerState& state,
                                   std::uint32_t itemIdx,
                                   int itemCount) {
    auto* info = get_item_limit_info(state, itemIdx);
    if (info == nullptr) return false;
    info->nItemCurrentCount += itemCount;
    return true;
}

inline bool sync_current_item_count(ItemLimitManagerState& state,
                                    std::uint32_t itemIdx,
                                    std::uint32_t currentCount) {
    auto* info = get_item_limit_info(state, itemIdx);
    if (info == nullptr) return false;
    info->nItemCurrentCount = static_cast<int>(currentCount);
    return true;
}

inline bool set_item_limit_count(ItemLimitManagerState& state,
                                 std::uint32_t itemIdx,
                                 int limitCount) {
    auto* info = get_item_limit_info(state, itemIdx);
    if (info == nullptr) return false;
    info->nItemLimitCount = limitCount;
    return true;
}

inline std::size_t item_limit_record_count(const ItemLimitManagerState& state) {
    return state.m_ItemLimitTable.size();
}

}
