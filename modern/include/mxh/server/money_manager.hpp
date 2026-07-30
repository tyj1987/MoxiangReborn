// money_manager.hpp - 1:1 port of legacy [Server]Map/MoneyManager.h.
//
// Legacy CMoneyManager owns the per-player purse movements (MoneyLimit /
// Money filter / log transactions to DB). Modern port locks the rules:
//   - Cannot exceed PLAYER_MAX_MONEY (legacy 21,000,000 gold).
//   - Cannot go below 0.
//   - All operations log via callback (legacy writes DBLOG_MONEY).

#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace mxh::server {

inline constexpr std::uint32_t MXH_PLAYER_MAX_MONEY    = 21'000'000u;   // legacy
inline constexpr std::uint32_t MXH_PLAYER_MIN_TRANSFER = 1u;

struct MoneyLogEntry final {
    std::uint32_t player_id = 0;
    std::int64_t  delta      = 0;            // negative for spend
    std::uint32_t before     = 0;
    std::uint32_t after      = 0;
    std::string   reason;
};

class MoneyManager final {
public:
    using LogSink = std::function<void(const MoneyLogEntry&)>;

    bool set_money(std::uint32_t player_id, std::uint32_t amount,
                     const std::string& reason) noexcept;
    bool add(std::uint32_t player_id, std::uint32_t amount,
                const std::string& reason) noexcept;
    bool spend(std::uint32_t player_id, std::uint32_t amount,
                  const std::string& reason) noexcept;
    std::uint32_t balance(std::uint32_t player_id) const noexcept;
    std::size_t size() const noexcept { return ledger_.size(); }
    void set_log_sink(LogSink sink) noexcept { log_sink_ = std::move(sink); }

private:
    struct Row { std::uint32_t player_id = 0; std::uint32_t money = 0; };
    std::vector<Row> ledger_;
    LogSink log_sink_;
};

}  // namespace mxh::server
