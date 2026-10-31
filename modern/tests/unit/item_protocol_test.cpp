// Item protocol numbers against the SHA-pinned original profile.
// Pickup must not occupy original Buy, and every modern Item value that
// has an original counterpart must use that counterpart's number.
// Ground-drop versus direct-to-inventory payload paths are a separate change.

#include "mxh/proto/ground_item.hpp"
#include "mxh/proto/original_item_obtain_codec.hpp"
#include "mxh/proto/original_item_profile.hpp"
#include "mxh/proto/protocol.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace {

using Modern = mxh::proto::ItemProtocol;
using Original = mxh::proto::original_item::ItemProtocol;

unsigned modern_number(Modern value) {
    return static_cast<unsigned>(value);
}

unsigned original_number(Original value) {
    return static_cast<unsigned>(value);
}

void expect_same_number(Modern modern, Original original) {
    EXPECT_EQ(modern_number(modern), original_number(original))
        << "modern=" << modern_number(modern)
        << " original=" << original_number(original);
}


constexpr Original kOriginalItemProtocols[] = {
    Original::MP_ITEM_TOTALINFO_LOCAL,
    Original::MP_ITEM_PYOGUKITEM_INFO,
    Original::MP_ITEM_USE_SYN,
    Original::MP_ITEM_USE_ACK,
    Original::MP_ITEM_USE_NACK,
    Original::MP_ITEM_COMBINE_SYN,
    Original::MP_ITEM_COMBINE_ACK,
    Original::MP_ITEM_COMBINE_NACK,
    Original::MP_ITEM_DIVIDE_SYN,
    Original::MP_ITEM_DIVIDE_ACK,
    Original::MP_ITEM_DIVIDE_NACK,
    Original::MP_ITEM_DIVIDE_NEWITEM_NOTIFY,
    Original::MP_ITEM_DISCARD_SYN,
    Original::MP_ITEM_DISCARD_ACK,
    Original::MP_ITEM_DISCARD_NACK,
    Original::MP_ITEM_ERROR_NACK,
    Original::MP_ITEM_MOVE_SYN,
    Original::MP_ITEM_MOVE_ACK,
    Original::MP_ITEM_MOVE_NACK,
    Original::MP_ITEM_BUY_SYN,
    Original::MP_ITEM_BUY_ACK,
    Original::MP_ITEM_BUY_NACK,
    Original::MP_ITEM_SELL_SYN,
    Original::MP_ITEM_SELL_ACK,
    Original::MP_ITEM_SELL_NACK,
    Original::MP_ITEM_DEALER_SYN,
    Original::MP_ITEM_DEALER_ACK,
    Original::MP_ITEM_DEALER_NACK,
    Original::MP_ITEM_MONEY,
    Original::MP_ITEM_MONEY_ERROR,
    Original::MP_ITEM_APPEARANCE_CHANGE,
    Original::MP_ITEM_OBTAIN_MONEY,
    Original::MP_ITEM_MONSTER_OBTAIN_NOTIFY,
    Original::MP_ITEM_UPGRADE_SYN,
    Original::MP_ITEM_UPGRADE_SUCCESS_ACK,
    Original::MP_ITEM_UPGRADE_FAILED_ACK,
    Original::MP_ITEM_UPGRADE_NACK,
    Original::MP_ITEM_MIX_SYN,
    Original::MP_ITEM_MIX_SUCCESS_ACK,
    Original::MP_ITEM_MIX_BIGFAILED_ACK,
    Original::MP_ITEM_MIX_FAILED_ACK,
    Original::MP_ITEM_MIX_MSG,
    Original::MP_ITEM_REINFORCE_SYN,
    Original::MP_ITEM_REINFORCE_SUCCESS_ACK,
    Original::MP_ITEM_REINFORCE_FAILED_ACK,
    Original::MP_ITEM_REINFORCE_NACK,
    Original::MP_ITEM_DISSOLVE_SYN,
    Original::MP_ITEM_DISSOLVE_SUCCESS_ACK,
    Original::MP_ITEM_DISSOLVE_FAILED_ACK,
    Original::MP_ITEM_DISSOLVE_NACK,
    Original::MP_ITEM_MUNPA_COMBINE_SYN,
    Original::MP_ITEM_MUNPA_COMBINE_ACK,
    Original::MP_ITEM_MUNPA_COMBINE_NACK,
    Original::MP_ITEM_MUNPA_MOVE_SYN,
    Original::MP_ITEM_MUNPA_MOVE_ACK,
    Original::MP_ITEM_MUNPA_MOVE_NACK,
    Original::MP_ITEM_MUNPA_WAREHOUSE_INFO_SYN,
    Original::MP_ITEM_MUNPA_WAREHOUSE_INFO_ACK,
    Original::MP_ITEM_MUNPA_WAREHOUSE_INFO_NACK,
    Original::MP_ITEM_ADDTO_MUNPAWARE_NOTIFY,
    Original::MP_ITEM_DELETEFROM_MUNPAWARE_NOTIFY,
    Original::MP_ITEM_MUNPA_WAREHOUSE_LEAVE,
    Original::MP_ITEM_PYOGUK_MOVE_SYN,
    Original::MP_ITEM_PYOGUK_MOVE_ACK,
    Original::MP_ITEM_PYOGUK_MOVE_NACK,
    Original::MP_ITEM_PYOGUK_COMBINE_SYN,
    Original::MP_ITEM_PYOGUK_COMBINE_ACK,
    Original::MP_ITEM_PYOGUK_COMBINE_NACK,
    Original::MP_ITEM_PYOGUK_ITEM_INFO_SYN,
    Original::MP_ITEM_PYOGUK_ITEM_INFO_ACK,
    Original::MP_ITEM_PYOGUK_ITEM_INFO_NACK,
    Original::MP_ITEM_MIX_ADDITEM_SYN,
    Original::MP_ITEM_MIX_ADDITEM_ACK,
    Original::MP_ITEM_MIX_ADDITEM_NACK,
    Original::MP_ITEM_MIX_RELEASEITEM,
    Original::MP_ITEM_USE_FOR_QUESTSTART_SYN,
    Original::MP_ITEM_USE_FOR_QUESTSTART_ACK,
    Original::MP_ITEM_USE_FOR_QUESTSTART_NACK,
    Original::MP_ITEM_DISSOLUTION_SYN,
    Original::MP_ITEM_DISSOLUTION_ACK,
    Original::MP_ITEM_DISSOLUTION_NACK,
    Original::MP_ITEM_DISSOLUTION_GETITEM,
    Original::MP_ITEM_USE_CHANGEITEM_SYN,
    Original::MP_ITEM_USE_CHANGEITEM_ACK,
    Original::MP_ITEM_USE_CHANGEITEM_NACK,
    Original::MP_ITEM_GUILD_COMBINE_SYN,
    Original::MP_ITEM_GUILD_COMBINE_ACK,
    Original::MP_ITEM_GUILD_COMBINE_NACK,
    Original::MP_ITEM_GUILD_MOVE_SYN,
    Original::MP_ITEM_GUILD_MOVE_ACK,
    Original::MP_ITEM_GUILD_MOVE_NACK,
    Original::MP_ITEM_GUILD_WAREHOUSE_INFO_SYN,
    Original::MP_ITEM_GUILD_WAREHOUSE_INFO_ACK,
    Original::MP_ITEM_GUILD_WAREHOUSE_INFO_NACK,
    Original::MP_ITEM_ADDTO_GUILDWARE_NOTIFY,
    Original::MP_ITEM_DELETEFROM_GUILDWARE_NOTIFY,
    Original::MP_ITEM_GUILD_WAREHOUSE_LEAVE,
    Original::MP_ITEM_SHOPITEM_INFO_SYN,
    Original::MP_ITEM_SHOPITEM_INFO_ACK,
    Original::MP_ITEM_SHOPITEM_INFO_NACK,
    Original::MP_ITEM_SHOPITEM_USEDINFO,
    Original::MP_ITEM_SHOPITEM_MPINFO,
    Original::MP_ITEM_SHOPITEM_USE_SYN,
    Original::MP_ITEM_SHOPITEM_USE_ACK,
    Original::MP_ITEM_SHOPITEM_USE_NACK,
    Original::MP_ITEM_SHOPITEM_TIME_UPDATE,
    Original::MP_ITEM_SHOPITEM_USEEND,
    Original::MP_ITEM_SHOPITEM_MSG,
    Original::MP_ITEM_SHOPITEM_ONEMINUTE,
    Original::MP_ITEM_SHOPITEM_MONEYPROTECT,
    Original::MP_ITEM_SHOPITEM_EXPPROTECT,
    Original::MP_ITEM_SHOPITEM_MIXUP,
    Original::MP_ITEM_SHOPITEM_STATEMINUS,
    Original::MP_ITEM_SHOPITEM_UNSEAL_SYN,
    Original::MP_ITEM_SHOPITEM_UNSEAL_ACK,
    Original::MP_ITEM_SHOPITEM_UNSEAL_NACK,
    Original::MP_ITEM_SHOPITEM_CHANGEMAP_SYN,
    Original::MP_ITEM_SHOPITEM_CHANGEMAP_ACK,
    Original::MP_ITEM_SHOPITEM_CHANGEMAP_NACK,
    Original::MP_ITEM_SHOPITEM_SAVEPOINT_ADD_SYN,
    Original::MP_ITEM_SHOPITEM_SAVEPOINT_ADD_ACK,
    Original::MP_ITEM_SHOPITEM_SAVEPOINT_ADD_NACK,
    Original::MP_ITEM_SHOPITEM_SAVEPOINT_UPDATE_SYN,
    Original::MP_ITEM_SHOPITEM_SAVEPOINT_UPDATE_ACK,
    Original::MP_ITEM_SHOPITEM_SAVEPOINT_UPDATE_NACK,
    Original::MP_ITEM_SHOPITEM_SAVEPOINT_DEL_SYN,
    Original::MP_ITEM_SHOPITEM_SAVEPOINT_DEL_ACK,
    Original::MP_ITEM_SHOPITEM_SAVEPOINT_DEL_NACK,
    Original::MP_ITEM_SHOPITEM_REVIVEOTHER_SYN,
    Original::MP_ITEM_SHOPITEM_REVIVEOTHER_ACK,
    Original::MP_ITEM_SHOPITEM_REVIVEOTHER_NACK,
    Original::MP_ITEM_SHOPITEM_AVATAR_USE_SYN,
    Original::MP_ITEM_SHOPITEM_AVATAR_USE_ACK,
    Original::MP_ITEM_SHOPITEM_AVATAR_USE_NACK,
    Original::MP_ITEM_SHOPITEM_AVATAR_PUTON,
    Original::MP_ITEM_SHOPITEM_AVATAR_TAKEOFF,
    Original::MP_ITEM_SHOPITEM_AVATAR_CHANGE,
    Original::MP_ITEM_SHOPITEM_AVATAR_USE,
    Original::MP_ITEM_SHOPITEM_AVATAR_INFO,
    Original::MP_ITEM_SHOPITEM_AVATAR_DISCARD,
    Original::MP_ITEM_SHOPITEM_PET_SUMMON_SYN,
    Original::MP_ITEM_SHOPITEM_PET_SUMMON_ACK,
    Original::MP_ITEM_SHOPITEM_PET_SUMMON_NACK,
    Original::MP_ITEM_SHOPITEM_PET_SEAL_SYN,
    Original::MP_ITEM_SHOPITEM_PET_SEAL_ACK,
    Original::MP_ITEM_SHOPITEM_PET_SEAL_NACK,
    Original::MP_ITEM_SHOPITEM_PET_USE,
    Original::MP_ITEM_SHOPITEM_EFFECT,
    Original::MP_ITEM_SHOPITEM_SLOTOVERITEM,
    Original::MP_ITEM_EVENTITEM_USE,
    Original::MP_ITEM_SHOPITEM_PROTECTALL,
    Original::MP_ITEM_SHOPITEM_CHASEUSE_SYN,
    Original::MP_ITEM_SHOPITEM_CHASEUSE_ACK,
    Original::MP_ITEM_SHOPITEM_CHASEUSE_NACK,
    Original::MP_ITEM_SHOPITEM_CHASE_SYN,
    Original::MP_ITEM_SHOPITEM_CHASE_ACK,
    Original::MP_ITEM_SHOPITEM_CHASE_NACK,
    Original::MP_ITEM_SHOPITEM_CHASE_TRACKING,
    Original::MP_ITEM_SHOPITEM_NCUSE_SYN,
    Original::MP_ITEM_SHOPITEM_NCUSE_ACK,
    Original::MP_ITEM_SHOPITEM_NCUSE_NACK,
    Original::MP_ITEM_SHOPITEM_NCHANGE_SYN,
    Original::MP_ITEM_SHOPITEM_NCHANGE_ACK,
    Original::MP_ITEM_SHOPITEM_NCHANGE_NACK,
    Original::MP_ITEM_REINFORCE_WITHSHOPITEM_SYN,
    Original::MP_ITEM_REINFORCE_WITHSHOPITEM_ACK,
    Original::MP_ITEM_REINFORCE_WITHSHOPITEM_NACK,
    Original::MP_ITEM_SHOPITEM_CHARCHANGE_SYN,
    Original::MP_ITEM_SHOPITEM_CHARCHANGE_ACK,
    Original::MP_ITEM_SHOPITEM_CHARCHANGE_NACK,
    Original::MP_ITEM_SHOPITEM_CHARCHANGE,
    Original::MP_ITEM_SHOPITEM_SEAL_SYN,
    Original::MP_ITEM_SHOPITEM_SEAL_ACK,
    Original::MP_ITEM_SHOPITEM_SEAL_NACK,
    Original::MP_ITEM_MONSTER_RAREOBTAIN_NOTIFY,
    Original::MP_ITEM_SHOPITEM_JOBCHANGE_SYN,
    Original::MP_ITEM_SHOPITEM_JOBCHANGE_ACK,
    Original::MP_ITEM_SHOPITEM_JOBCHANGE_NACK,
    Original::MP_ITEM_USE_NOTIFY,
    Original::MP_ITEM_SHOPITEM_REINFORCERESET_SYN,
    Original::MP_ITEM_SHOPITEM_REINFORCERESET_ACK,
    Original::MP_ITEM_SHOPITEM_REINFORCERESET_NACK,
    Original::MP_ITEM_SHOPITEM_RARECREATE_SYN,
    Original::MP_ITEM_SHOPITEM_RARECREATE_ACK,
    Original::MP_ITEM_SHOPITEM_RARECREATE_NACK,
    Original::MP_ITEM_PETINVEN_INFO_SYN,
    Original::MP_ITEM_PETINVEN_INFO_ACK,
    Original::MP_ITEM_PETINVEN_INFO_NACK,
    Original::MP_ITEM_SHOPITEM_SHOUT_SYN,
    Original::MP_ITEM_SHOPITEM_SHOUT_ACK,
    Original::MP_ITEM_SHOPITEM_SHOUT_NACK,
    Original::MP_ITEM_SHOPITEM_SHOUT_SENDSERVER,
    Original::MP_ITEM_SHOPITEM_SHOUT_SENDACK,
    Original::MP_ITEM_CHANGEITEM_NACK,
    Original::MP_ITEM_SHOPITEM_AVATAR_USE_ACKDB,
    Original::MP_ITEM_SHOPITEM_EQUIP_USEINFODB,
    Original::MP_ITEM_FULLINVEN_ALERT,
    Original::MP_ITEM_TPM_SYN,
    Original::MP_ITEM_TPM_SUCCESS_ACK,
    Original::MP_ITEM_TPM_BIGFAILED_ACK,
    Original::MP_ITEM_TPM_FAILED_ACK,
    Original::MP_ITEM_TPM_MSG,
    Original::MP_ITEM_TPM_ADDITEM_SYN,
    Original::MP_ITEM_TPM_ADDITEM_ACK,
    Original::MP_ITEM_TPM_ADDITEM_NACK,
    Original::MP_ITEM_TPM_RELEASEITEM,
    Original::MP_ITEM_TITANMIX_SYN,
    Original::MP_ITEM_TITANMIX_ACK,
    Original::MP_ITEM_TITANMIX_ACK_DELETEPARTS,
    Original::MP_ITEM_TITANMIX_NACK,
    Original::MP_ITEM_TITANMIX_ADDITEM_SYN,
    Original::MP_ITEM_TITANMIX_ADDITEM_ACK,
    Original::MP_ITEM_TITANMIX_ADDITEM_NACK,
    Original::MP_ITEM_TITANMIX_RELEASEITEM,
    Original::MP_ITEM_TITANUPGRADE_SYN,
    Original::MP_ITEM_TITANUPGRADE_ACK,
    Original::MP_ITEM_TITANUPGRADE_ACK_DELETEMATERIAL,
    Original::MP_ITEM_TITANUPGRADE_NACK,
    Original::MP_ITEM_TITANUPGRADE_ADDITEM_SYN,
    Original::MP_ITEM_TITANUPGRADE_ADDITEM_ACK,
    Original::MP_ITEM_TITANUPGRADE_ADDITEM_NACK,
    Original::MP_ITEM_TITANUPGRADE_RELEASEITEM,
    Original::MP_ITEM_TITAN_REGISTER_SYN,
    Original::MP_ITEM_TITAN_REGISTER_ACK,
    Original::MP_ITEM_TITAN_REGISTER_NACK,
    Original::MP_ITEM_TITAN_REGISTER_ADDITEM_SYN,
    Original::MP_ITEM_TITAN_REGISTER_ADDITEM_ACK,
    Original::MP_ITEM_TITAN_REGISTER_ADDITEM_NACK,
    Original::MP_ITEM_TITAN_REGISTER_RELEASEITEM,
    Original::MP_ITEM_TITANBREAK_SYN,
    Original::MP_ITEM_TITANBREAK_ACK,
    Original::MP_ITEM_TITANBREAK_ACK_DELETEITEM,
    Original::MP_ITEM_TITANBREAK_NACK,
    Original::MP_ITEM_TITANBREAK_ADDITEM_SYN,
    Original::MP_ITEM_TITANBREAK_ADDITEM_ACK,
    Original::MP_ITEM_TITANBREAK_ADDITEM_NACK,
    Original::MP_ITEM_TITANBREAK_RELEASEITEM,
    Original::MP_ITEM_TITAN_DISSOLUTION_SYN,
    Original::MP_ITEM_TITAN_DISSOLUTION_ACK,
    Original::MP_ITEM_TITAN_DISSOLUTION_NACK,
    Original::MP_ITEM_TITAN_DISSOLUTION_ADDITEM_SYN,
    Original::MP_ITEM_TITAN_DISSOLUTION_ADDITEM_ACK,
    Original::MP_ITEM_TITAN_DISSOLUTION_ADDITEM_NACK,
    Original::MP_ITEM_TITAN_DISSOLUTION_RELEASEITEM,
    Original::MP_ITEM_TITAN_DISCARD_EQUIPITEM,
    Original::MP_ITEM_TITAN_EQUIP_REPAIR_SYN,
    Original::MP_ITEM_TITAN_EQUIP_REPAIR_ACK,
    Original::MP_ITEM_TITAN_EQUIP_REPAIR_TOTAL_SYN,
    Original::MP_ITEM_TITAN_EQUIP_REPAIR_TOTAL_ACK,
    Original::MP_ITEM_TITAN_EQUIP_REPAIR_NACK,
    Original::MP_ITEM_TITAN_APPEARANCE_CHANGE,
    Original::MP_ITEM_PROTOCOL_MAX,
};

