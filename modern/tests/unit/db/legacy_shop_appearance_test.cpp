#include "mxh/db/legacy_shop_appearance.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/db/modern_shop_state.hpp"
#include "mxh/server/revive_shop_plan.hpp"
#include "mxh/db/revive_state.hpp"
#include <gtest/gtest.h>
#include <cstdlib>
#include <stdexcept>

namespace {
// Real SQLite durability with a lost adapter acknowledgement after COMMIT.
class LostCommitReply final : public mxh::db::IDbAdapter {
public:
    explicit LostCommitReply(mxh::db::IDbAdapter& inner, bool throws)
        : m_inner(inner), m_throws(throws) {}
    mxh::db::DbResult connect(const mxh::db::ConnectionConfig& cfg) override { return m_inner.connect(cfg); }
    void disconnect() override { m_inner.disconnect(); }
    bool is_connected() const noexcept override { return m_inner.is_connected(); }
    mxh::db::DbResult execute(std::string_view sql, std::span<const mxh::db::Bind> args) override {
        return m_inner.execute(sql,args);
    }
    mxh::db::DbResult query(std::string_view sql, std::span<const mxh::db::Bind> args,
        mxh::db::ResultSet& out) override { return m_inner.query(sql,args,out); }
    mxh::db::DbResult begin_transaction() override { return m_inner.begin_transaction(); }
    mxh::db::DbResult commit() override {
        auto result=m_inner.commit();
        if (!result.ok()) return result;
        if (m_throws) throw std::runtime_error("injected lost commit reply");
        return {mxh::db::DbError::IoError,"injected lost commit reply"};
    }
    mxh::db::DbResult rollback() override { return m_inner.rollback(); }
    std::string backend_name() const noexcept override { return m_inner.backend_name(); }
private:
    mxh::db::IDbAdapter& m_inner;
    bool m_throws;
};
class LegacyShopAppearance : public ::testing::Test {
protected:
    mxh::db::SqliteAdapter adapter;
    mxh::db::IDbAdapter& db = adapter;
    void SetUp() override {
        mxh::db::ConnectionConfig cfg; cfg.path = ":memory:";
        ASSERT_TRUE(db.connect(cfg).ok());
    }
    void schema() {
        ASSERT_TRUE(db.execute("CREATE TABLE TB_SHOPITEMUSEINFO(CHARACTER_IDX INT,ITEM_IDX INT,ITEM_DBIDX INT,ITEM_PARAM INT,BEGIN_TIME INT,REMAIN_TIME INT)").ok());
        ASSERT_TRUE(db.execute("CREATE TABLE TB_ITEM(ITEM_DBIDX INT,item_position INT)").ok());
        ASSERT_TRUE(db.execute("CREATE TABLE TB_SKININFO(CharacterIdx INT,Hat INT,Mask INT,Dress INT,Shoulder INT,Shoes INT)").ok());
    }
};

TEST_F(LegacyShopAppearance, MissingSourceIsNotAnEmptyCharacter) {
    auto result = mxh::db::load_legacy_shop_appearance(db, 42);
    EXPECT_EQ(result.status, mxh::db::LegacyShopLoadStatus::QueryFailed);
    EXPECT_FALSE(result.rows);
    schema();
    result = mxh::db::load_legacy_shop_appearance(db, 42);
    ASSERT_EQ(result.status, mxh::db::LegacyShopLoadStatus::Loaded);
    ASSERT_TRUE(result.rows); EXPECT_TRUE(result.rows->used_items.empty());
    EXPECT_EQ(result.rows->skin, (std::array<std::uint16_t,5>{}));
}

TEST_F(LegacyShopAppearance, PetBlobRoundTripSurvivesShopSaveAndRejectsMalformedInput) {
    ASSERT_TRUE(db.execute("CREATE TABLE character_info(chrid INT PRIMARY KEY,userid TEXT,character_data BLOB)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO character_info VALUES(42,'7',NULL)").ok());
    const auto initial=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(initial);
    std::vector<mxh::db::PersistedPet> pets{{901,1,2,1234,567,1,0,1},{902,4,1,55,0,0,1,0}};
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db,42,7,*initial,{},mxh::db::PersistedVitals{102,30,0},pets));
    auto loaded=mxh::db::load_modern_shop_state(db,42,7);
    ASSERT_TRUE(loaded); ASSERT_TRUE(loaded->pets); ASSERT_EQ(loaded->pets->size(),2u);
    EXPECT_EQ((*loaded->pets)[0].summon_item,901u);
    EXPECT_EQ((*loaded->pets)[0].grade,2u);
    EXPECT_EQ((*loaded->pets)[0].stamina,1234u);
    EXPECT_EQ((*loaded->pets)[0].friendship,567u);
    EXPECT_EQ((*loaded->pets)[0].summoned,1u);
    EXPECT_EQ((*loaded->pets)[1].rest,1u);
    mxh::db::LegacyShopAppearanceRows shop; shop.skin[0]=101;
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db,42,7,*loaded,shop));
    loaded=mxh::db::load_modern_shop_state(db,42,7);
    ASSERT_TRUE(loaded); ASSERT_TRUE(loaded->pets); EXPECT_EQ(loaded->pets->size(),2u);
    EXPECT_EQ((*loaded->pets)[0].friendship,567u);
    const auto blob=std::get<std::vector<std::uint8_t>>(loaded->original);
    for(std::size_t length=0;length<blob.size();++length) {
        const std::vector<std::uint8_t> truncated(blob.begin(),blob.begin()+length);
        ASSERT_TRUE(db.execute("UPDATE character_info SET character_data=? WHERE chrid=42",{mxh::db::bind(truncated)}).ok());
        EXPECT_FALSE(mxh::db::load_modern_shop_state(db,42,7)) << length;
    }
    pets[1].summon_item=901;
    EXPECT_FALSE(mxh::db::encode_modern_shop_state(42,{},mxh::db::PersistedVitals{},pets));
    pets[1].summon_item=902; pets[1].alive=2;
    EXPECT_FALSE(mxh::db::encode_modern_shop_state(42,{},mxh::db::PersistedVitals{},pets));
}

