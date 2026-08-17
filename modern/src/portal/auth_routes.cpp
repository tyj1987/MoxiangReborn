// M5.3: /api/auth/* route implementations.

#include "portal/auth_routes.hpp"
#include "portal/http_server.hpp"
#include "portal/jwt_token.hpp"
#include "portal/portal_log.hpp"

#include "mxh/server/account_service.hpp"
#include "mxh/server/account_moderation.hpp"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mxh::portal {

namespace {

// Current UTC time formatted as ISO8601 (used for registerdate / lastlogindate).
std::string now_iso8601() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

// SELECT usepoint + registerdate / lastlogin* from chr_log_info.
mxh::db::DbResult query_account_row(mxh::db::IDbAdapter& db,
                                     std::string_view account,
                                     mxh::db::ResultSet& out) {
    const std::vector<mxh::db::Bind> args{mxh::db::bind(std::string(account))};
    return db.query(
        "SELECT usepoint, registerdate, lastlogindate, lastloginip "
        "FROM chr_log_info WHERE id = ?",
        args, out);
}

}  // namespace

// ---------------------------------------------------------------------------
// POST /api/auth/register
// ---------------------------------------------------------------------------
AuthRegisterResult handle_register(mxh::db::IDbAdapter& db,
                                   const nlohmann::json& body) {
    AuthRegisterResult out;
    if (!body.is_object()) {
        out.status_code = 400;
        out.error = "invalid JSON body";
        return out;
    }
    const auto account_j  = body.find("account");
    const auto password_j = body.find("password");
    const auto confirm_j  = body.find("confirm");
    if (account_j == body.end() || password_j == body.end() ||
        confirm_j  == body.end() ||
        !account_j->is_string()  || !password_j->is_string() ||
        !confirm_j->is_string()) {
        out.status_code = 400;
        out.error = "missing account / password / confirm";
        return out;
    }
    const std::string account  = account_j->get<std::string>();
    const std::string password = password_j->get<std::string>();
    const std::string confirm  = confirm_j->get<std::string>();
    if (password != confirm) {
        out.status_code = 400;
        out.error = "password and confirm do not match";
        return out;
    }
    auto created = mxh::server::create_account(db, account, password);
    if (!created.ok()) {
        switch (created.status) {
            case mxh::server::AccountCreateStatus::InvalidAccount:
            case mxh::server::AccountCreateStatus::WeakPassword:
                out.status_code = 400; break;
            case mxh::server::AccountCreateStatus::AlreadyExists:
                out.status_code = 409; break;
            default:
                out.status_code = 500; break;
        }
        out.error = created.message;
        return out;
    }
    // Stamp registerdate on first creation (only if NULL).
    const std::vector<mxh::db::Bind> args{
        mxh::db::bind(std::string(now_iso8601())),
        mxh::db::bind(std::string(account))};
    db.execute(
        "UPDATE chr_log_info SET registerdate = ? "
        "WHERE id = ? AND registerdate IS NULL",
        args);
    const auto user_idx = mxh::server::ensure_account_user_idx(db, account);
    if (user_idx == 0) {
        out.status_code = 500;
        out.error = "user_idx allocation failed";
        return out;
    }
    out.status_code = 201;
    out.user_idx = user_idx;
    out.account = account;
    return out;
}

// ---------------------------------------------------------------------------
// POST /api/auth/login
// ---------------------------------------------------------------------------
AuthLoginResult handle_login(mxh::db::IDbAdapter& db,
                             std::string_view jwt_secret,
                             const nlohmann::json& body) {
    AuthLoginResult out;
    if (jwt_secret.empty()) {
        out.status_code = 500;
        out.error = "JWT secret not configured";
        return out;
    }
    if (!body.is_object()) {
        out.status_code = 401;
        out.error = "invalid JSON body";
        return out;
    }
    const auto account_j  = body.find("account");
    const auto password_j = body.find("password");
    if (account_j == body.end() || password_j == body.end() ||
        !account_j->is_string() || !password_j->is_string()) {
        out.status_code = 401;
        out.error = "missing account or password";
        return out;
    }
    const std::string account  = account_j->get<std::string>();
    const std::string password = password_j->get<std::string>();

    // Ban check first (cheaper than PBKDF2 hashing).
    if (mxh::server::is_account_login_blocked(db, account)) {
        out.status_code = 401;
        out.error = "account is banned";
        return out;
    }
    mxh::db::ResultSet rows;
    const std::vector<mxh::db::Bind> lookup{mxh::db::bind(std::string(account))};
    auto q = db.query("SELECT pw FROM chr_log_info WHERE id = ?", lookup, rows);
    if (!q.ok() || rows.empty()) {
        out.status_code = 401;
        out.error = "invalid credentials";
        return out;
    }
    const auto* pw_value = std::get_if<std::string>(&rows.rows[0][0]);
    if (!pw_value || !mxh::server::verify_account_password(password, *pw_value)) {
        out.status_code = 401;
        out.error = "invalid credentials";
        return out;
    }
    const auto user_idx = mxh::server::ensure_account_user_idx(db, account);
    if (user_idx == 0) {
        out.status_code = 500;
        out.error = "user_idx allocation failed";
        return out;
    }
    mxh::db::ResultSet info;
    if (query_account_row(db, account, info).ok() && !info.empty()) {
        if (const auto* p = std::get_if<std::int64_t>(&info.rows[0][0])) {
            out.usepoint = *p;
        }
    }
    out.token = create_jwt(jwt_secret, account, user_idx, 86400);
    out.user_idx = user_idx;
    out.account = account;
    out.status_code = 200;
    return out;
}

