// db_adapter.cpp - Common IDbAdapter helpers + factory.

#include "mxh/db/db_adapter.hpp"

#include <sstream>
#include <utility>

namespace mxh::db {

const char* to_string(DbError e) noexcept {
    switch (e) {
        case DbError::Ok: return "Ok";
        case DbError::NotConnected: return "NotConnected";
        case DbError::ConnectionFailed: return "ConnectionFailed";
        case DbError::QuerySyntaxError: return "QuerySyntaxError";
        case DbError::ConstraintViolation: return "ConstraintViolation";
        case DbError::NoSuchTable: return "NoSuchTable";
        case DbError::IoError: return "IoError";
        case DbError::NotImplemented: return "NotImplemented";
        case DbError::Unknown: return "Unknown";
    }
    return "Unknown";
}

int ResultSet::column_index(std::string_view name) const {
    for (std::size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].size() == name.size()) {
            bool match = true;
            for (std::size_t j = 0; j < name.size(); ++j) {
                if (std::tolower(static_cast<unsigned char>(columns[i][j])) !=
                    std::tolower(static_cast<unsigned char>(name[j]))) {
                    match = false;
                    break;
                }
            }
            if (match) return static_cast<int>(i);
        }
    }
    return -1;
}

ConnectionConfig ConnectionConfig::from_kv_string(std::string_view s) {
    ConnectionConfig cfg;
    std::size_t pos = 0;
    while (pos < s.size()) {
        // Find next token (between ';' separators).
        auto semi = s.find(';', pos);
        if (semi == std::string_view::npos) semi = s.size();
        auto token = s.substr(pos, semi - pos);
        pos = semi + 1;
        if (token.empty()) continue;

        auto eq = token.find('=');
        if (eq == std::string_view::npos) continue;
        auto key = token.substr(0, eq);
        auto val = token.substr(eq + 1);

        std::string k(key), v(val);
        if      (k == "backend") cfg.backend = v;
        else if (k == "path") cfg.path = v;
        else if (k == "host") cfg.host = v;
        else if (k == "port") cfg.port = static_cast<std::uint16_t>(std::stoi(v));
        else if (k == "database") cfg.database = v;
        else if (k == "user") cfg.user = v;
        else if (k == "password") cfg.password = v;
        else if (k == "odbc_driver") cfg.odbc_driver = v;
        else if (k == "encrypt") cfg.encrypt = (v == "yes" || v == "true" || v == "1");
        else if (k == "trust_server_certificate") cfg.trust_server_certificate = (v == "yes" || v == "true" || v == "1");
    }
    return cfg;
}

std::string ConnectionConfig::to_kv_string() const {
    std::ostringstream os;
    os << "backend=" << backend << ';';
    if (!path.empty()) os << "path=" << path << ';';
    if (!host.empty()) os << "host=" << host << ';';
    os << "port=" << port << ';';
    if (!database.empty()) os << "database=" << database << ';';
    if (!user.empty()) os << "user=" << user << ';';
    if (!odbc_driver.empty()) os << "odbc_driver=" << odbc_driver << ';';
    os << "encrypt=" << (encrypt ? "yes" : "no") << ';';
    os << "trust_server_certificate=" << (trust_server_certificate ? "yes" : "no") << ';';
    return os.str();
}

}  // namespace mxh::db