constexpr std::size_t kOriginalItemProtocolCount =
    sizeof(kOriginalItemProtocols) / sizeof(kOriginalItemProtocols[0]);

}  // namespace

TEST(ItemProtocol, BuyMatchesOriginalNumbers) {
    EXPECT_EQ(modern_number(Modern::MoveNack), 18u);
    expect_same_number(Modern::MoveNack, Original::MP_ITEM_MOVE_NACK);

    EXPECT_EQ(modern_number(Modern::BuySyn), 19u);
    EXPECT_EQ(modern_number(Modern::BuyAck), 20u);
    EXPECT_EQ(modern_number(Modern::BuyNack), 21u);
    expect_same_number(Modern::BuySyn, Original::MP_ITEM_BUY_SYN);
    expect_same_number(Modern::BuyAck, Original::MP_ITEM_BUY_ACK);
    expect_same_number(Modern::BuyNack, Original::MP_ITEM_BUY_NACK);
}

TEST(ItemProtocol, PickupDoesNotOccupyOriginalBuy) {
    const unsigned syn = modern_number(Modern::PickupSyn);
    const unsigned ack = modern_number(Modern::PickupAck);
    const unsigned nack = modern_number(Modern::PickupNack);
    const unsigned last_original =
        original_number(Original::MP_ITEM_TITAN_APPEARANCE_CHANGE);
    const unsigned protocol_max =
        original_number(Original::MP_ITEM_PROTOCOL_MAX);

    EXPECT_NE(syn, 19u);
    EXPECT_NE(ack, 20u);
    EXPECT_NE(nack, 21u);
    EXPECT_NE(syn, original_number(Original::MP_ITEM_BUY_SYN));
    EXPECT_NE(ack, original_number(Original::MP_ITEM_BUY_ACK));
    EXPECT_NE(nack, original_number(Original::MP_ITEM_BUY_NACK));
    EXPECT_NE(syn, modern_number(Modern::BuySyn));
    EXPECT_NE(ack, modern_number(Modern::BuyAck));
    EXPECT_NE(nack, modern_number(Modern::BuyNack));

    EXPECT_GT(syn, last_original);
    EXPECT_GT(ack, last_original);
    EXPECT_GT(nack, last_original);
    EXPECT_LT(syn, protocol_max);
    EXPECT_LT(ack, protocol_max);
    EXPECT_LT(nack, protocol_max);
    EXPECT_NE(syn, ack);
    EXPECT_NE(syn, nack);
    EXPECT_NE(ack, nack);
    EXPECT_NE(syn, static_cast<unsigned>(mxh::proto::kModernShopList));
    EXPECT_NE(ack, static_cast<unsigned>(mxh::proto::kModernShopList));
    EXPECT_NE(nack, static_cast<unsigned>(mxh::proto::kModernShopList));
}

