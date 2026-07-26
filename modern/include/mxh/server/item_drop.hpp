#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::server {

inline constexpr std::size_t DROP_ITEM_KIND_COUNT = 5u;
inline constexpr std::size_t MAX_DROPITEM_NUM = 20u;
inline constexpr std::uint32_t MAX_DROP_ITEM_PERCENT = 10000u;
inline constexpr std::uint32_t MAX_DROP_MAPITEM_PERCENT = 1000000u;

enum class DropItemKind : std::uint8_t {
    NoItem = 0,
    Money = 1,
    Item1 = 2,
    Item2 = 3,
    Item3 = 4,
};

struct DropRateContext {
    std::array<std::uint32_t, DROP_ITEM_KIND_COUNT> baseRates{};
    float moneyEventRate = 1.0f;
    float itemEventRate = 1.0f;
    float partyItemRate = 1.0f;
    std::uint32_t addItemDropPercent = 0;
    float channelDropRate = 1.0f;
};

struct DropRateTable {
    std::array<std::uint32_t, DROP_ITEM_KIND_COUNT> rates{};
    std::uint32_t totalRate = 0;
};

struct MonsterDropEntry {
    std::uint16_t itemId = 0;
    std::uint32_t configuredRate = 0;
    std::uint32_t currentRate = 0;
};

struct MonsterDropPool {
    std::vector<MonsterDropEntry> entries;
    std::uint32_t currentTotalRate = 0;
};

struct MapDropItem {
    std::uint16_t itemId = 0;
    std::uint32_t dropRate = 0;
    std::uint16_t dropCount = 0;
    std::uint16_t maxDropCount = 0;
};

struct MapDropTable {
    std::uint32_t channel = 0;
    std::uint16_t maxDropNum = 0;
    std::vector<MapDropItem> items;
    std::uint32_t totalRate = 0;
};

inline DropRateTable calculate_drop_rates(const DropRateContext& context) {
    DropRateTable table;
    table.rates = context.baseRates;
    table.rates[1] = static_cast<std::uint32_t>(table.rates[1] * context.moneyEventRate);
    for (std::size_t index = 2; index <= 4; ++index) {
        table.rates[index] = static_cast<std::uint32_t>(table.rates[index] * context.itemEventRate);
        if (context.partyItemRate != 0.0f)
            table.rates[index] = static_cast<std::uint32_t>(table.rates[index] * context.partyItemRate);
    }

    if (context.addItemDropPercent != 0u) {
        const auto multiplier = static_cast<std::uint32_t>(
            1.0f + context.addItemDropPercent * 0.01f + 0.001f);
        for (std::size_t index = 2; index <= 4; ++index) table.rates[index] *= multiplier;
    }

    if (context.channelDropRate != 1.0f) {
        for (std::size_t index = 2; index <= 4; ++index)
            table.rates[index] = static_cast<std::uint32_t>(context.channelDropRate * table.rates[index]);
    }

    for (const auto rate : table.rates) table.totalRate += rate;
    if (table.totalRate > MAX_DROP_ITEM_PERCENT) {
        const auto rewardTotal = table.rates[1] + table.rates[2] + table.rates[3] + table.rates[4];
        if (rewardTotal < MAX_DROP_ITEM_PERCENT) {
            table.rates[0] = MAX_DROP_ITEM_PERCENT - rewardTotal;
            table.totalRate = MAX_DROP_ITEM_PERCENT;
        } else {
            table.rates[0] = 0;
            table.totalRate = rewardTotal;
        }
    }
    return table;
}

inline std::optional<DropItemKind> select_drop_kind(const DropRateTable& table,
                                                    std::uint32_t oneBasedRoll) {
    if (table.totalRate == 0u || oneBasedRoll == 0u || oneBasedRoll > table.totalRate)
        return std::nullopt;
    std::uint32_t upper = 0;
    for (std::size_t index = 0; index < table.rates.size(); ++index) {
        const auto lower = upper;
        upper += table.rates[index];
        if (oneBasedRoll > lower && oneBasedRoll <= upper)
            return static_cast<DropItemKind>(index);
    }
    return std::nullopt;
}

inline std::uint32_t monster_drop_money(std::uint32_t minMoney,
                                        std::uint32_t maxMoney,
                                        std::uint32_t rangedValue,
                                        float getMoneyEventRate) {
    const auto base = maxMoney > minMoney ? rangedValue : minMoney;
    return static_cast<std::uint32_t>(base * getMoneyEventRate);
}

inline bool reload_monster_drop_pool(MonsterDropPool& pool) {
    pool.currentTotalRate = 0;
    for (auto& entry : pool.entries) {
        entry.currentRate = entry.configuredRate;
        pool.currentTotalRate += entry.currentRate;
    }
    return pool.currentTotalRate != 0u;
}

inline std::optional<std::uint16_t> draw_monster_drop_item(MonsterDropPool& pool,
                                                          std::uint32_t oneBasedRoll) {
    if (pool.currentTotalRate == 0u && !reload_monster_drop_pool(pool)) return std::nullopt;
    if (oneBasedRoll == 0u || oneBasedRoll > pool.currentTotalRate) return std::nullopt;
    std::uint32_t upper = 0;
    for (auto& entry : pool.entries) {
        const auto lower = upper;
        upper += entry.currentRate;
        if (oneBasedRoll > lower && oneBasedRoll <= upper) {
            --entry.currentRate;
            --pool.currentTotalRate;
            if (entry.itemId == 0u) return std::nullopt;
            return entry.itemId;
        }
    }
    return std::nullopt;
}

inline std::optional<std::size_t> select_map_drop_item(const MapDropTable& table,
                                                      std::uint32_t oneBasedRoll) {
    if (table.totalRate == 0u || oneBasedRoll == 0u || oneBasedRoll > table.totalRate)
        return std::nullopt;
    std::uint32_t upper = 0;
    for (std::size_t index = 0; index < table.items.size(); ++index) {
        const auto lower = upper;
        upper += table.items[index].dropRate;
        if (oneBasedRoll > lower && oneBasedRoll <= upper) return index;
    }
    return std::nullopt;
}

inline std::optional<std::uint16_t> try_map_drop(MapDropTable& table,
                                                std::uint32_t oneBasedRoll) {
    const auto index = select_map_drop_item(table, oneBasedRoll);
    if (!index.has_value()) return std::nullopt;
    auto& item = table.items[*index];
    if (item.itemId == 0u || item.dropCount >= item.maxDropCount) return std::nullopt;
    ++item.dropCount;
    return item.itemId;
}

inline void reset_map_drop_counts(MapDropTable& table) {
    for (auto& item : table.items) item.dropCount = 0;
}

inline bool is_map_drop_reset_window(std::uint16_t currentDay,
                                     std::uint16_t currentHour,
                                     std::uint16_t currentMinute,
                                     std::uint16_t initDay,
                                     std::uint16_t initHour) {
    return currentDay == initDay && currentHour == initHour && currentMinute <= 10u;
}

}