// ---------------------------------------------------------------------------
// GET /api/auth/me
// ---------------------------------------------------------------------------
AuthMeResult handle_me(mxh::db::IDbAdapter& db,
                       std::string_view account_name) {
    AuthMeResult out;
    mxh::db::ResultSet info;
    if (auto r = query_account_row(db, account_name, info); !r.ok() || info.empty()) {
        out.status_code = 404;
        out.body = nlohmann::json{{"error", "account not found"}};
        return out;
    }
    const auto& row = info.rows[0];
    auto field_str = [&](std::size_t i) -> std::string {
        if (i >= row.size()) return {};
        if (const auto* s = std::get_if<std::string>(&row[i])) return *s;
        return {};
    };
    auto field_i64 = [&](std::size_t i) -> std::int64_t {
        if (i >= row.size()) return 0;
        if (const auto* v = std::get_if<std::int64_t>(&row[i])) return *v;
        return 0;
    };
    const auto user_idx = mxh::server::ensure_account_user_idx(db, account_name);
    out.status_code = 200;
    out.body = nlohmann::json{
        {"account", std::string(account_name)},
        {"user_idx", user_idx},
        {"points", field_i64(0)},
        {"registerdate", field_str(1)},
        {"lastlogindate", field_str(2)},
        {"lastloginip", field_str(3)},
    };
    return out;
}

// ---------------------------------------------------------------------------
// POST /api/auth/logout (stateless)
// ---------------------------------------------------------------------------
AuthLogoutResult handle_logout() {
    return {};
}

// ---------------------------------------------------------------------------
// Wire all four routes onto the HttpServer.
// ---------------------------------------------------------------------------
void register_auth_routes(HttpServer& server,
                          mxh::db::IDbAdapter& db,
                          std::string jwt_secret) {
    auto db_ptr = &db;
    auto secret = jwt_secret;  // copy for the closures below

    server.post_public_with_status("/api/auth/register", RateLimits::register_,
        [db_ptr](const nlohmann::json& body) -> std::pair<int, nlohmann::json> {
            const auto r = handle_register(*db_ptr, body);
            const auto status = r.status_code == 0 ? 500 : r.status_code;
            nlohmann::json out = r.error.empty()
                ? nlohmann::json{{"user_idx", r.user_idx}, {"account", r.account}}
                : nlohmann::json{{"error", r.error}};
            return {status, std::move(out)};
        });

    server.post_public_with_status("/api/auth/login", RateLimits::login,
        [db_ptr, secret](const nlohmann::json& body) -> std::pair<int, nlohmann::json> {
            const auto r = handle_login(*db_ptr, secret, body);
            const auto status = r.status_code == 0 ? 500 : r.status_code;
            nlohmann::json out = r.error.empty()
                ? nlohmann::json{
                    {"token", r.token},
                    {"user_idx", r.user_idx},
                    {"account", r.account},
                    {"points", r.usepoint}}
                : nlohmann::json{{"error", r.error}};
            return {status, std::move(out)};
        });

    server.get_protected_with_status("/api/auth/me", RateLimits::general,
        [db_ptr](const AuthContext& ctx) -> std::pair<int, nlohmann::json> {
            const auto r = handle_me(*db_ptr, ctx.account_name);
            const auto status = r.status_code == 0 ? 500 : r.status_code;
            return {status, r.body};
        });

    server.post_protected_with_status("/api/auth/logout", RateLimits::general,
        [](const AuthContext&, const nlohmann::json&) -> std::pair<int, nlohmann::json> {
            (void)handle_logout();
            return {204, nlohmann::json::object()};
        });

    MLOG_INFO("[portal] /api/auth/* routes registered (4 handlers)");
}

}  // namespace mxh::portal
