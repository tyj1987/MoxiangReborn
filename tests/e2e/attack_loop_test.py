"""Deterministic client -> server -> client attack-loop scenarios."""
from dataclasses import dataclass
import struct
@dataclass
class Request: minimum:int; maximum:int; mana:int; cost:int; random_gap:int
@dataclass
class Result: damage:int; mana_after:int; executed:bool
def server_resolve(r:Request)->Result:
    if r.mana<r.cost:return Result(0,r.mana,False)
    span=r.maximum-r.minimum+1
    return Result(r.minimum+(r.random_gap%span),r.mana-r.cost,True) if span>0 else Result(r.minimum,r.mana-r.cost,True)
def encode_client_packet(r:Result)->bytes:return struct.pack("<III",r.damage,r.mana_after,int(r.executed))
def decode_client_packet(p:bytes)->Result:return Result(*((lambda x:(x[0],x[1],bool(x[2])))(struct.unpack("<III",p))))
def run(r:Request)->Result:return decode_client_packet(encode_client_packet(server_resolve(r)))
def test_successful_attack_round_trip():assert run(Request(10,20,50,7,3))==Result(13,43,True)
def test_insufficient_mana_round_trip():assert run(Request(10,20,6,7,3))==Result(0,6,False)
def test_multiple_deterministic_attacks():
    results=[run(Request(100,110,1000-i,10,i)) for i in range(6)]
    assert [r.damage for r in results]==[100,101,102,103,104,105] and all(r.executed for r in results)
def test_packet_is_fixed_size_and_little_endian():
    packet=encode_client_packet(Result(0x01020304,0x05060708,True)); assert len(packet)==12 and packet[:4]==b"\x04\x03\x02\x01"
if __name__=="__main__":
    tests=[test_successful_attack_round_trip,test_insufficient_mana_round_trip,test_multiple_deterministic_attacks,test_packet_is_fixed_size_and_little_endian]
    for test in tests:test()
    print(f"{len(tests)} attack-loop tests passed")