TEST_F(LegacyShopAppearance, ModernOwnedBlobRoundTripsWithoutSchemaExtension) {
    ASSERT_TRUE(db.execute("CREATE TABLE character_info(chrid INT PRIMARY KEY,userid TEXT,character_data BLOB)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO character_info VALUES(42,'7',NULL),(43,'8',NULL)").ok());
    const auto initial=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(initial);
    EXPECT_TRUE(initial->rows.used_items.empty());
    EXPECT_FALSE(mxh::db::load_modern_shop_state(db,42,8));
    EXPECT_FALSE(mxh::db::load_modern_shop_state(db,99,7));
    mxh::db::LegacyShopAppearanceRows rows;
    rows.skin={101,102,103,104,105};
    rows.used_items.push_back({55134,390,901,10,0xffffffffu,123});
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db,42,7,*initial,rows));
    const auto loaded=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(loaded);
    ASSERT_EQ(loaded->rows.used_items.size(),1u); EXPECT_EQ(loaded->rows.skin,rows.skin);
    EXPECT_EQ(loaded->rows.used_items[0].begin_time,0xffffffffu);
    EXPECT_EQ(loaded->rows.used_items[0].parameter,10u);
    EXPECT_EQ(loaded->rows.used_items[0].position,390u);
    EXPECT_TRUE(mxh::db::load_modern_shop_state(db,43,8)->rows.used_items.empty());
}

