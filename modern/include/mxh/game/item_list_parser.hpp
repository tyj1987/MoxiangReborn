#pragma once
//
// 1:1 parser for `Resource/ItemList.bin`, the packed-text item
// table from the legacy ?? client.  1:1 with the legacy parser
// in `??????\[Server]Map\ItemManager.cpp::SetItemInfo` and the
// MHFile packed-text reader in
// `??????\[Client]MH\MHFile.cpp::OpenBin` / `CheckCRC`.
//
// File layout (all little-endian):
//   struct Header { uint32_t dwVersion; uint32_t dwType; uint32_t FileSize; };
//   Header  (12 bytes)
//   uint8_t crc1
//   uint8_t data[FileSize]   -- packed-text, EUC-KR per row
//   uint8_t crc2
//
// Decoding reuses mxh::compat::detail::decode_mhfile_text_payload
// (1:1 with MHFile::CheckCRC lines 188-197).
//
// Per-row layout (1:1 with ItemManager.cpp::SetItemInfo lines 3956-4023).
// Two variants are produced by the legacy code:
//   * Common row (no _JAPAN_LOCAL_): 56 tokens
//   * Japan-local row:               56 + 4 = 60 tokens
// The extra 4 are wItemAttr, wAcquireSkillIdx1, wAcquireSkillIdx2,
// wDeleteSkillIdx. We keep the same fields on ItemInfo and zero them
// when parsing the 65-token row.
//
// 56 tokens are laid out as:
//   0         ItemIdx            (u16)
//   1         ItemName           (string, 30+1 NUL, EUC-KR)
//   2         ItemTooltipIdx     (u16)
//   3         Image2DNum         (u16)
//   4         ItemKind           (u16)
//   5         BuyPrice           (u32)
//   6         SellPrice          (u32)
//   7         Rarity             (u32)
//   8         WeaponType         (u16)
//   9         GenGol             (u16)
//   10        MinChub            (u16)
//   11        CheRyuk            (u16)
//   12        SimMek             (u16)
//   13        Life               (u32)
//   14        Shield             (u32)
//   15        NaeRyuk            (u16)
//   16-20     AttrRegist 5 floats (FIRE WATER TREE IRON EARTH)
//   21        LimitJob           (u16)
//   22        LimitGender        (u16)
//   23        LimitLevel         (u16)
//   24        LimitGenGol        (u16)
//   25        LimitMinChub       (u16)
//   26        LimitCheRyuk       (u16)
//   27        LimitSimMek        (u16)
//   28        ItemGrade          (u16)
//   29        RangeType          (u16)
//   30        EquipKind          (u16)
//   31        Part3DType         (u16)
//   32        Part3DModelNum     (u16)
//   33        MeleeAttackMin     (u16)
//   34        MeleeAttackMax     (u16)
//   35        RangeAttackMin     (u16)
//   36        RangeAttackMax     (u16)
//   37        CriticalPercent    (u16)
//   38-42     AttrAttack 5 floats (FIRE WATER TREE IRON EARTH)
//   43        PhyDef             (u16)
//   44        Plus_MugongIdx     (u16)
//   45        Plus_Value         (u16)
//   46        AllPlus_Kind       (u16)
//   47        AllPlus_Value      (u16)
//   48        MugongNum          (u16)
//   49        MugongType         (u16)
//   50        LifeRecover        (u16)
//   51        LifeRecoverRate    (f32)
//   52        NaeRyukRecover     (u16)
//   53        NaeRyukRecoverRate (f32)
//   54        ItemType           (u16)
//   55-58     wItemAttr, wAcquireSkillIdx1, wAcquireSkillIdx2, wDeleteSkillIdx (u16 each, JAPAN_LOCAL only)
//   59        wSetItemKind       (u16)
// The legacy SetItemInfo stops here (5 more u16 for JAPAN_LOCAL =
// wItemAttr / 2 / 3 / wDeleteSkillIdx are read only inside
// #ifdef _JAPAN_LOCAL_; see lines 4016-4020 of ItemManager.cpp).
// 60-63     the four japan-local fields
//   64        wSetItemKind       (u16, legacy line 4023)
//
// Total = 56 tokens without JAPAN_LOCAL, 60 tokens with JAPAN_LOCAL.

#include "mxh/game/item_list_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::game {

struct ItemListParseResult {
    std::vector<ItemInfo>  items;
    std::uint32_t          rows_seen = 0;
    std::uint32_t          parse_errors = 0;
    std::uint8_t           decoded_crc = 0;
    std::uint8_t           stored_crc = 0;
    std::string            error_message;
};

ItemListParseResult load_item_list(const std::string& path);

bool parse_item_row(const std::vector<std::string>& tokens,
                    ItemInfo& out,
                    std::string& parse_error_msg);

}  // namespace mxh::game
