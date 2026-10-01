#include "unity_core/NpcDirectory.hpp"
#include "unity_core/ShopWire.hpp"
#include "mxh/unity/unity_client.h"
#include <gtest/gtest.h>
#include <limits>
#include <vector>

TEST(UnityNpcDirectory, DuplicateAddUpdatesRoleAndPositionAtCapacity) {
    mxh::unity::NpcDirectory directory(1);
    ASSERT_TRUE(directory.upsert(77, {1, 100, 200}));
    ASSERT_TRUE(directory.upsert(77, {27, 300, 400}));
    const auto npc = directory.find(77);
    ASSERT_TRUE(npc);
    EXPECT_EQ(npc->kind, 27);
    EXPECT_EQ(npc->x, 300);
    EXPECT_EQ(npc->z, 400);
    EXPECT_FALSE(directory.upsert(78, {6, 10, 20}));
    EXPECT_FALSE(directory.find(78));
    EXPECT_EQ(directory.find(77)->kind, 27);
}

TEST(UnityNpcDirectory, RemoveAndSessionResetInvalidateReusedObjectIdentity) {
    mxh::unity::NpcDirectory directory(1);
    EXPECT_FALSE(directory.upsert(0, {1, 0, 0}));
    ASSERT_TRUE(directory.upsert(77, {1, 100, 200}));
    directory.erase(77);
    EXPECT_FALSE(directory.find(77));
    ASSERT_TRUE(directory.upsert(78, {6, 500, 600}));
    directory.clear();
    EXPECT_FALSE(directory.find(78));
    ASSERT_TRUE(directory.upsert(78, {27, 700, 800}));
    EXPECT_EQ(directory.find(78)->kind, 27);
    EXPECT_EQ(directory.find(78)->x, 700);
}

TEST(UnityNpcDirectory, InteractionRangeUsesGameUnitsAndRejectsUnknownOrNonfinitePositions) {
    mxh::unity::NpcDirectory directory;
    ASSERT_TRUE(directory.upsert(77, {1, 1000, 1000}));
    EXPECT_TRUE(directory.in_range(77, 1300, 1400, 500));
    EXPECT_FALSE(directory.in_range(77, 1300, 1401, 500));
    EXPECT_FALSE(directory.in_range(78, 1000, 1000, 500));
    EXPECT_FALSE(directory.in_range(77, std::numeric_limits<float>::quiet_NaN(), 1000, 500));
    EXPECT_FALSE(directory.in_range(77, 1000, 1000, -1));
    directory.erase(77);
    EXPECT_FALSE(directory.in_range(77, 1000, 1000, 500));
}

TEST(UnityShopWire, ExactCatalogPreservesUnsignedPricesAndRejectsPartialOrTrailingData) {
    std::vector<std::uint8_t> bytes{77,0,0,0,1,0, 0x34,0x12,0xff,0xff,0xff,0xff};
    auto shop = mxh::unity::parse_shop_list(bytes);
    ASSERT_TRUE(shop);
    EXPECT_EQ(shop->npc_id, 77u);
    EXPECT_EQ(shop->count, 1);
    EXPECT_EQ(shop->entries[0], 0x34);
    EXPECT_EQ(shop->entries[5], 0xff);
    bytes.pop_back();
    EXPECT_FALSE(mxh::unity::parse_shop_list(bytes));
    bytes.push_back(0xff); bytes.push_back(0);
    EXPECT_FALSE(mxh::unity::parse_shop_list(bytes));
    bytes.assign(6, 0);
    ASSERT_TRUE(mxh::unity::parse_shop_list(bytes));
    bytes[4] = 0xff; bytes[5] = 0xff;
    EXPECT_FALSE(mxh::unity::parse_shop_list(bytes));
}

