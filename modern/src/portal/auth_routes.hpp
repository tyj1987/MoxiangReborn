// M5.3: /api/auth/* routes (register, login, me, logout).
//
// Reuses mxh::server::account_service for PBKDF2 registration + verification
// and mxh::server::account_moderation for ban checks. JWT issuance uses the
// portal::jwt_token helper already linked into mxh_portal_lib.

#pragma once

#include "mxh/db/db_adapter.hpp"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <string_view>

namespace mxh::portal {

class HttpServer;

// Wire all four /api/auth/* handlers onto the given HttpServer.
void register_auth_routes(HttpServer& server,
                          mxh::db::IDbAdapter& db,
                          std::string jwt_secret);

// POST /api/auth/register
// body: {account, password, confirm}
// 201 -> {user_idx, account}
// 400 -> {error}  invalid account / weak password / mismatch
// 409 -> {error}  account already exists
struct AuthRegisterResult {
    int status_code = 500;
    std::uint32_t user_idx = 0;
    std::string account;
    std::string error;
};
AuthRegisterResult handle_register(mxh::db::IDbAdapter& db,
                                   const nlohmann::json& body);

// POST /api/auth/login
// body: {account, password}
// 200 -> {token, user_idx, account, points}
// 401 -> {error}  bad credentials / banned
struct AuthLoginResult {
    int status_code = 500;
    std::string token;
    std::uint32_t user_idx = 0;
    std::string account;
    std::int64_t usepoint = 0;
    std::string error;
};
AuthLoginResult handle_login(mxh::db::IDbAdapter& db,
                             std::string_view jwt_secret,
                             const nlohmann::json& body);

// GET /api/auth/me (protected - JWT bearer)
// 200 -> {account, user_idx, points, registerdate, lastlogindate, lastloginip}
struct AuthMeResult {
    int status_code = 500;
    nlohmann::json body;
};
AuthMeResult handle_me(mxh::db::IDbAdapter& db,
                       std::string_view account_name);

// POST /api/auth/logout (protected - JWT bearer, stateless)
// 204 -> no body
struct AuthLogoutResult {
    int status_code = 204;
};
AuthLogoutResult handle_logout();

}  // namespace mxh::portal