TEST(ItemProtocol, DisplacedNumbersMatchOriginal) {
    expect_same_number(Modern::SellSyn, Original::MP_ITEM_SELL_SYN);
    expect_same_number(Modern::SellAck, Original::MP_ITEM_SELL_ACK);
    expect_same_number(Modern::SellNack, Original::MP_ITEM_SELL_NACK);
    expect_same_number(Modern::DealerSyn, Original::MP_ITEM_DEALER_SYN);
    expect_same_number(Modern::DealerAck, Original::MP_ITEM_DEALER_ACK);
    expect_same_number(Modern::DealerNack, Original::MP_ITEM_DEALER_NACK);
    expect_same_number(Modern::Money, Original::MP_ITEM_MONEY);
    expect_same_number(Modern::MoneyError, Original::MP_ITEM_MONEY_ERROR);
    expect_same_number(Modern::AppearanceChange, Original::MP_ITEM_APPEARANCE_CHANGE);
    expect_same_number(Modern::ObtainMoney, Original::MP_ITEM_OBTAIN_MONEY);
    expect_same_number(Modern::MonsterObtainNotify, Original::MP_ITEM_MONSTER_OBTAIN_NOTIFY);
    expect_same_number(Modern::UpgradeSyn, Original::MP_ITEM_UPGRADE_SYN);
    expect_same_number(Modern::UpgradeSuccessAck, Original::MP_ITEM_UPGRADE_SUCCESS_ACK);
    expect_same_number(Modern::UpgradeFailedAck, Original::MP_ITEM_UPGRADE_FAILED_ACK);
    expect_same_number(Modern::UpgradeNack, Original::MP_ITEM_UPGRADE_NACK);
    expect_same_number(Modern::MixSyn, Original::MP_ITEM_MIX_SYN);
    expect_same_number(Modern::MixSuccessAck, Original::MP_ITEM_MIX_SUCCESS_ACK);
    expect_same_number(Modern::MixBigFailedAck, Original::MP_ITEM_MIX_BIGFAILED_ACK);
    expect_same_number(Modern::MixFailedAck, Original::MP_ITEM_MIX_FAILED_ACK);
    expect_same_number(Modern::MixMsg, Original::MP_ITEM_MIX_MSG);
    expect_same_number(Modern::ReinforceSyn, Original::MP_ITEM_REINFORCE_SYN);
    expect_same_number(Modern::ReinforceSuccessAck, Original::MP_ITEM_REINFORCE_SUCCESS_ACK);
    expect_same_number(Modern::ReinforceFailedAck, Original::MP_ITEM_REINFORCE_FAILED_ACK);
    expect_same_number(Modern::ReinforceNack, Original::MP_ITEM_REINFORCE_NACK);
    expect_same_number(Modern::DissolveSyn, Original::MP_ITEM_DISSOLVE_SYN);
    expect_same_number(Modern::DissolveSuccessAck, Original::MP_ITEM_DISSOLVE_SUCCESS_ACK);
    expect_same_number(Modern::DissolveFailedAck, Original::MP_ITEM_DISSOLVE_FAILED_ACK);
    expect_same_number(Modern::DissolveNack, Original::MP_ITEM_DISSOLVE_NACK);
    expect_same_number(Modern::ShopItemChaseSyn, Original::MP_ITEM_SHOPITEM_CHASE_SYN);
    expect_same_number(Modern::ShopItemChaseAck, Original::MP_ITEM_SHOPITEM_CHASE_ACK);
    expect_same_number(Modern::ShopItemChaseNack, Original::MP_ITEM_SHOPITEM_CHASE_NACK);
    expect_same_number(Modern::ShopItemJobChangeSyn, Original::MP_ITEM_SHOPITEM_JOBCHANGE_SYN);
    expect_same_number(Modern::ShopItemJobChangeAck, Original::MP_ITEM_SHOPITEM_JOBCHANGE_ACK);
    expect_same_number(Modern::ShopItemJobChangeNack, Original::MP_ITEM_SHOPITEM_JOBCHANGE_NACK);

    EXPECT_EQ(modern_number(Modern::SellSyn), 22u);
    EXPECT_EQ(modern_number(Modern::Money), 28u);
    EXPECT_EQ(modern_number(Modern::MonsterObtainNotify), 32u);
    EXPECT_EQ(modern_number(Modern::UpgradeFailedAck), 35u);
    EXPECT_EQ(modern_number(Modern::DissolveNack), 49u);
    EXPECT_EQ(modern_number(Modern::ShopItemChaseSyn), 154u);
    EXPECT_EQ(modern_number(Modern::ShopItemJobChangeSyn), 175u);
}

