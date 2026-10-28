#include "mxh/db/schema_migration.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/game/item_list_parser.hpp"
#include "mxh/server/ai_group_loader.hpp"
#include "mxh/server/player_equipment_stats.hpp"
#include "mxh/server/server.hpp"
#include "mxh/server/skill_caster.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

class RecordingSender final : public mxh::net::ITcpSender {
public:
    [[nodiscard]] mxh::net::NetError send(const mxh::net::Message& msg) override {
        sent.push_back(msg);
        return mxh::net::NetError::Ok;
    }
    [[nodiscard]] bool is_connected() const noexcept override { return true; }
    std::vector<mxh::net::Message> sent;
};

class QuietDb final : public mxh::db::IDbAdapter {
public:
    mxh::db::DbResult connect(const mxh::db::ConnectionConfig&) override { return {}; }
    void disconnect() override {}
    bool is_connected() const noexcept override { return true; }
    mxh::db::DbResult query(std::string_view, std::span<const mxh::db::Bind>,
                            mxh::db::ResultSet& out) override {
        out = {};
        return {};
    }
    mxh::db::DbResult execute(std::string_view sql,
                              std::span<const mxh::db::Bind> params) override {
        ++executes;
        if (sql.find("INSERT INTO character_info") != std::string_view::npos &&
            params.size() > 10 &&
            std::holds_alternative<std::int64_t>(params[10].value)) {
            inserted_map = std::get<std::int64_t>(params[10].value);
        }
        return {mxh::db::DbError::Ok, {}, 1};
    }
    mxh::db::DbResult begin_transaction() override { return {}; }
    mxh::db::DbResult commit() override { return {}; }
    mxh::db::DbResult rollback() override { return {}; }
    std::string backend_name() const noexcept override { return "quiet"; }
    int executes = 0;
    std::optional<std::int64_t> inserted_map;
};

mxh::net::Message character_make(std::uint8_t start_area) {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeSyn);
    msg.payload.assign(59, 0);
    const char name[] = "BirthHero";
    std::memcpy(msg.payload.data(), name, sizeof(name) - 1);
    const std::uint32_t user_id = 42;
    std::memcpy(msg.payload.data() + 17, &user_id, sizeof(user_id));
    msg.payload[25] = start_area;
    const float scale = 1.0f;
    std::memcpy(msg.payload.data() + 51, &scale, sizeof(scale));
    std::memcpy(msg.payload.data() + 55, &scale, sizeof(scale));
    return msg;
}

std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST(StartAreaRoute, CreationRejectsUnregisteredStartMapWithoutMap10Fallback) {
    QuietDb db;
    std::vector<mxh::net::Message> replies;
    mxh::server::AgentHandler handler(db, [&](mxh::net::ConnectionId,
                                              const mxh::net::Message& msg) {
        replies.push_back(msg);
    }, true, false, {}, /*default_map_num=*/10);
    RecordingSender map10;
    handler.set_map_server(&map10, mxh::net::make_connection_id(18001));

    testing::internal::CaptureStderr();
    handler.on_message(mxh::net::make_connection_id(42), character_make(17));
    const auto log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("new-character StartArea map 17 is not registered"),
              std::string::npos) << log;
    EXPECT_FALSE(db.inserted_map.has_value());
    EXPECT_EQ(db.executes, 0);
    ASSERT_EQ(replies.size(), 1u);
    EXPECT_EQ(replies.front().header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeNack));
}

TEST(StartAreaRoute, RegisteredMap17PersistsStartAreaAndMap) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    std::vector<mxh::net::Message> replies;
    mxh::server::AgentHandler handler(db, [&](mxh::net::ConnectionId,
                                              const mxh::net::Message& msg) {
        replies.push_back(msg);
    }, true, false, {}, 10);
    RecordingSender map17;
    handler.set_map_server_for_map(17, &map17, mxh::net::make_connection_id(18017));
    handler.on_message(mxh::net::make_connection_id(43), character_make(17));

    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT start_area, map_num FROM character_info", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 17);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 17);
    EXPECT_EQ(std::count_if(replies.begin(), replies.end(), [](const auto& msg) {
        return msg.header.protocol == static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterMakeNack);
    }), 0);
}

