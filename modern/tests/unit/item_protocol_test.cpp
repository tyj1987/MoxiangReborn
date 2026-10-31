// Item protocol numbers against the SHA-pinned original profile.
// Pickup must not occupy original Buy, and every modern Item value that
// has an original counterpart must use that counterpart's number.
// Ground-drop versus direct-to-inventory payload paths are a separate change.

#include "mxh/proto/original_item_profile.hpp"
#include "mxh/proto/protocol.hpp"

#include <gtest/gtest.h>

#include <cstdint>

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
