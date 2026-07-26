"""Six-state client flow model used by the deterministic E2E harness."""
from enum import Enum
class State(Enum): LOGIN="CLoginState"; CHAR_SELECT="CCharSelectState"; IN_GAME="CInGameState"; BATTLE="BattleState"; SHOP="ShopState"; GUILD="GuildState"
TRANSITIONS={State.LOGIN:{State.CHAR_SELECT},State.CHAR_SELECT:{State.IN_GAME},State.IN_GAME:{State.BATTLE,State.SHOP,State.GUILD},State.BATTLE:{State.IN_GAME},State.SHOP:{State.IN_GAME},State.GUILD:{State.IN_GAME}}
class Flow:
 def __init__(self): self.state=State.LOGIN; self.history=[self.state]
 def go(self,next_state):
  if next_state not in TRANSITIONS[self.state]: raise ValueError(f"invalid transition {self.state}->{next_state}")
  self.state=next_state; self.history.append(next_state)
def test_login_to_game():
 f=Flow();f.go(State.CHAR_SELECT);f.go(State.IN_GAME);assert f.history==[State.LOGIN,State.CHAR_SELECT,State.IN_GAME]
def test_battle_returns_to_game():
 f=Flow();f.go(State.CHAR_SELECT);f.go(State.IN_GAME);f.go(State.BATTLE);f.go(State.IN_GAME);assert f.state is State.IN_GAME
def test_shop_and_guild_paths():
 f=Flow();f.go(State.CHAR_SELECT);f.go(State.IN_GAME);f.go(State.SHOP);f.go(State.IN_GAME);f.go(State.GUILD);f.go(State.IN_GAME);assert f.history[-5:]==[State.IN_GAME,State.SHOP,State.IN_GAME,State.GUILD,State.IN_GAME]
def test_invalid_login_to_battle_rejected():
 try: Flow().go(State.BATTLE)
 except ValueError: return
 assert False,"login must not transition directly to battle"
if __name__=="__main__":
 tests=[test_login_to_game,test_battle_returns_to_game,test_shop_and_guild_paths,test_invalid_login_to_battle_rejected]
 for t in tests:t()
 print(f"{len(tests)} six-state tests passed")
