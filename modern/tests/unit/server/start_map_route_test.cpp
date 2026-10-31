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

TEST(StartAreaRoute, ClientSends17ConfiguredDefault10LandsOn10) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
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
    EXPECT_NE(log.find("client StartArea 17 differs from configured StartArea 10"),
              std::string::npos) << log;

    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT start_area, map_num FROM character_info", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 10);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 10);
    EXPECT_EQ(std::count_if(replies.begin(), replies.end(), [](const auto& msg) {
        return msg.header.protocol == static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterMakeNack);
    }), 0);
}

TEST(StartAreaRoute, ConfiguredStartArea17WithoutRouteNacksWithoutFallback) {
    QuietDb db;
    std::vector<mxh::net::Message> replies;
    mxh::server::AgentHandler handler(db, [&](mxh::net::ConnectionId,
                                              const mxh::net::Message& msg) {
        replies.push_back(msg);
    }, true, false, {}, /*default_map_num=*/10, /*configured_start_area=*/17);
    RecordingSender map10;
    handler.set_map_server(&map10, mxh::net::make_connection_id(18001));

    testing::internal::CaptureStderr();
    handler.on_message(mxh::net::make_connection_id(44), character_make(17));
    const auto log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("new-character StartArea map 17 is not registered"),
              std::string::npos) << log;
    EXPECT_FALSE(db.inserted_map.has_value());
    EXPECT_EQ(db.executes, 0);
    ASSERT_EQ(replies.size(), 1u);
    EXPECT_EQ(replies.front().header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeNack));
}

TEST(StartAreaRoute, ConfiguredStartArea17LandsOn17WhenMap17Registered) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    std::vector<mxh::net::Message> replies;
    mxh::server::AgentHandler handler(db, [&](mxh::net::ConnectionId,
                                              const mxh::net::Message& msg) {
        replies.push_back(msg);
    }, true, false, {}, /*default_map_num=*/10, /*configured_start_area=*/17);
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
    // Intentional: life falls on the same hit that still leaves shield.
    // CAttackManager::GetComboPhyDamage (AttackManager.cpp:120-140) and
    // GetMugongPhyDamage (AttackManager.cpp:253-274) subtract only
    // CObject::CalcShieldDamage's return (Object.cpp:520-534) and then
    // floor the defended remainder at 1. The old assertion required life
    // to stay unchanged while shield remained.
    bool life_dropped_while_shield_remains = false;
    bool life_dropped = false;
    for (int guard = 0; guard < 8; ++guard) {
        const auto before = map.monster_vitals_for_test(monster.object_id);
        ASSERT_TRUE(before);
        if (before->dead || before->life == 0u) break;
        const auto rolled = mxh::server::skill_caster_calculate_monster_damage(
            attacker, monster_stats, skill, rng, before->shield);
        ASSERT_GT(rolled.damage, 0) << "kind=" << visual->kind
                                    << " defense=" << visual->defense
                                    << " shield=" << visual->shield
                                    << " life=" << visual->life;
        (void)map.apply_monster_damage(1u, monster.object_id,
            static_cast<std::uint32_t>(rolled.damage), rolled.shield_absorbed,
            static_cast<std::uint32_t>(guard));
        const auto after = map.monster_vitals_for_test(monster.object_id);
        ASSERT_TRUE(after);
        EXPECT_LT(after->life, before->life);
        EXPECT_GE(before->life - after->life, 1u);
        life_dropped = after->life < before->life;
        if (before->shield > 0u && after->life < before->life)
            life_dropped_while_shield_remains = true;
        break;
    }
    EXPECT_TRUE(life_dropped) << "kind=" << visual->kind
                              << " defense=" << visual->defense
                              << " shield=" << visual->shield
                              << " life=" << visual->life
                              << " attack=" << attacker.phy_attack_min
                              << "-" << attacker.phy_attack_max;
    if (visual->shield > 0u)
        EXPECT_TRUE(life_dropped_while_shield_remains);
}