TEST(StartAreaRoute, SavedMap10CharacterStillEntersMap10) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.execute(
        "INSERT INTO character_info(chrid,charname,userid,level,map_num,start_area) "
        "VALUES(100010,'MapTen','42',1,10,17)", {}).ok());
    std::vector<mxh::net::Message> replies;
    mxh::server::AgentHandler handler(db, [&](mxh::net::ConnectionId,
                                              const mxh::net::Message& msg) {
        replies.push_back(msg);
    }, true, false, {}, /*default_map_num=*/17);
    RecordingSender map17;
    RecordingSender map10;
    handler.set_map_server(&map17, mxh::net::make_connection_id(18017));
    handler.set_map_server_for_map(10, &map10, mxh::net::make_connection_id(18010));
    const auto connection = mxh::net::make_connection_id(77);
    handler.register_session(connection, 42u, 0u, 0u);

    mxh::net::Message select;
    select.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    select.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterSelectSyn);
    select.header.object_id = 100010u;
    select.payload.assign(2, 0);
    handler.on_message(connection, select);
    ASSERT_FALSE(replies.empty());
    EXPECT_EQ(replies.back().header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterSelectAck));
    ASSERT_FALSE(replies.back().payload.empty());
    EXPECT_EQ(replies.back().payload[0], 10u);

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 100010u;
    game_in.payload.assign(8, 0);
    handler.on_message(connection, game_in);
    EXPECT_EQ(map10.sent.size(), 1u);
    EXPECT_TRUE(map17.sent.empty());
    if (!map10.sent.empty()) {
        EXPECT_EQ(map10.sent.front().header.protocol, game_in.header.protocol);
        EXPECT_EQ(map10.sent.front().header.object_id, 100010u);
    }
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT map_num FROM character_info WHERE chrid=100010", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 10);
}