TEST_F(LegacyShopAppearance, ReviveCommitRollsBackLateBlobFailureAndRejectsReplay) {
    ASSERT_TRUE(db.execute("CREATE TABLE character_info(chrid INT PRIMARY KEY,userid TEXT,level INT,character_data BLOB)").ok());
    ASSERT_TRUE(db.execute("CREATE TABLE modern_player_state(player_id INT PRIMARY KEY,level INT,exp INT,money INT)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO character_info VALUES(42,'7',48,NULL)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO modern_player_state VALUES(42,48,1000,100)").ok());
    const auto empty=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(empty);
    const std::vector<mxh::db::PersistedPet> original_pets{{901,1,2,1234,57,1,0,1}};
    const std::vector<mxh::db::PersistedPet> revived_pets{{901,1,2,1234,0,0,0,1}};
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db,42,7,*empty,{},mxh::db::PersistedVitals{},original_pets));
    const auto initial=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(initial);
    ASSERT_TRUE(db.execute("CREATE TRIGGER fail_revive BEFORE UPDATE OF character_data ON character_info "
        "BEGIN SELECT RAISE(ABORT,'injected'); END").ok());
    const mxh::db::ReviveProgress before{48,100,1000},after{47,96,980};
    EXPECT_EQ(mxh::db::commit_revive_state(db,42,7,*initial,before,after,{}, {102,30,0},revived_pets),mxh::db::ReviveCommit::Rejected);
    const auto rolled_back=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(rolled_back);
    EXPECT_EQ(rolled_back->original,initial->original);
    mxh::db::ResultSet result;
    ASSERT_TRUE(db.query("SELECT S.level,S.exp,S.money,C.level FROM modern_player_state S JOIN character_info C ON C.chrid=S.player_id",result).ok());
    ASSERT_EQ(result.rows.size(),1u);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]),48);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]),1000);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][2]),100);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][3]),48);
    ASSERT_TRUE(db.execute("DROP TRIGGER fail_revive").ok());
    EXPECT_EQ(mxh::db::commit_revive_state(db,42,7,*initial,before,after,{}, {102,30,0},revived_pets),mxh::db::ReviveCommit::Committed);
    const auto saved=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(saved); ASSERT_TRUE(saved->vitals);
    EXPECT_EQ(saved->vitals->life,102u);
    ASSERT_TRUE(saved->pets); ASSERT_EQ(saved->pets->size(),1u);
    EXPECT_EQ((*saved->pets)[0].friendship,0u);
    EXPECT_EQ((*saved->pets)[0].alive,0u);
    EXPECT_EQ((*saved->pets)[0].stamina,1234u);
    EXPECT_EQ(mxh::db::commit_revive_state(db,42,7,*initial,before,after,{}, {102,30,0}),mxh::db::ReviveCommit::Rejected);
}

TEST_F(LegacyShopAppearance, LostReviveCommitReplyIsUncertainEvenWhenDurable) {
    ASSERT_TRUE(db.execute("CREATE TABLE character_info(chrid INT PRIMARY KEY,userid TEXT,level INT,character_data BLOB)").ok());
    ASSERT_TRUE(db.execute("CREATE TABLE modern_player_state(player_id INT PRIMARY KEY,level INT,exp INT,money INT)").ok());
    for (bool throws : {false,true}) {
        const int player=throws ? 43 : 42;
        ASSERT_TRUE(db.execute("INSERT INTO character_info VALUES(?,'7',48,NULL)",{mxh::db::bind(player)}).ok());
        ASSERT_TRUE(db.execute("INSERT INTO modern_player_state VALUES(?,48,1000,100)",{mxh::db::bind(player)}).ok());
        const auto initial=mxh::db::load_modern_shop_state(db,player,7); ASSERT_TRUE(initial);
        const mxh::db::ReviveProgress before{48,100,1000},after{47,96,980};
        LostCommitReply fault(db,throws);
        EXPECT_EQ(mxh::db::commit_revive_state(fault,player,7,*initial,before,after,{}, {102,30,0}),
            mxh::db::ReviveCommit::Uncertain);
        const auto saved=mxh::db::load_modern_shop_state(db,player,7);
        ASSERT_TRUE(saved); ASSERT_TRUE(saved->vitals); EXPECT_EQ(saved->vitals->life,102u);
        mxh::db::ResultSet result;
        ASSERT_TRUE(db.query("SELECT level,exp,money FROM modern_player_state WHERE player_id=?",
            {mxh::db::bind(player)},result).ok());
        ASSERT_EQ(result.rows.size(),1u);
        EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]),47);
        EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]),980);
        EXPECT_EQ(std::get<std::int64_t>(result.rows[0][2]),96);
        EXPECT_EQ(mxh::db::commit_revive_state(db,player,7,*initial,before,after,{}, {102,30,0}),
            mxh::db::ReviveCommit::Rejected);
    }
}