// TYJ-156 Map10 half. Kind 102 鬼吊 from MonsterList.bin column 18:
// level 59, life 6150, table shield 2294, defence 541. Per-hit life
// decrease only; this row is not a kill target. The sword matches the
// synthetic template in PlayerEquipmentStats (attack 29-35). Values only.
TEST(Map10FirstCombat, Level1StarterHitReducesLifeByOriginalSplit) {
    mxh::game::ItemInfo sword;
    sword.ItemIdx = 11000;
    sword.ItemKind = 2049;
    sword.EquipKind = mxh::game::WEARED_WEAPON;
    sword.WeaponType = 1;
    sword.MeleeAttackMin = 13;
    sword.MeleeAttackMax = 17;
    sword.LimitLevel = 1;
    const auto lookup = [&](std::uint16_t id, mxh::game::ItemInfo& out) {
        if (id != sword.ItemIdx) return false;
        out = sword;
        return true;
    };
    mxh::server::CalcBaseStats base;
    base.gengol = base.minchub = base.cheryuk = base.simmek = 12;
    auto state = mxh::server::make_player_state(1, 1, 1, base, {});
    state.equipment.items[mxh::game::WEARED_WEAPON] = mxh::game::make_item(
        101u, sword.ItemIdx, mxh::game::TP_WEAREDITEM_START + mxh::game::WEARED_WEAPON);
    mxh::game::PlayerCombatStats attacker;
    ASSERT_TRUE(mxh::server::rebuild_equipment_stats(state, attacker, lookup));
    EXPECT_EQ(attacker.phy_attack_min, 29u);
    EXPECT_EQ(attacker.phy_attack_max, 35u);
    attacker.level = 1;
    attacker.critical_rate = 0;
    attacker.dodge_rate = 0;

    // MonsterList.bin column 18, kind 102 鬼吊. Table shield is 2294;
    // the 2026-10-15 LifeNotify showed 2291 after hits had already landed.
    // This row is only a per-hit life check, never a kill target.
    constexpr std::uint32_t kMap10Life = 6150u;
    constexpr std::uint32_t kMap10Shield = 2294u;
    constexpr std::uint16_t kMap10Defense = 541u;
    mxh::game::PlayerCombatStats monster_stats;
    monster_stats.level = 59;
    monster_stats.phy_defence = kMap10Defense;
    monster_stats.dodge_rate = 0;
    mxh::game::SkillInfo skill{};

    QuietDb db;
    mxh::server::MapHandler map(db, 10, [](mxh::net::ConnectionId, const mxh::net::Message&) {});
    mxh::game::MonsterInstance monster;
    monster.object_id = 50143u;
    monster.monster_kind = 102u;
    monster.map_num = 10;
    monster.level = 59;
    monster.max_life = monster.current_life = kMap10Life;
    monster.max_shield = monster.current_shield = kMap10Shield;
    monster.defense = kMap10Defense;
    ASSERT_TRUE(map.add_monster_instance(monster));

    // Combo 0.5 (AttackManager.cpp:30, 120). Defence 541 at level 1 clamps
    // to 0.9 (AttackCalc.cpp:543-552). Table shield 2294 covers every request,
    // so reported ShieldDamage equals the amount removed (Object.cpp:531-533).
    // Life is the defended remainder, floored at 1 (AttackManager.cpp:139-140).
    const auto defence = mxh::game::compute_phy_defence_level(kMap10Defense, 1);
    EXPECT_DOUBLE_EQ(defence, 0.9);
    const struct {
        std::uint32_t attack;
        std::uint32_t shield;
        std::uint32_t life;
    } rows[] = {
        {29u, 14u, 1u}, {30u, 15u, 1u}, {31u, 15u, 1u}, {32u, 16u, 1u},
        {33u, 16u, 1u}, {34u, 17u, 1u}, {35u, 17u, 1u},
    };
    std::uint32_t life = kMap10Life;
    std::uint32_t shield = kMap10Shield;
    for (const auto& row : rows) {
        attacker.phy_attack_min = attacker.phy_attack_max = row.attack;
        std::mt19937 rng(1u);
        const auto rolled = mxh::server::skill_caster_calculate_monster_damage(
            attacker, monster_stats, skill, rng, shield);
        EXPECT_FALSE(rolled.is_miss);
        EXPECT_EQ(rolled.shield_damage, row.shield) << "attack=" << row.attack;
        EXPECT_EQ(rolled.shield_absorbed, row.shield) << "attack=" << row.attack;
        EXPECT_EQ(rolled.damage, static_cast<std::int32_t>(row.life))
            << "attack=" << row.attack;
        const auto split = mxh::server::original_monster_physical_split(
            static_cast<double>(row.attack), 0.5f, shield, defence);
        EXPECT_EQ(split.real_damage, row.life);
        EXPECT_EQ(split.shield_damage, row.shield);
        (void)map.apply_monster_damage(1u, monster.object_id,
            static_cast<std::uint32_t>(rolled.damage), rolled.shield_absorbed, 0u);
        const auto now = map.monster_vitals_for_test(monster.object_id);
        ASSERT_TRUE(now);
        EXPECT_EQ(now->life, life - row.life);
        EXPECT_EQ(now->shield, shield - row.shield);
        EXPECT_FALSE(now->dead);
        life = now->life;
        shield = now->shield;
    }
    EXPECT_EQ(life, kMap10Life - 7u);
    EXPECT_GT(shield, 0u);
}

