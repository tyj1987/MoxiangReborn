// server_table.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/ServerTable.h + ServerTable.cpp (CServerTable : CYHHashTable<SERVERINFO>)
// plus the SERVERINFO POD from [CC]Header/ServerGameStruct.h (packed 1:1) and
// the SERVER_KIND enum from [CC]Header/ServerGameDefine.h.
//
// Locked invariants (1:1 with legacy):
//   - ServerInfo POD fields, widths, defaults mirror legacy SERVERINFO
//     (CamelCase identifiers preserved for byte-level diff against the
//      legacy struct definition; `#pragma pack(push, 1)` preserved so the
//      memory layout matches the wire format that MSG_PWRUP_* messages
//      already rely on).
//   - SERVER_KIND enum values match legacy numbering (legacy codes 0..8
//     plus MAX_SERVER_KIND=9; the eSERVER_KIND enum in ServerTable.h is
//     dead code and is intentionally not ported).
//   - AddServer uses port (wPortForServer) as the hash key, just like
//     legacy CYHHashTable<SERVERINFO>::Add(info, Port).
//   - FindServerForConnectionIndex / RemoveServer-by-connection walk all
//     entries (linear scan, matches legacy).
//   - GetFastServer picks the connected server of `kind` with the lowest
//     wAgentUserCnt, treating wServerKind == kind as a fallback signal
//     when dwConnectionIndex is 0 (legacy quirk).
//   - AddSelfServer / AddMSServer must reject a second assignment
//     (legacy asserts; modern returns bool).
//
// Out of scope for this port:
//   - The legacy CYHHashTable<SERVERINFO> base class. We model the
//     registry as an unordered_map<port, ServerInfo>; the test suite
//     covers lookup / add / remove semantics, not the underlying hash
//     bucket layout.
//   - GetNext*Server() stateful iteration: legacy walks the hash with
//     SetPositionHead + GetData (stateful cursor). Modern exposes a
//     collect_servers() snapshot + direct find helpers, which is the
//     idiomatic C++17 alternative and is what callers will use.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mxh::server {

// ---- Constants 1:1 with legacy [CC]Header/ServerGameDefine.h ----

inline constexpr std::size_t MAX_IPADDRESS_SIZE = 16u;

// ---- Enums 1:1 with legacy SERVER_KIND ----

enum class ServerKind : std::uint16_t {
    ERROR_SERVER         = 0,
    DISTRIBUTE_SERVER    = 1,
    AGENT_SERVER         = 2,
    MAP_SERVER           = 3,
    CHAT_SERVER          = 4,
    MURIM_SERVER         = 5,
    MONITOR_AGENT_SERVER = 6,
    MONITOR_SERVER       = 7,
    BUDDYAUTH_SERVER     = 8,
    MAX_SERVER_KIND      = 9,
};

// ---- POD (1:1 with legacy SERVERINFO; legacy uses #pragma pack(1)) ----

#pragma pack(push, 1)

struct ServerInfo {
    std::uint16_t wServerKind = static_cast<std::uint16_t>(ServerKind::ERROR_SERVER);
    char          szIPForServer[MAX_IPADDRESS_SIZE] = {};
    char          szIPForUser[MAX_IPADDRESS_SIZE]   = {};
    std::uint16_t wPortForServer = 0;
    std::uint16_t wPortForUser   = 0;
    std::uint16_t wServerNum     = 0;
    std::uint32_t dwConnectionIndex = 0;
    std::uint16_t wAgentUserCnt  = 0;
};

#pragma pack(pop)

// Mirrors legacy parameterized ctor (kind, num, ip_svr, port_svr, ip_usr, port_usr).
inline ServerInfo make_server_info(std::uint16_t kind,
                                    std::uint16_t num,
                                    const char* ip_svr, std::uint16_t port_svr,
                                    const char* ip_usr, std::uint16_t port_usr) {
    ServerInfo s;
    s.wServerKind    = kind;
    s.wServerNum     = num;
    s.wPortForServer = port_svr;
    s.wPortForUser   = port_usr;
    if (ip_svr != nullptr) {
        std::strncpy(s.szIPForServer, ip_svr, MAX_IPADDRESS_SIZE - 1u);
        s.szIPForServer[MAX_IPADDRESS_SIZE - 1u] = 0;
    }
    if (ip_usr != nullptr) {
        std::strncpy(s.szIPForUser, ip_usr, MAX_IPADDRESS_SIZE - 1u);
        s.szIPForUser[MAX_IPADDRESS_SIZE - 1u] = 0;
    }
    return s;
}

