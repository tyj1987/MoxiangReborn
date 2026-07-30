// login_server_db_msg_parser.hpp - 1:1 port of legacy [Server]Distribute/LoginServerDBMsgParser.
//
// Connects Distribute server's userdb queries. Modern port declares
// the dispatch table; the actual DB glue lives in mxh::db.

#pragma once

#include "mxh/db/db_adapter.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace mxh::server {

// Result of LoginServer authentication query.
struct LoginAuthResult final {
    bool ok = false;
    std::uint32_t user_id = 0;
    std::string   account_name;
    std::uint8_t  user_level = 0;     // 0..5 (gm ladder)
    std::uint8_t  reserved0 = 0;
    std::uint16_t reserved1 = 0;
    std::vector<std::uint32_t> character_ids;
};

class LoginServerDBMsgParser final {
public:
    LoginServerDBMsgParser() = default;

    // Look up by (account_name, password_hash). Real impl wraps mxh::db::mssql.
    LoginAuthResult auth_by_account(const std::string& account_name,
                                       const std::string& password_hash) noexcept;

    // Set the backing DB adapter (legacy plugs MssqlAdapter; modern tests use SQLite).
    void set_adapter(std::shared_ptr<mxh::db::IDbAdapter> adapter) noexcept {
        adapter_ = std::move(adapter);
    }

private:
    std::shared_ptr<mxh::db::IDbAdapter> adapter_;
};

}  // namespace mxh::server

