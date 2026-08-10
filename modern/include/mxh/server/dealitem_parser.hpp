// Phase 13.4: 1:1 port of the legacy [Server]Map/ItemManager.cpp::LoadDealerItem
// data plane for the NPC shop catalog (Resource/DealItem.bin).
//
// This header ships the data plane only: pure-function parser that
// turns the raw bytes of a DealItem.bin file into a per-NPC catalog
// table (npc_index -> tab -> [item_idx, item_count]).
//
// Wire / on-disk layout (1:1 with legacy [Server]Map/ItemManager.cpp:
// LoadDealerItem lines 6702-6809 and [Server]Map/MHFile.cpp::CheckCRC
// lines 462-489):
//
//   struct MhFileHeader { uint32 version, type, file_size; }  (12 B)
//   uint8  crc1
//   uint8  data[file_size]   -- CRLF-terminated text, EUC-KR tokens
//   uint8  crc2
//
// Each decoded line is a (npc, items-for-tab) row:
//   [0]  WORD   map_num
//   [1]  string mapname        (EUC-KR)
//   [2]  WORD   npc_kind
//   [3]  string npcname        (EUC-KR)
//   [4]  WORD   npc_index      (== legacy m_DealerTable key)
//   [5]  WORD   point_x
//   [6]  WORD   point_z
//   [7]  WORD   angle
//   [8]  BYTE   tabnum         (1-based; items in this line go to
//                              tab=tabnum-1 in the legacy struct)
//   ... then item triples, each 3 tokens:
//        [9+3k+0]  string tabname        (EUC-KR, ignored)
//        [9+3k+1]  WORD   item_idx       (== legacy DealerItem.ItemIdx)
//        [9+3k+2]  int    item_count     (-1 == unlimited, 0 == not
//                                        sold, >0 == stock; legacy
//                                        DealerItem.ItemCount)
//
// 1:1 quirks preserved:
//   * The file is grouped by npc_index: one physical line per NPC
//     tab, multiple lines per NPC (the legacy parser reads each line
//     independently, so multiple tabs of the same NPC appear as
//     consecutive lines).
//   * Empty / 0-item lines are valid (NPC exists but the tab is
//     empty) -- they still get aggregated under the same npc_index.
//   * Within a tab, position is 0-based and monotonically increases
//     (legacy assigns Pos++ per item).
//   * Strings (mapname / npcname / tabname) are stored as raw bytes
//     and never transcoded; the parser just keeps them as std::string
//     (Latin-1 safe), matching how the rest of the 1:1 ports handle
//     EUC-KR.

#pragma once

#include "cstdint"
#include "filesystem"
#include "optional"
#include "span"
#include "string"
#include "unordered_map"
#include "vector"

namespace mxh::server {

// Forward decl so this header stays self-contained even when
// the data-plane adapter (catalog_for_npc) is used in the .cpp.
struct NpcShopCatalog;

// ---- One catalog row (per NPC x per item) ----
struct DealItemEntry final {
    std::uint32_t item_idx   = 0;
    std::uint32_t item_count = 0;  // -1=unlimited, 0=not-sold, >0=stock
    std::uint16_t tab        = 0;  // 0-based
    std::uint16_t pos        = 0;  // 0-based slot within the tab
};

// ---- Per-NPC metadata + tab x pos matrix ----
struct DealItemNPC final {
    std::uint16_t npc_index = 0;
    std::uint16_t map_num   = 0;
    std::uint16_t npc_kind  = 0;
    std::uint16_t point_x   = 0;
    std::uint16_t point_z   = 0;
    std::uint16_t angle     = 0;
    std::uint8_t  max_tabs  = 0;  // highest tabnum observed for this NPC

    std::string mapname;
    std::string npcname;

    // tabs[tab][pos] = item entry. tabs is sized so that
    // tabs.size() == max_tabs; missing slots are zero-valued.
    std::vector<std::vector<DealItemEntry>> tabs;

    // Flat lookup across all tabs/positions.
    const DealItemEntry* find_item(std::uint32_t item_idx) const noexcept;
};

// ---- Top-level parse result ----
struct DealItemParseResult final {
    std::vector<DealItemNPC> npcs;
    std::unordered_map<std::uint32_t, std::size_t> npc_index_to_offset;

    std::uint32_t rows_seen    = 0;
    std::uint32_t rows_parsed  = 0;
    std::uint32_t parse_errors = 0;
    std::uint32_t file_type    = 0;
    std::uint8_t  decoded_crc  = 0;
    std::uint8_t  stored_crc   = 0;
    std::string   error_message;

    const DealItemNPC* find_npc(std::uint32_t npc_index) const noexcept;
};

// ---- Pure-function parsers ----
//
// parse_dealitem_bytes() runs the full XOR-decode + tokenize + tab-
// aggregation pipeline on an already-loaded buffer (use this in
// tests and for in-memory tooling).
//
// load_dealitem() wraps the disk read + parse step.  Both return by
// value; the result is small enough that an std::vector of NPC
// entries is the canonical format.
DealItemParseResult parse_dealitem_bytes(std::span<const std::uint8_t> raw);
DealItemParseResult load_dealitem(const std::filesystem::path& path);

// ---- Data-plane adapter ----
//
// Build a flat NpcShopCatalog from one NPC tabs.  Stock == -1 maps
// to unlimited (entry.stock = 0).  Returns nullopt if npc_index is
// unknown.  The price field is filled by the orchestrator from the
// ItemList catalog; this adapter only carries the item_id + stock
// needed to validate a BuySyn.
std::optional<NpcShopCatalog>
catalog_for_npc(const DealItemParseResult& r,
                std::uint32_t npc_index) noexcept;

}  // namespace mxh::server