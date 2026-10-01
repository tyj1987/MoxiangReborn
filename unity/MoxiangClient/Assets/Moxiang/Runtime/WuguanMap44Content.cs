using System;
using System.IO;
using System.Security.Cryptography;
using UnityEngine;
namespace Moxiang
{
    public sealed class WuguanMap44Content : MonoBehaviour
    {
        public const ushort MapNumber=44;
        public const float WorldWidth=51200,WorldDepth=51200,TileSize=50;
        public const string TileDigest="6e936bc95cc95a3a070df6d20106337d699da6c8d3e65c08b6c049f4bf9d9cfb";
        public MeshCollider navigation;
        public TextAsset tileSource;
        public WuguanTrainingHallModule hall;
        private byte[] cachedTiles;
        private TextAsset cachedSource;
        private byte[] Tiles {get{if(cachedSource!=tileSource){cachedSource=tileSource;cachedTiles=tileSource?tileSource.bytes:null;}return cachedTiles;}}
        public Vector3 SpawnGame => new Vector3(10650,0,12500);
        public bool IsWalkable(Vector3 game)
        {
            if(!tileSource||!float.IsFinite(game.x)||!float.IsFinite(game.z)||game.x<0||game.z<0||game.x>=WorldWidth||game.z>=WorldDepth)return false;
            var bytes=Tiles;int x=(int)(game.x/TileSize),z=(int)(game.z/TileSize);
            if(bytes.Length!=2097160)return false;
            return (bytes[8+(z*1024+x)*2]&1)==0;
        }
        public void Validate()
        {
            if(!navigation||!navigation.sharedMesh||!hall||!hall.IsConfigured||!tileSource)
                throw new InvalidDataException("Map44 content references incomplete.");
            var bytes=Tiles;
            if(bytes.Length!=2097160||BitConverter.ToUInt32(bytes,0)!=1024||BitConverter.ToUInt32(bytes,4)!=1024)
                throw new InvalidDataException("Map44 tile dimensions changed.");
            using(var hash=SHA256.Create())
                if(!string.Equals(BitConverter.ToString(hash.ComputeHash(bytes)).Replace("-",""),TileDigest,StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException("Map44 source TTB hash mismatch.");
            if(!IsWalkable(SpawnGame))throw new InvalidDataException("Canonical Map44 spawn is blocked.");
            var spawn=new MapCoordinates(WorldWidth,WorldDepth).ToScene(SpawnGame);
            if(Vector3.Distance(hall.combatZone.position,spawn)>.01f)throw new InvalidDataException("Hall lost its authoritative map origin.");
        }
    }
}