// Mirrors legacy CServerTable state.
struct ServerTable {
    std::unordered_map<std::uint16_t, ServerInfo> m_Table;  // key = wPortForServer
    ServerInfo* m_pSelfServerInfo = nullptr;
    ServerInfo* m_pMSServerInfo   = nullptr;
    std::uint32_t m_MaxServerConnectionIndex = 0;
};

// ---- Lifecycle ----

inline ServerTable make_server_table(std::size_t bucket_hint = 0) {
    ServerTable t;
    if (bucket_hint > 0) t.m_Table.reserve(bucket_hint);
    return t;
}

inline void server_table_init(ServerTable& t, std::size_t bucket_hint = 0) {
    t.m_Table.clear();
    if (bucket_hint > 0) t.m_Table.reserve(bucket_hint);
    t.m_pSelfServerInfo = nullptr;
    t.m_pMSServerInfo   = nullptr;
    t.m_MaxServerConnectionIndex = 0;
}

// Mirrors legacy Release() / RemoveAllServer() combined: clear all entries
// plus the self / MS pointers. Legacy RemoveAllServer also `delete`d the
// SERVERINFO objects; in modern the registry owns them via the map, so no
// separate delete is needed.
inline void server_table_release(ServerTable& t) {
    t.m_Table.clear();
    t.m_pSelfServerInfo = nullptr;
    t.m_pMSServerInfo   = nullptr;
}

// ---- Lookup ----

inline ServerInfo* find_server(ServerTable& t, std::uint16_t port) {
    auto it = t.m_Table.find(port);
    return it == t.m_Table.end() ? nullptr : &it->second;
}

inline const ServerInfo* find_server(const ServerTable& t, std::uint16_t port) {
    auto it = t.m_Table.find(port);
    return it == t.m_Table.end() ? nullptr : &it->second;
}

inline ServerInfo* find_server_for_connection_index(ServerTable& t,
                                                     std::uint32_t dwConnectionIndex) {
    for (auto& kv : t.m_Table) {
        if (kv.second.dwConnectionIndex == dwConnectionIndex) return &kv.second;
    }
    return nullptr;
}

inline const ServerInfo* find_server_for_connection_index(const ServerTable& t,
                                                           std::uint32_t dwConnectionIndex) {
    for (const auto& kv : t.m_Table) {
        if (kv.second.dwConnectionIndex == dwConnectionIndex) return &kv.second;
    }
    return nullptr;
}

// FindMapServer(MapNum): legacy = GetServerPort(MAP_SERVER, MapNum) + FindServer(port).
inline ServerInfo* find_map_server(ServerTable& t, std::uint16_t map_num) {
    std::uint16_t port = 0;
    for (const auto& kv : t.m_Table) {
        if (kv.second.wServerKind == static_cast<std::uint16_t>(ServerKind::MAP_SERVER)
            && kv.second.wServerNum == map_num) {
            port = kv.first;
            break;
        }
    }
    return port == 0 ? nullptr : find_server(t, port);
}

// GetServerPort / GetServerNum: legacy linear scans keyed by port.
inline std::uint16_t get_server_port(const ServerTable& t,
                                      ServerKind kind, std::uint16_t num) {
    for (const auto& kv : t.m_Table) {
        if (kv.second.wServerKind == static_cast<std::uint16_t>(kind)
            && kv.second.wServerNum == num) {
            return kv.first;
        }
    }
    return 0;
}

inline std::uint16_t get_server_num(const ServerTable& t, std::uint16_t port) {
    auto it = t.m_Table.find(port);
    if (it == t.m_Table.end()) return 0;
    return it->second.wServerNum;
}

// ---- Mutate ----

inline void add_server(ServerTable& t, const ServerInfo& info, std::uint16_t port) {
    t.m_Table[port] = info;
}

inline void add_server(ServerTable& t, ServerInfo&& info, std::uint16_t port) {
    t.m_Table[port] = std::move(info);
}

// AddSelfServer / AddMSServer: legacy asserts no prior assignment; modern
// returns false if a self / MS server is already set.
inline bool add_self_server(ServerTable& t, ServerInfo& info) {
    if (t.m_pSelfServerInfo != nullptr) return false;
    t.m_pSelfServerInfo = &info;
    return true;
}

inline bool add_ms_server(ServerTable& t, ServerInfo& info) {
    if (t.m_pMSServerInfo != nullptr) return false;
    t.m_pMSServerInfo = &info;
    return true;
}