TEST_F(LegacyShopAppearance, ReviveShopCandidateKeepsUnrelatedRecordsAndDoesNotMutateInput) {
    mxh::db::LegacyShopAppearanceRows rows;
    rows.skin[0]=101;
    rows.used_items={{55000,390,1,2,10,20},{55311,391,2,0,11,21},{55312,392,3,0,12,22}};
    const auto combined=mxh::server::prepare_revive_shop(rows,{60,240},2,55000);
    ASSERT_TRUE(combined);
    ASSERT_EQ(combined->rows.used_items.size(),3u);
    EXPECT_EQ(combined->rows.used_items[0].parameter,1u);
    EXPECT_EQ(rows.used_items[0].parameter,2u);
    EXPECT_EQ(combined->rows.skin,rows.skin);
    ASSERT_EQ(combined->notices.size(),1u);
    EXPECT_EQ(static_cast<unsigned>(combined->notices[0].protocol),150u);
    EXPECT_EQ(combined->notices[0].value,1u);
    EXPECT_FALSE(combined->reduce_pet_friendship);
    const auto last=mxh::server::prepare_revive_shop(combined->rows,{60,240},1,55000);
    ASSERT_TRUE(last); ASSERT_EQ(last->rows.used_items.size(),2u);
    ASSERT_EQ(last->notices.size(),2u);
    EXPECT_EQ(static_cast<unsigned>(last->notices[0].protocol),106u);
    EXPECT_EQ(last->notices[0].value,55000u);
    EXPECT_EQ(static_cast<unsigned>(last->notices[1].protocol),150u);
    EXPECT_EQ(last->notices[1].value,0u);
    EXPECT_EQ(last->rows.used_items[0].item_id,55311);
    const auto individual=mxh::server::prepare_revive_shop(rows,{60,240},2,55999);
    ASSERT_TRUE(individual); ASSERT_EQ(individual->rows.used_items.size(),1u);
    EXPECT_EQ(individual->rows.used_items[0].item_id,55000);
    EXPECT_EQ(individual->protection.loss.money,0u);
    ASSERT_EQ(individual->notices.size(),2u);
    EXPECT_EQ(static_cast<unsigned>(individual->notices[0].protocol),109u);
    EXPECT_EQ(individual->notices[0].value,55311u);
    EXPECT_EQ(static_cast<unsigned>(individual->notices[1].protocol),110u);
    EXPECT_EQ(individual->notices[1].value,55312u);
    EXPECT_FALSE(individual->reduce_pet_friendship);
    const auto unprotected=mxh::server::prepare_revive_shop({}, {0,0},0,0);
    ASSERT_TRUE(unprotected);
    EXPECT_TRUE(unprotected->notices.empty());
    // The source invokes the pet hook even when rounding makes exp loss zero.
    EXPECT_TRUE(unprotected->reduce_pet_friendship);
    EXPECT_EQ(individual->protection.loss.experience,0u);
    rows.used_items.push_back(rows.used_items.front());
    EXPECT_FALSE(mxh::server::prepare_revive_shop(rows,{60,240},2,55000));
}

