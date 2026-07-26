// bobusang_manager.hpp - Phase D5 1:1 port of legacy [Server]Map/BobusangManager.h.
// Manages the traveling merchant NPC (bobusang), its appearance schedule,
// selling list, and customer list. Mirrors legacy BobusangManager fields.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::server {

// ---- Constants 1:1 ----

inline constexpr std::uint32_t BOBUSANG_NPCIDX         = 74u;
inline constexpr std::uint32_t BOBUSANG_wNpcUniqueIdx  = 300u;

// ---- POD structs ----

// Mirrors legacy BOBUSANGINFO.
struct BobusangInfo {
    std::uint32_t AppearanceMapNum        = 0;
    std::uint32_t AppearanceChannel       = 0;
    std::uint32_t AppearanceStartTime     = 0;
    std::uint32_t AppearanceDurationTime  = 0;
    std::uint32_t SellingListIndex        = 0;
};

// Mirrors legacy DealerItem (subset).
struct DealerItem {
    std::uint16_t itemIdx = 0;
    std::uint32_t money   = 0;
    std::uint16_t volume  = 0;
    std::uint16_t pos     = 0;
};

// Mirrors legacy BOBUSANGTOTALINFO.
struct BobusangTotalInfo {
    BobusangInfo AppearanceInfo{};
    std::optional<std::uint32_t> pBobusang;       // CNpc* opaque
    std::optional<std::uint32_t> pDealItemInfo;   // DealerData* opaque
    std::vector<DealerItem>      SellingItemList;
    std::vector<std::uint32_t>   pCustomerList;   // opaque CPlayer*
};

// Mirrors legacy BobusangManager.
struct BobusangManagerState {
    std::optional<BobusangTotalInfo> m_pBobusang;
};

// ---- Free functions ----

inline BobusangManagerState make_bobusang_manager() {
    return BobusangManagerState{};
}

// BobusangMgr_Init: resets state.
inline void bobusang_mgr_init(BobusangManagerState& s) {
    s.m_pBobusang.reset();
}

// BobusangMgr_Release: clears any active merchant.
inline void bobusang_mgr_release(BobusangManagerState& s) {
    s.m_pBobusang.reset();
}

// MakeNewBobusangNpc: replaces current merchant with a fresh one.
inline bool make_new_bobusang_npc(BobusangManagerState& s, const BobusangInfo& info) {
    BobusangTotalInfo t;
    t.AppearanceInfo = info;
    s.m_pBobusang = t;
    return true;
}

inline bool remove_bobusang_npc(BobusangManagerState& s) {
    if (!s.m_pBobusang) return false;
    s.m_pBobusang.reset();
    return true;
}

// SetBobusanInfo overwrites AppearanceInfo in-place.
inline bool set_bobusang_info(BobusangManagerState& s, const BobusangInfo& info) {
    if (!s.m_pBobusang) return false;
    s.m_pBobusang->AppearanceInfo = info;
    return true;
}

inline bool is_bobusang_active(const BobusangManagerState& s) {
    return s.m_pBobusang.has_value();
}

// Selling list accessors.
inline void add_selling_item(BobusangManagerState& s, const DealerItem& item) {
    if (!s.m_pBobusang) return;
    s.m_pBobusang->SellingItemList.push_back(item);
}

inline void clear_selling_items(BobusangManagerState& s) {
    if (!s.m_pBobusang) return;
    s.m_pBobusang->SellingItemList.clear();
}

// GetBobusangSellingRt fills an array with up to (count) items; returns
// number written (clamped at legacy SLOT_NPCINVEN_NUM-style cap = 20).
inline int get_bobusang_selling_rt(const BobusangManagerState& s,
                                   DealerItem* out_list, int count) {
    if (!s.m_pBobusang || out_list == nullptr || count <= 0) return 0;
    const int cap = 20;
    const int max = (count > cap) ? cap : count;
    const int avail = static_cast<int>(s.m_pBobusang->SellingItemList.size());
    const int n = (avail < max) ? avail : max;
    for (int i = 0; i < n; ++i) {
        out_list[i] = s.m_pBobusang->SellingItemList[static_cast<std::size_t>(i)];
    }
    return n;
}

// GetSellingItem: find by item idx. Returns nullptr if missing.
inline DealerItem* get_selling_item(BobusangManagerState& s, std::uint16_t buy_item_idx) {
    if (!s.m_pBobusang) return nullptr;
    for (auto& it : s.m_pBobusang->SellingItemList) {
        if (it.itemIdx == buy_item_idx) return &it;
    }
    return nullptr;
}

// Customer list.
inline void add_guest(BobusangManagerState& s, std::uint32_t player_id) {
    if (!s.m_pBobusang) return;
    s.m_pBobusang->pCustomerList.push_back(player_id);
}

inline void leave_guest(BobusangManagerState& s, std::uint32_t player_id) {
    if (!s.m_pBobusang) return;
    for (auto it = s.m_pBobusang->pCustomerList.begin(); it != s.m_pBobusang->pCustomerList.end(); ++it) {
        if (*it == player_id) {
            s.m_pBobusang->pCustomerList.erase(it);
            return;
        }
    }
}

inline int get_customer_count(const BobusangManagerState& s) {
    if (!s.m_pBobusang) return 0;
    return static_cast<int>(s.m_pBobusang->pCustomerList.size());
}

// BuyItem: returns true if the requested idx exists in selling list.
inline bool buy_item_available(const BobusangManagerState& s, std::uint16_t buy_item_idx) {
    if (!s.m_pBobusang) return false;
    for (const auto& it : s.m_pBobusang->SellingItemList) {
        if (it.itemIdx == buy_item_idx) return true;
    }
    return false;
}

} // namespace mxh::server