inline ServerInfo* get_self_server(ServerTable& t) { return t.m_pSelfServerInfo; }
inline ServerInfo* get_ms_server(ServerTable& t)   { return t.m_pMSServerInfo; }

// RemoveServer(dwConnectionIndex): legacy linear scan, removes by port key.
//
// NOTE: legacy returns the raw SERVERINFO pointer AFTER calling Remove(),
// which leaves a dangling reference (legacy CYHHashTable marks the node
// as not-exist without deleting it, so the legacy pointer is "stale but
// readable"; the modern unordered_map actually erases the slot and the
// pointer would dangle). Modern fixes the lifetime bug by returning the
// removed value by copy (std::optional<ServerInfo>), matching the pattern
// already used by remove_server_by_port.
inline std::optional<ServerInfo> remove_server_by_connection_index(ServerTable& t,
                                                                    std::uint32_t dwConnectionIndex) {
    for (auto it = t.m_Table.begin(); it != t.m_Table.end(); ++it) {
        if (it->second.dwConnectionIndex == dwConnectionIndex) {
            ServerInfo out = it->second;
            std::uint16_t port = it->first;
            (void)port;
            t.m_Table.erase(it);
            return out;
        }
    }
    return std::nullopt;
}

// RemoveServer(port): legacy returns the SERVERINFO and erases by key.
inline ServerInfo remove_server_by_port(ServerTable& t, std::uint16_t port) {
    auto it = t.m_Table.find(port);
    if (it == t.m_Table.end()) return ServerInfo{};
    ServerInfo out = it->second;
    t.m_Table.erase(it);
    return out;
}

// RemoveAllServer: legacy walked and `delete`d every entry, then RemoveAll().
// Modern just clears the map (no manual delete needed).
inline void remove_all_servers(ServerTable& t) {
    t.m_Table.clear();
}

// ---- GetFastServer ----
//
// legacy picks the entry where (dwConnectionIndex != 0 OR wServerKind == kind)
// AND wAgentUserCnt is the minimum. Returns the connection index (legacy
// stores it directly) plus an out IP / port form.

struct FastServerPick {
    bool           valid = false;
    ServerInfo*    info  = nullptr;
    std::uint32_t  connection_index = 0;
};

inline FastServerPick get_fast_server(ServerTable& t, ServerKind kind) {
    FastServerPick pick;
    std::uint32_t min_cnt = 0xFFFFFFFFu;
    ServerInfo*   min_info = nullptr;
    for (auto& kv : t.m_Table) {
        ServerInfo& info = kv.second;
        if (info.dwConnectionIndex != 0u
            || info.wServerKind == static_cast<std::uint16_t>(kind)) {
            if (info.wAgentUserCnt < min_cnt) {
                min_cnt  = info.wAgentUserCnt;
                min_info = &info;
            }
        }
    }
    if (min_info == nullptr) return pick;
    pick.valid            = true;
    pick.info             = min_info;
    pick.connection_index = min_info->dwConnectionIndex;
    return pick;
}

// (kind, ip_buf, port_out) overload mirrors legacy GetFastServer 2nd form.
inline bool get_fast_server(ServerTable& t, ServerKind kind,
                             char* out_ip, std::uint16_t* out_port) {
    FastServerPick pick = get_fast_server(t, kind);
    if (!pick.valid || pick.info == nullptr) return false;
    if (out_ip != nullptr) {
        std::strncpy(out_ip, pick.info->szIPForUser, MAX_IPADDRESS_SIZE - 1u);
        out_ip[MAX_IPADDRESS_SIZE - 1u] = 0;
    }
    if (out_port != nullptr) *out_port = pick.info->wPortForUser;
    return true;
}

// ---- MaxServerConnectionIndex ----

inline std::uint32_t get_max_server_connection_index(const ServerTable& t) {
    return t.m_MaxServerConnectionIndex;
}
inline void set_max_server_connection_index(ServerTable& t, std::uint32_t val) {
    t.m_MaxServerConnectionIndex = val;
}

// ---- Snapshot helpers (modern alternative to legacy GetNext*Server) ----

inline std::vector<ServerInfo> collect_servers(const ServerTable& t) {
    std::vector<ServerInfo> out;
    out.reserve(t.m_Table.size());
    for (const auto& kv : t.m_Table) out.push_back(kv.second);
    return out;
}

} // namespace mxh::server