TEST(StartAreaRoute, Level1StarterGearDamagesMap17MonsterAfterShield) {
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data" / "PlayDH";
    const auto ai_path = root / "Resource" / "Server" / "Monster_17.bin";
    const auto list_path = root / "Resource" / "MonsterList.bin";
    const auto item_path = root / "Resource" / "ItemList.bin";
    const auto groups = mxh::server::load_ai_group_list_bin(ai_path, "playdh-current");
    ASSERT_TRUE(groups.has_value()) << ai_path.string();
    const auto catalog = mxh::compat::MonsterCatalog::parse_bin(read_file(list_path));
    ASSERT_TRUE(catalog.has_value()) << list_path.string();

    // Login position recorded for LoginPoint 2017 / lanzhou / map 17 in
    // docs/evidence/original-birth-route-static-20261015.json. The closest
    // live Map17 AI spawn is the starting monster for this check.
    const mxh::server::AiSpawnDefinition* nearest = nullptr;
    double best = std::numeric_limits<double>::infinity();
    for (const auto& group : groups->groups) {
        for (const auto& spawn : group.spawns) {
            // The last #ADD field is ISREGEN, not a dead-on-arrival flag.
            // MapHandler spawns those slots alive.
            const double dx = static_cast<double>(spawn.pos_x) - 18000.0;
            const double dz = static_cast<double>(spawn.pos_z) - 37200.0;
            const double distance = dx * dx + dz * dz;
            if (distance < best) {
                best = distance;
                nearest = &spawn;
            }
        }
    }
    ASSERT_NE(nearest, nullptr);
    const auto* visual = catalog->find(static_cast<std::uint16_t>(nearest->monster_kind));
    ASSERT_NE(visual, nullptr) << "kind=" << nearest->monster_kind;
    ASSERT_GT(visual->life, 0u);

    const auto parsed = mxh::game::load_item_list(item_path.string());
    std::map<std::uint16_t, mxh::game::ItemInfo> entries;
    for (const auto id : {11000u, 23000u, 27000u}) {
        const auto item = std::find_if(parsed.items.begin(), parsed.items.end(),
            [id](const mxh::game::ItemInfo& row) { return row.ItemIdx == id; });
        ASSERT_NE(item, parsed.items.end()) << id;
        entries.emplace(static_cast<std::uint16_t>(id), *item);
    }
    const auto lookup = [&](std::uint16_t id, mxh::game::ItemInfo& out) {
        const auto it = entries.find(id);
        if (it == entries.end()) return false;
        out = it->second;
        return true;
    };
    mxh::server::CalcBaseStats base;
    base.gengol = base.minchub = base.cheryuk = base.simmek = 12;
    auto state = mxh::server::make_player_state(1, 1, 1, base, {});
    const auto wear = [&](std::uint8_t slot, std::uint16_t icon) {
        state.equipment.items[slot] = mxh::game::make_item(
            100u + slot, icon, mxh::game::TP_WEAREDITEM_START + slot);
    };
    wear(mxh::game::WEARED_WEAPON, 11000);
    wear(mxh::game::WEARED_DRESS, 23000);
    wear(mxh::game::WEARED_SHOES, 27000);
    mxh::game::PlayerCombatStats attacker;
    ASSERT_TRUE(mxh::server::rebuild_equipment_stats(state, attacker, lookup))
        << "kind=" << visual->kind << " defense=" << visual->defense;
    attacker.level = 1;
    attacker.critical_rate = 0;
    attacker.dodge_rate = 0;
    mxh::game::PlayerCombatStats monster_stats;
    monster_stats.level = visual->level == 0 ? 1 : visual->level;
    monster_stats.phy_defence = visual->defense;
    monster_stats.dodge_rate = 0;

    QuietDb db;
    mxh::server::MapHandler map(db, 17, [](mxh::net::ConnectionId, const mxh::net::Message&) {});
    mxh::game::MonsterInstance monster;
    monster.object_id = 170017u;
    monster.monster_kind = visual->kind;
    monster.map_num = 17;
    monster.level = static_cast<std::uint8_t>(monster_stats.level);
    monster.max_life = monster.current_life = visual->life;
    monster.max_shield = monster.current_shield = visual->shield;
    monster.defense = visual->defense;
    ASSERT_TRUE(map.add_monster_instance(monster));

    std::mt19937 rng(156u);
    mxh::game::SkillInfo skill{};
    bool life_dropped_after_shield = false;
    for (int guard = 0; guard < 100000; ++guard) {
        const auto before = map.monster_vitals_for_test(monster.object_id);
        ASSERT_TRUE(before);
        if (before->dead || before->life == 0u) break;
        const auto rolled = mxh::server::skill_caster_calculate_monster_damage(
            attacker, monster_stats, skill, rng);
        ASSERT_GT(rolled.damage, 0) << "kind=" << visual->kind
                                    << " defense=" << visual->defense
                                    << " shield=" << visual->shield
                                    << " life=" << visual->life;
        (void)map.apply_monster_damage(1u, monster.object_id,
            static_cast<std::uint32_t>(rolled.damage), static_cast<std::uint32_t>(guard));
        const auto after = map.monster_vitals_for_test(monster.object_id);
        ASSERT_TRUE(after);
        if (before->shield == 0u) {
            EXPECT_LT(after->life, before->life);
            life_dropped_after_shield = after->life < before->life;
            break;
        }
        if (after->shield > 0u) {
            EXPECT_EQ(after->life, before->life);
            continue;
        }
        if (after->life < before->life) {
            life_dropped_after_shield = true;
            break;
        }
        const auto next = mxh::server::skill_caster_calculate_monster_damage(
            attacker, monster_stats, skill, rng);
        ASSERT_GT(next.damage, 0);
        (void)map.apply_monster_damage(1u, monster.object_id,
            static_cast<std::uint32_t>(next.damage), static_cast<std::uint32_t>(guard));
        const auto later = map.monster_vitals_for_test(monster.object_id);
        ASSERT_TRUE(later);
        EXPECT_LT(later->life, after->life);
        life_dropped_after_shield = later->life < after->life;
        break;
    }
    EXPECT_TRUE(life_dropped_after_shield) << "kind=" << visual->kind
                                           << " defense=" << visual->defense
                                           << " shield=" << visual->shield
                                           << " life=" << visual->life
                                           << " attack=" << attacker.phy_attack_min
                                           << "-" << attacker.phy_attack_max;
}