TEST_F(LegacyShopAppearance, VitalsExtensionPreservesShopEditsAndRejectsStaleUpdate) {
    ASSERT_TRUE(db.execute("CREATE TABLE character_info(chrid INT PRIMARY KEY,userid TEXT,character_data BLOB)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO character_info VALUES(42,'7',NULL)").ok());
    const auto initial=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(initial);
    mxh::db::LegacyShopAppearanceRows rows; rows.skin[0]=101;
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db,42,7,*initial,rows,mxh::db::PersistedVitals{102,30,0}));
    const auto loaded=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(loaded);
    ASSERT_TRUE(loaded->vitals);
    EXPECT_EQ(loaded->vitals->life,102u); EXPECT_EQ(loaded->vitals->shield,30u);
    EXPECT_EQ(loaded->vitals->naeryuk,0u); EXPECT_EQ(loaded->rows.skin[0],101u);
    rows.skin[0]=202;
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db,42,7,*loaded,rows));
    const auto edited=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(edited);
    ASSERT_TRUE(edited->vitals); EXPECT_EQ(edited->vitals->life,102u);
    EXPECT_EQ(edited->rows.skin[0],202u);
    EXPECT_FALSE(mxh::db::save_modern_shop_state(db,42,7,*loaded,rows,mxh::db::PersistedVitals{0,0,0}));
    EXPECT_EQ(mxh::db::load_modern_shop_state(db,42,7)->vitals->life,102u);
}

TEST_F(LegacyShopAppearance, ModernBlobRejectsStaleSaveAndUnknownOrCrossCharacterData) {
    ASSERT_TRUE(db.execute("CREATE TABLE character_info(chrid INT PRIMARY KEY,userid TEXT,character_data BLOB)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO character_info VALUES(42,'7',NULL),(43,'7',NULL)").ok());
    const auto initial=mxh::db::load_modern_shop_state(db,42,7); ASSERT_TRUE(initial);
    mxh::db::LegacyShopAppearanceRows rows; rows.skin[0]=101;
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db,42,7,*initial,rows));
    rows.skin[0]=202;
    EXPECT_FALSE(mxh::db::save_modern_shop_state(db,42,7,*initial,rows));
    ASSERT_TRUE(db.execute("UPDATE character_info SET character_data=(SELECT character_data FROM character_info WHERE chrid=42) WHERE chrid=43").ok());
    EXPECT_FALSE(mxh::db::load_modern_shop_state(db,43,7));
    ASSERT_TRUE(db.execute("UPDATE character_info SET character_data=X'010203' WHERE chrid=42").ok());
    EXPECT_FALSE(mxh::db::load_modern_shop_state(db,42,7));
    EXPECT_FALSE(mxh::db::save_modern_shop_state(db,42,7,*initial,rows));
    mxh::db::ResultSet result;
    ASSERT_TRUE(db.query("SELECT character_data FROM character_info WHERE chrid=42",result).ok());
    EXPECT_EQ(std::get<std::vector<std::uint8_t>>(result.rows[0][0]),(std::vector<std::uint8_t>{1,2,3}));
}

TEST_F(LegacyShopAppearance, ModernBlobRejectsDuplicateIconsAndBadVersion) {
    mxh::db::LegacyShopAppearanceRows rows;
    rows.used_items.push_back({55134,390,901,10,0,123});
    rows.used_items.push_back(rows.used_items[0]);
    EXPECT_FALSE(mxh::db::encode_modern_shop_state(42,rows));
    rows.used_items.pop_back();
    auto encoded=mxh::db::encode_modern_shop_state(42,rows); ASSERT_TRUE(encoded);
    (*encoded)[4]=2;
    ASSERT_TRUE(db.execute("CREATE TABLE character_info(chrid INT PRIMARY KEY,userid TEXT,character_data BLOB)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO character_info VALUES(42,'7',?)",{mxh::db::bind(*encoded)}).ok());
    EXPECT_FALSE(mxh::db::load_modern_shop_state(db,42,7));
}

