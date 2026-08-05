#pragma once
//
// 1:1 port of the legacy ITEM_INFO struct from
//   `??????\[CC]Header\GameResourceStruct.h` lines 476-544.
// Names and sizes are 1:1 with the legacy struct so the wire and
// ItemList.bin text layout stay in sync. Field-level comments keep
// the legacy Korean annotations for cross-reference.

#include <cstdint>

namespace mxh::game {

inline constexpr std::uint16_t ITEM_MAX_NAME  = 31;  // MAX_ITEMNAME_LENGTH + 1
inline constexpr std::uint16_t ITEM_ELEM_MAX  = 5;   // 5 attribute elements

struct ItemAttribReg {
    float Element[ITEM_ELEM_MAX] = {};  // ATTRIBUTEREGIST, 5 floats
};

struct ItemAttribAtt {
    float Element[ITEM_ELEM_MAX] = {};  // ATTRIBUTEATTACK, 5 floats
};

struct ItemInfo {
    // --- identity (offset 0) ---
    std::uint16_t ItemIdx       = 0;
    char          ItemName[ITEM_MAX_NAME] = {};
    std::uint16_t ItemTooltipIdx = 0;
    std::uint16_t Image2DNum    = 0;
    // --- classification ---
    std::uint16_t ItemKind      = 0;  // 0 potion 1 mugongbook 2 equip 3 etc
    std::uint32_t BuyPrice      = 0;
    std::uint32_t SellPrice     = 0;
    std::uint32_t Rarity        = 0;
    // --- stats / attributes (equip + potion shared) ---
    std::uint16_t WeaponType    = 0;
    std::uint16_t GenGol        = 0;
    std::uint16_t MinChub       = 0;
    std::uint16_t CheRyuk       = 0;
    std::uint16_t SimMek        = 0;
    std::uint32_t Life          = 0;
    std::uint32_t Shield        = 0;
    std::uint16_t NaeRyuk       = 0;
    ItemAttribReg AttrRegist;
    // --- equip-only limits ---
    std::uint16_t LimitJob      = 0;
    std::uint16_t LimitGender   = 0;
    std::uint16_t LimitLevel    = 0;  // LEVELTYPE = WORD
    std::uint16_t LimitGenGol   = 0;
    std::uint16_t LimitMinChub  = 0;
    std::uint16_t LimitCheRyuk  = 0;
    std::uint16_t LimitSimMek   = 0;
    // --- equip combat ---
    std::uint16_t ItemGrade     = 0;
    std::uint16_t RangeType     = 0;
    std::uint16_t EquipKind     = 0;
    std::uint16_t Part3DType    = 0;
    std::uint16_t Part3DModelNum = 0;
    std::uint16_t MeleeAttackMin = 0;
    std::uint16_t MeleeAttackMax = 0;
    std::uint16_t RangeAttackMin = 0;
    std::uint16_t RangeAttackMax = 0;
    std::uint16_t CriticalPercent = 0;
    ItemAttribAtt AttrAttack;
    std::uint16_t PhyDef        = 0;
    std::uint16_t Plus_MugongIdx = 0;
    std::uint16_t Plus_Value    = 0;
    std::uint16_t AllPlus_Kind  = 0;
    std::uint16_t AllPlus_Value = 0;
    // --- mugongbook ---
    std::uint16_t MugongNum     = 0;
    std::uint16_t MugongType    = 0;
    // --- potion (recovery) ---
    std::uint16_t LifeRecover       = 0;
    float         LifeRecoverRate   = 0.0f;
    std::uint16_t NaeRyukRecover    = 0;
    float         NaeRyukRecoverRate = 0.0f;
    // --- etc (quest, recipe) ---
    std::uint16_t ItemType      = 0;
    // --- japan-local fields (only parsed when 76-token row is read) ---
    std::uint16_t wItemAttr        = 0;
    std::uint16_t wAcquireSkillIdx1 = 0;
    std::uint16_t wAcquireSkillIdx2 = 0;
    std::uint16_t wDeleteSkillIdx  = 0;
    // --- set item kind ---
    std::uint16_t wSetItemKind  = 0;
};

}  // namespace mxh::game
