// login_server_db_msg_parser.cpp

#include "mxh/server/login_server_db_msg_parser.hpp"

namespace mxh::server {

LoginAuthResult LoginServerDBMsgParser::auth_by_account(
    const std::string& account_name,
    const std::string& password_hash) noexcept {
    LoginAuthResult r;
    if (account_name.empty() || password_hash.empty()) {
        r.ok = false;
        return r;
    }
    // With no adapter, we treat this as the legacy test harness: any non-empty input succeeds.
    r.ok = true;
    r.user_id = static_cast<std::uint32_t>(std::hash<std::string>{}(account_name) & 0x7FFFFFFFu);
    r.account_name = account_name;
    r.user_level = 0;  // 0 == normal player
    return r;
}

}  // namespace mxh::server

