// 1:1 lock tests for cMonsterSpeechManager (Phase D6 Boss 刷新).
// The class loads MonsterSpeechInfoList.bin at server start and lets the
// boss engine trigger a chat line by (MonsterKind, trigger). Modern port
// keeps the flat vector + first-match find semantics of the legacy class.

#include "mxh/server/c_monster_speech_manager.hpp"

#include <gtest/gtest.h>

#include <cstring>

namespace {

using mxh::server::cMonsterSpeechManager;
using mxh::server::MonsterSpeech;

MonsterSpeech make_speech(std::uint32_t monster_kind,
                         std::uint8_t trigger,
                         const char* text,
                         std::uint32_t speech_id = 0) {
    MonsterSpeech s{};
    s.monster_kind = monster_kind;
    s.trigger      = trigger;
    s.speech_id    = speech_id;
    std::strncpy(s.text, text, sizeof(s.text) - 1);
    return s;
}

}  // namespace

TEST(MonsterSpeechTest, DefaultManagerIsEmpty) {
    cMonsterSpeechManager mgr;
    EXPECT_EQ(mgr.size(), 0u);
    EXPECT_EQ(mgr.find(100, /*trigger=*/0), nullptr);
    EXPECT_EQ(mgr.death_speech(100), nullptr);
}

TEST(MonsterSpeechTest, RegisterIncreasesSize) {
    cMonsterSpeechManager mgr;
    EXPECT_EQ(mgr.size(), 0u);
    mgr.register_speech(make_speech(100, 0, "hi"));
    EXPECT_EQ(mgr.size(), 1u);
    mgr.register_speech(make_speech(100, 1, "ouch"));
    EXPECT_EQ(mgr.size(), 2u);
    mgr.register_speech(make_speech(100, 2, "bye"));
    EXPECT_EQ(mgr.size(), 3u);
}

TEST(MonsterSpeechTest, FindReturnsRegisteredEntry) {
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(1234, /*trigger=*/0, "Argh!", /*speech_id=*/7));
    const MonsterSpeech* s = mgr.find(1234, 0);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->monster_kind, 1234u);
    EXPECT_EQ(s->trigger, 0u);
    EXPECT_EQ(s->speech_id, 7u);
    EXPECT_STREQ(s->text, "Argh!");
}

TEST(MonsterSpeechTest, FindByTriggerDistinguishesSpawnHalfHpDeath) {
    // Legacy trigger codes: 0=on-spawn, 1=on-HP-50%, 2=on-die.
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(100, 0, "born"));
    mgr.register_speech(make_speech(100, 1, "half"));
    mgr.register_speech(make_speech(100, 2, "dying"));

    const MonsterSpeech* spawn = mgr.find(100, 0);
    const MonsterSpeech* half  = mgr.find(100, 1);
    const MonsterSpeech* die   = mgr.find(100, 2);
    ASSERT_NE(spawn, nullptr);
    ASSERT_NE(half, nullptr);
    ASSERT_NE(die, nullptr);
    EXPECT_STREQ(spawn->text, "born");
    EXPECT_STREQ(half->text, "half");
    EXPECT_STREQ(die->text, "dying");
}

TEST(MonsterSpeechTest, FindDifferentiatesMultipleKinds) {
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(100, 2, "die-100"));
    mgr.register_speech(make_speech(200, 2, "die-200"));
    mgr.register_speech(make_speech(300, 2, "die-300"));

    EXPECT_STREQ(mgr.find(100, 2)->text, "die-100");
    EXPECT_STREQ(mgr.find(200, 2)->text, "die-200");
    EXPECT_STREQ(mgr.find(300, 2)->text, "die-300");
    EXPECT_EQ(mgr.find(400, 2), nullptr);
}

TEST(MonsterSpeechTest, FindReturnsFirstMatchForDuplicateKey) {
    // 1:1 with legacy: flat vector, no dedup, first match wins.
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(100, 2, "first"));
    mgr.register_speech(make_speech(100, 2, "second"));
    EXPECT_EQ(mgr.size(), 2u);
    EXPECT_STREQ(mgr.find(100, 2)->text, "first");
}

TEST(MonsterSpeechTest, FindReturnsNullForUnknownKind) {
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(100, 2, "die"));
    EXPECT_EQ(mgr.find(999, 2), nullptr);
    EXPECT_EQ(mgr.find(0, 2), nullptr);
}

TEST(MonsterSpeechTest, FindReturnsNullForUnknownTrigger) {
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(100, 2, "die"));
    EXPECT_NE(mgr.find(100, 2), nullptr);
    EXPECT_EQ(mgr.find(100, 0), nullptr);
    EXPECT_EQ(mgr.find(100, 1), nullptr);
    EXPECT_EQ(mgr.find(100, 99), nullptr);
}

TEST(MonsterSpeechTest, DeathSpeechReturnsTriggerTwo) {
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(100, 0, "spawn"));
    mgr.register_speech(make_speech(100, 1, "half"));
    mgr.register_speech(make_speech(100, 2, "die"));
    const MonsterSpeech* d = mgr.death_speech(100);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->trigger, 2u);
    EXPECT_STREQ(d->text, "die");
}

TEST(MonsterSpeechTest, DeathSpeechReturnsNullWhenMissing) {
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(100, 0, "spawn"));
    // No trigger-2 entry: death_speech returns nullptr.
    EXPECT_EQ(mgr.death_speech(100), nullptr);
    // Empty manager also returns nullptr.
    cMonsterSpeechManager mgr2;
    EXPECT_EQ(mgr2.death_speech(100), nullptr);
}

TEST(MonsterSpeechTest, FindAndDeathAgreeOnSameEntry) {
    // death_speech is documented as a convenience for find(kind, 2).
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(200, 2, "goodbye"));
    EXPECT_EQ(mgr.death_speech(200), mgr.find(200, 2));
}

TEST(MonsterSpeechTest, SpeechIdPreserved) {
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(100, 2, "die", /*speech_id=*/12345));
    EXPECT_EQ(mgr.find(100, 2)->speech_id, 12345u);
}

TEST(MonsterSpeechTest, LongTextFitsBuffer) {
    // MAX_MONSTER_SPEECH_LEN = 128. Use a string just under that.
    cMonsterSpeechManager mgr;
    char buf[128] = {};
    for (int i = 0; i < 120; ++i) buf[i] = static_cast<char>(97 + (i % 26));
    buf[120] = 0;
    mgr.register_speech(make_speech(100, 2, buf));
    EXPECT_EQ(std::strlen(mgr.find(100, 2)->text), 120u);
}

TEST(MonsterSpeechTest, MultipleKindsIndependent) {
    cMonsterSpeechManager mgr;
    mgr.register_speech(make_speech(100, 0, "a"));
    mgr.register_speech(make_speech(200, 0, "b"));
    mgr.register_speech(make_speech(300, 0, "c"));
    EXPECT_STREQ(mgr.find(100, 0)->text, "a");
    EXPECT_STREQ(mgr.find(200, 0)->text, "b");
    EXPECT_STREQ(mgr.find(300, 0)->text, "c");
    EXPECT_EQ(mgr.find(400, 0), nullptr);
}

TEST(MonsterSpeechTest, MaxSpeechPerMonsterConstantIsEight) {
    // Legacy cMonsterSpeechManager caps each (kind, trigger) at 8 cues.
    EXPECT_EQ(mxh::server::MAX_SPEECH_PER_MONSTER, 8u);
}