TEST_F(LegacyShopAppearance, ReadsOnlySelectedCharacterAndPreservesSignedDwordBits) {
    schema();
    ASSERT_TRUE(db.execute("INSERT INTO TB_SHOPITEMUSEINFO VALUES(42,55001,901,2,-1,3600),(43,55002,902,2,4,5)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO TB_ITEM VALUES(901,91)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO TB_SKININFO VALUES(42,101,102,103,104,105),(43,1,2,3,4,5)").ok());
    const auto result = mxh::db::load_legacy_shop_appearance(db,42);
    ASSERT_TRUE(result.rows); ASSERT_EQ(result.rows->used_items.size(),1u);
    const auto& item = result.rows->used_items[0];
    EXPECT_EQ(item.item_id,55001); EXPECT_EQ(item.database_id,901u); EXPECT_EQ(item.position,91);
    EXPECT_EQ(item.parameter,2u); EXPECT_EQ(item.begin_time,0xffffffffu); EXPECT_EQ(item.remaining_time,3600u);
    EXPECT_EQ(result.rows->skin,(std::array<std::uint16_t,5>{101,102,103,104,105}));
    const auto other = mxh::db::load_legacy_shop_appearance(db,43);
    ASSERT_TRUE(other.rows); EXPECT_EQ(other.rows->used_items[0].position,0);
}

TEST_F(LegacyShopAppearance, InvalidOrFailedSkinDoesNotPublishPartialShopRows) {
    schema();
    ASSERT_TRUE(db.execute("INSERT INTO TB_SHOPITEMUSEINFO VALUES(42,55001,901,2,0,3600)").ok());
    ASSERT_TRUE(db.execute("INSERT INTO TB_SKININFO VALUES(42,NULL,0,0,0,0)").ok());
    EXPECT_EQ(mxh::db::load_legacy_shop_appearance(db,42).status,mxh::db::LegacyShopLoadStatus::InvalidData);
    ASSERT_TRUE(db.execute("DROP TABLE TB_SKININFO").ok());
    const auto result = mxh::db::load_legacy_shop_appearance(db,42);
    EXPECT_EQ(result.status,mxh::db::LegacyShopLoadStatus::QueryFailed); EXPECT_FALSE(result.rows);
}

TEST_F(LegacyShopAppearance, RejectsOutOfRangeFieldsAndAmbiguousSkinRows) {
    schema();
    EXPECT_EQ(mxh::db::load_legacy_shop_appearance(db,0).status,mxh::db::LegacyShopLoadStatus::InvalidCharacter);
    EXPECT_EQ(mxh::db::load_legacy_shop_appearance(db,0x80000000u).status,mxh::db::LegacyShopLoadStatus::InvalidCharacter);
    ASSERT_TRUE(db.execute("INSERT INTO TB_SHOPITEMUSEINFO VALUES(42,65536,1,0,0,0)").ok());
    EXPECT_FALSE(mxh::db::load_legacy_shop_appearance(db,42).rows);
    ASSERT_TRUE(db.execute("DELETE FROM TB_SHOPITEMUSEINFO").ok());
    ASSERT_TRUE(db.execute("INSERT INTO TB_SKININFO VALUES(42,0,0,0,0,0),(42,1,1,1,1,1)").ok());
    EXPECT_FALSE(mxh::db::load_legacy_shop_appearance(db,42).rows);
}