TEST(UnityNpcDirectory, RouteLookupPreservesLegacyNameAndRejectsAmbiguousDestinations) {
    mxh::unity::NpcDirectory directory;
    const std::string name = "\xB0\xA1\xB0\xA2";
    ASSERT_TRUE(directory.upsert(77, {1, 100, 200, name}));
    mxh::compat::MapChangeCatalog catalog;
    mxh::compat::MapChangeEntry route;
    route.current_map_num = 10;
    route.move_map_num = 12;
    route.object_name = name;
    catalog.entries.push_back(route);
    ASSERT_TRUE(directory.destination(77, 10, catalog));
    EXPECT_EQ(*directory.destination(77, 10, catalog), 12);
    EXPECT_FALSE(directory.destination(77, 11, catalog));
    EXPECT_FALSE(directory.destination(78, 10, catalog));
    route.move_map_num = 17;
    catalog.entries.push_back(route);
    EXPECT_FALSE(directory.destination(77, 10, catalog));
    EXPECT_FALSE(directory.upsert(77, {1, 100, 200, std::string(65, 'a')}));
    EXPECT_EQ(directory.find(77)->raw_name, name);
    directory.erase(77);
    EXPECT_FALSE(directory.destination(77, 10, catalog));
}

TEST(UnityMapRoutes, ExplicitCanonicalFileLoadsThroughCAbiAndRejectsInvalidBounds) {
    mxh_unity_handle handle = 0;
    ASSERT_EQ(mxh_unity_create(&handle), MXH_UNITY_OK);
    EXPECT_EQ(mxh_unity_load_map_routes(handle, nullptr, 1), MXH_UNITY_INVALID_ARGUMENT);
    EXPECT_EQ(mxh_unity_load_map_routes(handle, "x", 0), MXH_UNITY_INVALID_ARGUMENT);
    EXPECT_EQ(mxh_unity_load_map_routes(handle, "x", 4097), MXH_UNITY_INVALID_ARGUMENT);
    const char embedded[] = {'a', 0, 'b'};
    EXPECT_EQ(mxh_unity_load_map_routes(handle, embedded, 3), MXH_UNITY_INVALID_ARGUMENT);
    EXPECT_EQ(mxh_unity_load_map_routes(handle, "missing.bin", 11), MXH_UNITY_INVALID_ARGUMENT);
    const auto path = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()
        / "data" / "PlayDH" / "Resource" / "MapChange.bin";
    const auto bytes = path.u8string();
    const auto loaded = mxh_unity_load_map_routes(handle,
        reinterpret_cast<const char*>(bytes.data()), static_cast<std::uint32_t>(bytes.size()));
    EXPECT_EQ(loaded, MXH_UNITY_OK);
    EXPECT_EQ(mxh_unity_destroy(handle), MXH_UNITY_OK);
    EXPECT_EQ(mxh_unity_load_map_routes(handle, "x", 1), MXH_UNITY_INVALID_HANDLE);
}

TEST(UnityMapRoutes, MixedLocaleStaticPortalUsesUniqueCoordinateJoinAndOriginalIndex) {
    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data/PlayDH/Resource";
    const auto statics = mxh::compat::load_static_npc_bin(root / "StaticNpc.bin");
    const auto routes = mxh::compat::load_map_change_bin(root / "MapChange.bin");
    ASSERT_TRUE(statics); ASSERT_TRUE(routes);
    const auto portal = std::find_if(statics->begin(), statics->end(), [](const auto& entry) {
        return entry.map == 10 && entry.index == 1017;
    });
    ASSERT_NE(portal, statics->end());
    EXPECT_EQ(portal->x, 46973); EXPECT_EQ(portal->z, 4198);
    const auto* route = routes->find_destination(10, 2);
    ASSERT_NE(route, nullptr);
    EXPECT_NE(portal->name, route->object_name);
    mxh::unity::NpcDirectory directory;
    ASSERT_TRUE(directory.upsert(portal->index, {27, static_cast<std::uint16_t>(portal->x),
        static_cast<std::uint16_t>(portal->z), portal->name}));
    ASSERT_TRUE(directory.destination(1017, 10, *routes));
    EXPECT_EQ(*directory.destination(1017, 10, *routes), 2);
    auto ambiguous = *routes;
    ambiguous.entries.push_back(*route);
    EXPECT_FALSE(directory.destination(1017, 10, ambiguous));
}
