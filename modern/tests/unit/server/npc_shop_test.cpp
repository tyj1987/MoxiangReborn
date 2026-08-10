// npc_shop_test.cpp - Phase 13.4 NPC-shop data plane contract test.
//
// Covers every status path of npc_shop_buy_decision plus the
// BuySyn payload parser.  The orchestrator (MapHandler) wires the
// decision into its player-mu critical section; this test pins
// the data plane so that wiring is mechanical.

#include "mxh/server/npc_shop.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {

using mxh::server::NpcShopBuyRequest;
using mxh::server::NpcShopBuyStatus;
using mxh::server::NpcShopCatalog;
using mxh::server::NpcShopEntry;
using mxh::server::npc_shop_buy_decision;
using mxh::server::parse_npc_shop_buy_request;

mxh::server::NpcShopCatalog make_catalog() {
    mxh::server::NpcShopCatalog c;
    c.npc_id = 42;
    c.entries.push_back({101, 25, 0});   // unlimited stock
    c.entries.push_back({202, 100, 10});  // finite stock
    return c;
}

}

TEST(NpcShopBuyDecision, AcceptsAffordableUnlimitedStack) {
    auto cat = make_catalog();
    NpcShopBuyRequest req{cat.npc_id, 101, 3, 200};
    auto d = npc_shop_buy_decision(cat, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::Ok);
    EXPECT_EQ(d.total_price, 75u);
    EXPECT_EQ(d.new_money, 125u);
}

TEST(NpcShopBuyDecision, AcceptsAffordableFiniteStack) {
    auto cat = make_catalog();
    NpcShopBuyRequest req{cat.npc_id, 202, 5, 1000};
    auto d = npc_shop_buy_decision(cat, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::Ok);
    EXPECT_EQ(d.total_price, 500u);
    EXPECT_EQ(d.new_money, 500u);
}

TEST(NpcShopBuyDecision, RejectsInsufficientFunds) {
    auto cat = make_catalog();
    NpcShopBuyRequest req{cat.npc_id, 202, 5, 100};  // 500 > 100
    auto d = npc_shop_buy_decision(cat, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::InsufficientFunds);
    EXPECT_EQ(d.total_price, 0u);
    EXPECT_EQ(d.new_money, 0u);
}

TEST(NpcShopBuyDecision, RejectsUnknownItem) {
    auto cat = make_catalog();
    NpcShopBuyRequest req{cat.npc_id, 999, 1, 1000};
    auto d = npc_shop_buy_decision(cat, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::UnknownItem);
}

TEST(NpcShopBuyDecision, RejectsZeroItemId) {
    auto cat = make_catalog();
    NpcShopBuyRequest req{cat.npc_id, 0, 1, 1000};
    auto d = npc_shop_buy_decision(cat, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::InvalidItem);
}

TEST(NpcShopBuyDecision, RejectsZeroQty) {
    auto cat = make_catalog();
    NpcShopBuyRequest req{cat.npc_id, 101, 0, 1000};
    auto d = npc_shop_buy_decision(cat, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::InvalidQty);
}

TEST(NpcShopBuyDecision, RejectsOutOfStock) {
    auto cat = make_catalog();
    NpcShopBuyRequest req{cat.npc_id, 202, 11, 10000};  // stock=10, qty=11
    auto d = npc_shop_buy_decision(cat, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::OutOfStock);
}

TEST(NpcShopBuyDecision, AcceptsExactStock) {
    auto cat = make_catalog();
    NpcShopBuyRequest req{cat.npc_id, 202, 10, 10000};  // stock=10, qty=10
    auto d = npc_shop_buy_decision(cat, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::Ok);
    EXPECT_EQ(d.total_price, 1000u);
    EXPECT_EQ(d.new_money, 9000u);
}

TEST(NpcShopBuyDecision, RejectsNpcMismatch) {
    auto cat = make_catalog();
    NpcShopBuyRequest req{cat.npc_id + 1, 101, 1, 1000};
    auto d = npc_shop_buy_decision(cat, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::NpcMismatch);
}

TEST(NpcShopBuyDecision, DetectsPriceQtyOverflow) {
    // Entry price = max u32, qty = 2 -> 2 * max > max -> overflow
    // detected, decision is rejected (we use InvalidQty because the
    // request itself is invalid in the legacy sense).
    mxh::server::NpcShopCatalog c;
    c.npc_id = 1;
    c.entries.push_back({7, std::numeric_limits<std::uint32_t>::max(), 0});
    NpcShopBuyRequest req{c.npc_id, 7, 2, 1000};
    auto d = npc_shop_buy_decision(c, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::InvalidQty);
}

TEST(NpcShopBuyDecision, ZeroPriceFreeBuyIsAccepted) {
    // 1:1 quirk: zero-priced entries (e.g. starter gear on some
    // NPCs) are accepted; only qty and item guards reject.
    mxh::server::NpcShopCatalog c;
    c.npc_id = 5;
    c.entries.push_back({9, 0, 0});
    NpcShopBuyRequest req{c.npc_id, 9, 3, 0};
    auto d = npc_shop_buy_decision(c, req);
    EXPECT_EQ(d.status, NpcShopBuyStatus::Ok);
    EXPECT_EQ(d.total_price, 0u);
    EXPECT_EQ(d.new_money, 0u);
}

TEST(NpcShopCatalog, FindReturnsNulloptForMissingItem) {
    auto cat = make_catalog();
    EXPECT_TRUE(cat.find(101).has_value());
    EXPECT_TRUE(cat.find(202).has_value());
    EXPECT_FALSE(cat.find(404).has_value());
}

TEST(ParseNpcShopBuyRequest, ParsesFourBytePayload) {
    std::array<std::uint8_t, 4> buf{};
    // item_id = 101, qty = 3, LE.
    buf[0] = 0x65; buf[1] = 0x00;
    buf[2] = 0x03; buf[3] = 0x00;
    auto req = parse_npc_shop_buy_request(42, buf);
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->npc_id, 42u);
    EXPECT_EQ(req->item_id, 101u);
    EXPECT_EQ(req->qty, 3u);
    EXPECT_EQ(req->player_money, 0u);  // orchestrator fills this
}

TEST(ParseNpcShopBuyRequest, RejectsShortPayload) {
    std::array<std::uint8_t, 3> buf{};
    auto req = parse_npc_shop_buy_request(42, buf);
    EXPECT_FALSE(req.has_value());
}