TEST(ItemProtocol, AllEnumeratorsDistinctFitUint8AndPickupAboveOriginalMax) {
    static_assert(std::is_same_v<std::underlying_type_t<Modern>, std::uint8_t>);
    static_assert(mxh::proto::kItemProtocolCount ==
                  mxh::proto::kItemProtocolEnumeratorCount);
    static_assert(mxh::proto::kItemProtocolCount == 60u);
    static_assert(kOriginalItemProtocolCount == 252u);

    ASSERT_EQ(mxh::proto::kItemProtocolCount, 60u);
    for (std::size_t i = 0; i < mxh::proto::kItemProtocolCount; ++i) {
        const unsigned value = modern_number(mxh::proto::kItemProtocolValues[i]);
        EXPECT_LE(value, 255u);
        for (std::size_t j = i + 1; j < mxh::proto::kItemProtocolCount; ++j) {
            EXPECT_NE(value, modern_number(mxh::proto::kItemProtocolValues[j]))
                << "index " << i << " and " << j;
        }
    }
    EXPECT_NE(static_cast<unsigned>(mxh::proto::kModernShopList),
              modern_number(Modern::PickupSyn));
    EXPECT_NE(static_cast<unsigned>(mxh::proto::kModernShopList),
              modern_number(Modern::PickupAck));
    EXPECT_NE(static_cast<unsigned>(mxh::proto::kModernShopList),
              modern_number(Modern::PickupNack));
    EXPECT_NE(static_cast<unsigned>(mxh::proto::kModernShopList),
              modern_number(Modern::GroundItemAdd));

    const unsigned original_max =
        original_number(Original::MP_ITEM_TITAN_APPEARANCE_CHANGE);
    const unsigned protocol_limit =
        original_number(Original::MP_ITEM_PROTOCOL_MAX);
    EXPECT_EQ(original_number(Original::MP_ITEM_TOTALINFO_LOCAL), 0u);
    EXPECT_EQ(original_max, 250u);
    EXPECT_EQ(protocol_limit, 256u);
    unsigned extension_values = 0;
    for (std::size_t i = 0; i < mxh::proto::kItemProtocolCount; ++i) {
        const unsigned value = modern_number(mxh::proto::kItemProtocolValues[i]);
        if (value <= original_max || value >= protocol_limit) continue;
        ++extension_values;
        EXPECT_LE(value, 255u);
        EXPECT_NE(value, static_cast<unsigned>(mxh::proto::kModernShopList));
        for (const Original assigned : kOriginalItemProtocols) {
            EXPECT_NE(value, original_number(assigned));
        }
    }
    EXPECT_EQ(extension_values, 4u);
}

