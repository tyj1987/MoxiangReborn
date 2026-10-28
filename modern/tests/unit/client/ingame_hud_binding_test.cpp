#include "CInGameState.hpp"
#include "mxh/ui/cObjectGuagen.hpp"
#include "mxh/ui/cStatic.hpp"

#include <gtest/gtest.h>

#include <cstring>

namespace {

mxh::ui::cStatic* label(mxh::client::CInGameState& state,
                        std::string_view id) {
    return dynamic_cast<mxh::ui::cStatic*>(
        state.ui_runtime().findWindowByLegacyId(id));
}

mxh::ui::cObjectGuagen* gauge(mxh::client::CInGameState& state,
                              std::string_view id) {
    return dynamic_cast<mxh::ui::cObjectGuagen*>(
        state.ui_runtime().findWindowByLegacyId(id));
}

mxh::client::GameInInfo live_ack() {
    mxh::client::GameInInfo info;
    info.player_id = 100003;
    info.name = "MoxR12Gui";
    info.level = 1;
    info.map_num = 10;
    info.life = 125;
    info.max_life = 125;
    info.shield = 20;
    info.max_shield = 40;
    info.mp = 60;
    info.max_mp = 100;
    info.position_x = 25000;
    info.position_z = 25100;
    return info;
}

TEST(InGameHudBinding, GameInAndDamageRefreshShippedGaugeWidgets) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    ASSERT_NE(label(state, "CG_GUAGENAME"), nullptr);
    ASSERT_NE(label(state, "CG_GUAGELEVEL"), nullptr);
    ASSERT_NE(label(state, "MNM_MAPNAME"), nullptr);
    state.dispatch_gamein_ack(live_ack());
    state.Process();

    EXPECT_EQ(label(state, "CG_GUAGENAME")->GetStaticText(), "MoxR12Gui");
    EXPECT_EQ(label(state, "CG_GUAGELEVEL")->GetStaticText(), "1");
    EXPECT_EQ(label(state, "CG_GUAGELIFETEXT")->GetStaticText(), "125 / 125");
    EXPECT_EQ(label(state, "CG_GUAGESHIELDTEXT")->GetStaticText(), "20 / 40");
    EXPECT_EQ(label(state, "CG_GUAGENAERYUKTEXT")->GetStaticText(), "60 / 100");
    EXPECT_EQ(label(state, "MNM_MAPNAME")->GetStaticText(), "Map 10");
    EXPECT_EQ(label(state, "MNM_POSXY")->GetStaticText(), "250,251");
    ASSERT_NE(gauge(state, "CG_GUAGELIFE"), nullptr);
    EXPECT_FLOAT_EQ(gauge(state, "CG_GUAGELIFE")->GetCurPercentRate(), 1.0f);
    EXPECT_FLOAT_EQ(gauge(state, "CG_GUAGESHIELD")->GetCurPercentRate(), 0.5f);
    EXPECT_FLOAT_EQ(gauge(state, "CG_GUAGENAERYUK")->GetCurPercentRate(), 0.6f);

    mxh::net::Message life;
    life.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Character);
    life.header.protocol = static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::LifeAck);
    life.header.object_id = 100003;
    life.payload.resize(4);
    const std::int32_t damage = -25;
    std::memcpy(life.payload.data(), &damage, sizeof(damage));
    state.on_message({}, life);
    EXPECT_EQ(label(state, "CG_GUAGELIFETEXT")->GetStaticText(), "100 / 125");
    EXPECT_FLOAT_EQ(gauge(state, "CG_GUAGELIFE")->GetCurPercentRate(), 0.8f);

    auto shield = life;
    shield.header.protocol = static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::ShieldAck);
    const std::int32_t shield_damage = -10;
    std::memcpy(shield.payload.data(), &shield_damage, sizeof(shield_damage));
    state.on_message({}, shield);
    EXPECT_EQ(label(state, "CG_GUAGESHIELDTEXT")->GetStaticText(), "10 / 40");
    EXPECT_FLOAT_EQ(gauge(state, "CG_GUAGESHIELD")->GetCurPercentRate(), 0.25f);
}

TEST(InGameHudBinding, MissingAuthoritativeFieldsStayBlank) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.Process();
    EXPECT_TRUE(label(state, "CG_GUAGENAME")->GetStaticText().empty());
    EXPECT_TRUE(label(state, "MNM_MAPNAME")->GetStaticText().empty());

    mxh::client::GameInInfo incomplete;
    incomplete.player_id = 7;
    incomplete.life = 0;
    incomplete.max_life = 0;
    state.dispatch_gamein_ack(incomplete);
    state.Process();
    for (const auto id : {"CG_GUAGENAME", "CG_GUAGELEVEL", "CG_GUAGELIFETEXT",
                          "CG_GUAGESHIELDTEXT", "CG_GUAGENAERYUKTEXT",
                          "MNM_MAPNAME", "MNM_POSXY"}) {
        ASSERT_NE(label(state, id), nullptr);
        EXPECT_TRUE(label(state, id)->GetStaticText().empty()) << id;
    }
    EXPECT_FLOAT_EQ(gauge(state, "CG_GUAGELIFE")->GetCurPercentRate(), 0.0f);
}

}  // namespace