// Isolated MonsterList.bin column 18 values (read-only confirmation).
// Swap this table if the spawn-map check names a different row.
// No original file bytes.
struct BeginnerColumn18 {
    const char* name;
    std::uint16_t kind;
    std::uint8_t level;
    std::uint32_t life;
    std::uint32_t shield;
    std::uint16_t defense;
};

TEST(BeginnerColumn18Kill, OriginalSplitKillsMap17AndLoginPointRows) {
    const BeginnerColumn18 rows[] = {
        // Map17 start area.
        {"小呆呆", 1u, 1u, 53u, 14u, 10u},
        // Nearest row to the classic Distribute login point (8303,9403).
        // The confirmation did not include this row's display name.
        {"", 6u, 6u, 188u, 92u, 25u},
    };
    mxh::game::ItemInfo sword;
    sword.ItemIdx = 11000;
    sword.ItemKind = 2049;
    sword.EquipKind = mxh::game::WEARED_WEAPON;
    sword.WeaponType = 1;
    sword.MeleeAttackMin = 13;
    sword.MeleeAttackMax = 17;
    sword.LimitLevel = 1;
    const auto lookup = [&](std::uint16_t id, mxh::game::ItemInfo& out) {
        if (id != sword.ItemIdx) return false;
        out = sword;
        return true;
    };
    mxh::server::CalcBaseStats base;
    base.gengol = base.minchub = base.cheryuk = base.simmek = 12;
    auto state = mxh::server::make_player_state(1, 1, 1, base, {});
    state.equipment.items[mxh::game::WEARED_WEAPON] = mxh::game::make_item(
        101u, sword.ItemIdx, mxh::game::TP_WEAREDITEM_START + mxh::game::WEARED_WEAPON);
    mxh::game::PlayerCombatStats attacker;
    ASSERT_TRUE(mxh::server::rebuild_equipment_stats(state, attacker, lookup));
    attacker.level = 1;
    attacker.critical_rate = 0;
    attacker.dodge_rate = 0;
    mxh::game::SkillInfo skill{};
    QuietDb db;
    mxh::server::MapHandler map(db, 17, [](mxh::net::ConnectionId, const mxh::net::Message&) {});
    std::uint32_t next_id = 170001u;
    for (const auto& row : rows) {
        mxh::game::MonsterInstance monster;
        monster.object_id = next_id++;
        monster.monster_kind = row.kind;
        monster.map_num = 17;
        monster.level = row.level;
        monster.max_life = monster.current_life = row.life;
        monster.max_shield = monster.current_shield = row.shield;
        monster.defense = row.defense;
        ASSERT_TRUE(map.add_monster_instance(monster)) << row.kind;
        mxh::game::PlayerCombatStats stats;
        stats.level = row.level;
        stats.phy_defence = row.defense;
        stats.dodge_rate = 0;
        bool died = false;
        for (int hit = 0; hit < 40 && !died; ++hit) {
            const auto before = map.monster_vitals_for_test(monster.object_id);
            ASSERT_TRUE(before);
            std::mt19937 rng(static_cast<std::uint32_t>(hit + 1));
            const auto rolled = mxh::server::skill_caster_calculate_monster_damage(
                attacker, stats, skill, rng, before->shield);
            ASSERT_GE(rolled.damage, 1) << row.name << " kind=" << row.kind;
            (void)map.apply_monster_damage(1u, monster.object_id,
                static_cast<std::uint32_t>(rolled.damage), rolled.shield_absorbed,
                static_cast<std::uint32_t>(hit));
            const auto after = map.monster_vitals_for_test(monster.object_id);
            ASSERT_TRUE(after);
            EXPECT_LT(after->life, before->life);
            died = after->dead || after->life == 0u;
        }
        EXPECT_TRUE(died) << "kind=" << row.kind << " name=" << row.name;
    }
}