TEST(ItemProtocol, GroundAddIsNotDirectObtain) {
    unsigned ground_add = 0;
    unsigned extension_count = 0;
    for (std::size_t i = 0; i < mxh::proto::kItemProtocolCount; ++i) {
        const unsigned value = modern_number(mxh::proto::kItemProtocolValues[i]);
        const bool pickup =
            value == modern_number(Modern::PickupSyn) ||
            value == modern_number(Modern::PickupAck) ||
            value == modern_number(Modern::PickupNack);
        if (value > 250u && value < 256u && !pickup) {
            ++extension_count;
            ground_add = value;
        }
    }
    EXPECT_EQ(extension_count, 1u);
    EXPECT_EQ(ground_add, 254u);
    EXPECT_NE(ground_add, modern_number(Modern::MonsterObtainNotify));
    EXPECT_EQ(modern_number(Modern::MonsterObtainNotify), 32u);
    EXPECT_EQ(modern_number(Modern::GroundItemAdd), ground_add);

    mxh::proto::GroundItemAddBody ground{};
    ground.object_id = 9001u;
    ground.source_monster_id = 77u;
    ground.item_id = 501u;
    ground.count = 1u;
    ground.pos_x = 10.0f;
    ground.pos_z = 20.0f;
    const auto ground_body = mxh::proto::encode_ground_item_add(ground);
    ASSERT_TRUE(ground_body);
    EXPECT_EQ(ground_body->size(), mxh::proto::kGroundItemAddBytes);
    EXPECT_FALSE(mxh::proto::original_item::decode_item_obtain_payload(*ground_body));
    const auto ground_round = mxh::proto::decode_ground_item_add(*ground_body);
    ASSERT_TRUE(ground_round);
    EXPECT_EQ(ground_round->object_id, ground.object_id);
    EXPECT_EQ(ground_round->item_id, ground.item_id);

    mxh::proto::original_item::ItemObtainArray obtained;
    obtained.obtain_count = 1u;
    obtained.items.push_back(mxh::game::ItemBase{});
    obtained.items.back().dwDBIdx = 42u;
    obtained.items.back().wIconIdx = 501u;
    obtained.items.back().Position = 3u;
    const auto obtain_body =
        mxh::proto::original_item::encode_item_obtain_payload(obtained);
    ASSERT_TRUE(obtain_body);
    EXPECT_FALSE(mxh::proto::decode_ground_item_add(*obtain_body));
    const auto obtain_round =
        mxh::proto::original_item::decode_item_obtain_payload(*obtain_body);
    ASSERT_TRUE(obtain_round);
    EXPECT_EQ(obtain_round->items.size(), 1u);
    EXPECT_EQ(obtain_round->items[0].Position, 3u);
}

TEST(ItemProtocol, DirectObtainEmitsProtocol32AndRejectsGroundDecode) {
    const auto built = mxh::proto::original_item::make_direct_inventory_obtain(
        7001u, 501u, 4u, 100u, 1u);
    ASSERT_TRUE(built);
    EXPECT_EQ(built->protocol, 32u);
    EXPECT_EQ(built->protocol, modern_number(Modern::MonsterObtainNotify));
    const auto obtained =
        mxh::proto::original_item::decode_item_obtain_payload(built->payload);
    ASSERT_TRUE(obtained);
    ASSERT_EQ(obtained->items.size(), 1u);
    EXPECT_EQ(obtained->items[0].dwDBIdx, 7001u);
    EXPECT_EQ(obtained->items[0].wIconIdx, 501u);
    EXPECT_EQ(obtained->items[0].Position, 4u);
    EXPECT_EQ(obtained->obtain_count, 1u);
    EXPECT_FALSE(mxh::proto::decode_ground_item_add(built->payload));
}
