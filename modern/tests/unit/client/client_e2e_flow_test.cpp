#include "CEngine.hpp"
#include "CMainGame.hpp"
#include "CGameState.hpp"
#include <gtest/gtest.h>
using namespace mxh::client;
namespace {
class ProbeState final : public CGameState {
public:
 explicit ProbeState(int* enters, int* leaves): enters_(enters), leaves_(leaves) {}
 void Init(void*) override { ++*enters_; setInitialized(true); }
 void Release() override { ++*leaves_; setInitialized(false); }
private: int* enters_; int* leaves_;
};
}
TEST(ClientE2E, SixStateLifecyclePath) {
 CMainGame game; game.Init(nullptr); int enters=0, leaves=0;
 auto add=[&](GameStateId id){ game.RegisterState(id, std::make_unique<ProbeState>(&enters,&leaves)); };
 add(GameStateId::Title); add(GameStateId::Connect); add(GameStateId::CharSelect); add(GameStateId::GameIn); add(GameStateId::MurimNet);
 for (auto id: {GameStateId::Title,GameStateId::Connect,GameStateId::CharSelect,GameStateId::GameIn,GameStateId::MurimNet,GameStateId::Title}) { game.SetGameState(id); game.Process(); ASSERT_EQ(game.GetCurStateNum(),id); ASSERT_NE(game.GetCurGameState(),nullptr); }
 EXPECT_EQ(enters,6); EXPECT_EQ(leaves,5); game.Release();
}