TEST(ModernShopStateMssql, SkinAndUsedRowsRoundTripWithCasAndRollback) {
    const char* connection = std::getenv("MXH_MSSQL_E2E");
    if (!connection || !*connection) GTEST_SKIP() << "Explicit MSSQL test database required";
    auto cfg = mxh::db::ConnectionConfig::from_kv_string(connection);
    cfg.backend = "mssql_odbc";
    auto db = mxh::db::make_adapter(cfg.backend);
    ASSERT_TRUE(db);
    const auto connected = db->connect(cfg);
    ASSERT_TRUE(connected.ok()) << connected.error_message;
    ASSERT_TRUE(db->execute(
        "IF COL_LENGTH(N'dbo.character_info', N'character_data') IS NULL "
        "ALTER TABLE dbo.character_info ADD character_data VARBINARY(MAX) NULL").ok());

    constexpr std::int64_t player = 99042;
    constexpr std::int64_t account = 4242;
    const std::vector<mxh::db::Bind> player_arg{mxh::db::bind(player)};
    ASSERT_TRUE(db->execute("DELETE FROM dbo.character_info WHERE chrid=?", player_arg).ok());
    ASSERT_TRUE(db->execute(
        "INSERT INTO dbo.character_info "
        "(chrid,charname,userid,sex_type,hair_type,face_type,body_type,start_area,"
        "height,width,level,map_num,standing_idx,character_data) "
        "VALUES (?,N'MXSHState',?,0,0,0,0,0,1.0,1.0,1,10,0,NULL)",
        {mxh::db::bind(player), mxh::db::bind(account)}).ok());

    const auto initial = mxh::db::load_modern_shop_state(*db, player, account);
    ASSERT_TRUE(initial);
    mxh::db::LegacyShopAppearanceRows rows;
    rows.skin = {101, 102, 103, 104, 105};
    rows.used_items.push_back({55001, 390, 9001, 1, 0x12345678u, 30000});
    ASSERT_TRUE(mxh::db::save_modern_shop_state(*db, player, account, *initial, rows));
    const auto loaded = mxh::db::load_modern_shop_state(*db, player, account);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded->rows.skin, rows.skin);
    ASSERT_EQ(loaded->rows.used_items.size(), 1u);
    EXPECT_EQ(loaded->rows.used_items[0].database_id, 9001u);
    EXPECT_EQ(loaded->rows.used_items[0].begin_time, 0x12345678u);
    EXPECT_FALSE(mxh::db::save_modern_shop_state(*db, player, account, *initial, {}));

    ASSERT_TRUE(db->begin_transaction().ok());
    ASSERT_TRUE(mxh::db::save_modern_shop_state(*db, player, account, *loaded, {}));
    ASSERT_TRUE(db->rollback().ok());
    const auto rolled_back = mxh::db::load_modern_shop_state(*db, player, account);
    ASSERT_TRUE(rolled_back);
    EXPECT_EQ(rolled_back->rows.skin, rows.skin);
    ASSERT_EQ(rolled_back->rows.used_items.size(), 1u);

    ASSERT_TRUE(db->execute("DELETE FROM dbo.character_info WHERE chrid=?", player_arg).ok());
    db->disconnect();
}

TEST(LegacyShopAppearanceMssql, ReadOnlyRestoredSourceMatchesProcedureRowCounts) {
    const char* connection = std::getenv("MXH_SHOP_SOURCE_READONLY_DB");
    if (!connection || !*connection) GTEST_SKIP() << "Explicit restored-source connection required";
    auto cfg = mxh::db::ConnectionConfig::from_kv_string(connection);
    cfg.backend = "mssql_odbc";
    auto db = mxh::db::make_adapter(cfg.backend);
    ASSERT_TRUE(db);
    const auto connected = db->connect(cfg);
    ASSERT_TRUE(connected.ok()) << connected.error_message;
    // Source-query contract only, not character ownership or gameplay acceptance.
    const auto result = mxh::db::load_legacy_shop_appearance(*db,1);
    ASSERT_EQ(result.status,mxh::db::LegacyShopLoadStatus::Loaded); ASSERT_TRUE(result.rows);
    mxh::db::ResultSet used, skins;
    ASSERT_TRUE(db->query("EXEC dbo.MP_SHOPITEM_UseInfo 1",used).ok());
    ASSERT_TRUE(db->query("EXEC dbo.MP_CHARACTER_SkinInfo 1",skins).ok());
    EXPECT_EQ(result.rows->used_items.size(),used.rows.size());
    ASSERT_EQ(skins.rows.size(),1u); ASSERT_EQ(skins.rows[0].size(),6u);
    for (std::size_t i=0;i<5;++i)
        EXPECT_EQ(result.rows->skin[i],std::get<std::int64_t>(skins.rows[0][i+1]));
}
}
