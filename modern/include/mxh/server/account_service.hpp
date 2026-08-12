#pragma once

#include "mxh/db/db_adapter.hpp"

#include <string>
#include <string_view>
#include <cstdint>

namespace mxh::server {

enum class AccountCreateStatus {
    Ok,
    InvalidAccount,
    WeakPassword,
    AlreadyExists,
    CryptoError,
    DatabaseError,
};

struct AccountCreateResult {
    AccountCreateStatus status = AccountCreateStatus::DatabaseError;
    std::string message;
    [[nodiscard]] bool ok() const noexcept { return status == AccountCreateStatus::Ok; }
};

[[nodiscard]] bool valid_account_name(std::string_view account) noexcept;
[[nodiscard]] bool valid_account_password(std::string_view password) noexcept;
[[nodiscard]] std::string hash_account_password(std::string_view password);
[[nodiscard]] bool verify_account_password(std::string_view password,
                                           std::string_view stored) noexcept;
[[nodiscard]] AccountCreateResult create_account(mxh::db::IDbAdapter& db,
                                                 std::string_view account,
                                                 std::string_view password);
[[nodiscard]] std::uint32_t ensure_account_user_idx(mxh::db::IDbAdapter& db,
                                                    std::string_view account);

}  // namespace mxh::server
