using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class LocalAppearanceStateTests
    {
        static CoreSnapshot Snapshot() => new CoreSnapshot {state=CoreState.InGame,sessionGeneration=2,mapGeneration=3,
            selectedCharacterId=101,game=new CoreGame {playerId=101},characters=new[] {
                new CoreCharacter {valid=1,characterId=101,gender=0,faceType=2,hairType=1,wornItems=new ushort[] {999}} }};
        static CoreEvent Chunk(CoreSnapshot snapshot,uint offset,ushort equipped)
        {
            var bytes=new byte[System.Math.Min(11u,124-offset)*22];
            for(int i=0;i<bytes.Length/22;++i)if(offset+i==80) {bytes[i*22+4]=(byte)equipped;bytes[i*22+5]=(byte)(equipped>>8);}
            return new CoreEvent {type=NativeClient.EventInventory,state=CoreState.InGame,
                sessionGeneration=snapshot.sessionGeneration,mapGeneration=snapshot.mapGeneration,argument0=snapshot.game.playerId,
                argument1=124,reserved0=offset,text=bytes,textLength=(uint)bytes.Length};
        }
        static void Fill(InventoryState inventory,CoreSnapshot snapshot,ushort item)
        {
            inventory.Observe(snapshot);
            for(uint offset=0;offset<124;offset+=11)inventory.Accept(Chunk(snapshot,offset,item));
        }
        [Test]
        public void CurrentEquipmentOverridesCharacterListAndPublishesOnlyCompleteUpdates()
        {
            var snapshot=Snapshot();var inventory=new InventoryState();var state=new LocalAppearanceState();
            state.Observe(snapshot,inventory);Assert.That(state.Current,Is.Null);
            Fill(inventory,snapshot,123);state.Observe(snapshot,inventory);
            var first=state.Current;Assert.That(first.WornItems[0],Is.EqualTo(123));
            Assert.That(first.FaceType,Is.EqualTo(2));Assert.That(first.HairType,Is.EqualTo(1));
            state.Observe(snapshot,inventory);Assert.That(state.Current,Is.SameAs(first));
            inventory.Accept(Chunk(snapshot,0,456));state.Observe(snapshot,inventory);Assert.That(state.Current,Is.Null);
            Fill(inventory,snapshot,456);state.Observe(snapshot,inventory);
            Assert.That(state.Current.WornItems[0],Is.EqualTo(456));Assert.That(first.WornItems[0],Is.EqualTo(123));
        }
        [Test]
        public void StaleInventoryOrAmbiguousCharacterCannotPopulateAppearance()
        {
            var snapshot=Snapshot();var inventory=new InventoryState();var state=new LocalAppearanceState();
            Fill(inventory,snapshot,123);state.Observe(snapshot,inventory);Assert.That(state.Current,Is.Not.Null);
            snapshot.mapGeneration++;state.Observe(snapshot,inventory);Assert.That(state.Current,Is.Null);
            snapshot.mapGeneration--;snapshot.sessionGeneration++;state.Observe(snapshot,inventory);Assert.That(state.Current,Is.Null);
            snapshot.sessionGeneration--;snapshot.characters=new[] {snapshot.characters[0],snapshot.characters[0]};
            state.Observe(snapshot,inventory);Assert.That(state.Current,Is.Null);
            snapshot=Snapshot();snapshot.selectedCharacterId=999;state.Observe(snapshot,inventory);Assert.That(state.Current,Is.Null);
        }
    }
}
