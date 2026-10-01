using System;
using System.Collections.Generic;

namespace Moxiang
{
    public sealed class LocalAppearance
    {
        public readonly uint PlayerId, CharacterId;
        public readonly ulong SessionGeneration, MapGeneration;
        public readonly byte Gender, FaceType, HairType;
        public readonly IReadOnlyList<ushort> WornItems;
        internal LocalAppearance(CoreSnapshot snapshot,CoreCharacter character,ushort[] equipment)
        {
            PlayerId=snapshot.game.playerId; CharacterId=character.characterId;
            SessionGeneration=snapshot.sessionGeneration; MapGeneration=snapshot.mapGeneration;
            Gender=character.gender; FaceType=character.faceType; HairType=character.hairType;
            WornItems=Array.AsReadOnly(equipment);
        }
    }
    public sealed class LocalAppearanceState
    {
        public LocalAppearance Current {get;private set;}
        public void Clear() {Current=null;}
        public void Observe(CoreSnapshot snapshot,InventoryState inventory)
        {
            if(snapshot.state!=CoreState.InGame || snapshot.game.playerId==0 || inventory==null ||
                !inventory.Matches(snapshot) || snapshot.characters==null) {Clear();return;}
            CoreCharacter? selected=null;
            foreach(var character in snapshot.characters) {
                if(character.valid==0 || character.characterId!=snapshot.selectedCharacterId)continue;
                if(selected.HasValue) {Clear();return;}
                selected=character;
            }
            if(!selected.HasValue) {Clear();return;}
            var chosen=selected.Value;
            bool same=Current!=null && Current.PlayerId==snapshot.game.playerId && Current.CharacterId==chosen.characterId &&
                Current.SessionGeneration==snapshot.sessionGeneration && Current.MapGeneration==snapshot.mapGeneration &&
                Current.Gender==chosen.gender && Current.FaceType==chosen.faceType && Current.HairType==chosen.hairType;
            if(same)for(int i=0;i<10;++i)if(Current.WornItems[i]!=inventory.Slots[80+i].ItemId) {same=false;break;}
            if(same)return;
            var equipment=new ushort[10];
            for(int i=0;i<equipment.Length;++i)equipment[i]=inventory.Slots[80+i].ItemId;
            Current=new LocalAppearance(snapshot,chosen,equipment);
        }
    }
}
