using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace Moxiang
{
    [Serializable] public sealed class AppearanceSource { public string path,sha256; public ulong bytes; }
    [Serializable] public sealed class AppearanceGender {
        public uint gender; public string baseObject; public string[] models,faces,hairs;
    }
    [Serializable] public sealed class AppearanceItem { public uint itemId,partType,modelIndex,weaponType; }
    [Serializable] public sealed class AppearanceDescriptor {
        public uint schemaVersion; public bool releaseReady; public string kind,profileId,converterVersion;
        public AppearanceSource[] sources; public AppearanceGender[] genders; public AppearanceItem[] items;
    }
    public sealed class ImportedAppearance : ScriptableObject
    {
        public AppearanceDescriptor descriptor;
        AppearanceIndex index;
        public AppearanceIndex Index => index ?? (index=new AppearanceIndex(descriptor));
        void OnEnable() {index=null;}
    }
    public sealed class AppearanceIndex
    {
        readonly Dictionary<uint,AppearanceGender> genders=new Dictionary<uint,AppearanceGender>();
        readonly Dictionary<uint,AppearanceItem> items=new Dictionary<uint,AppearanceItem>();
        public int ItemCount => items.Count;
        public AppearanceIndex(AppearanceDescriptor source)
        {
            if(source==null || source.schemaVersion!=1 || source.kind!="appearance-catalog" || source.profileId!="unity-remaster-v1" ||
                source.releaseReady || source.genders==null || source.items==null)throw new InvalidDataException("Unsupported appearance catalog.");
            foreach(var gender in source.genders) {
                if(gender==null || gender.gender>1 || !genders.TryAdd(gender.gender,gender) || string.IsNullOrEmpty(gender.baseObject) ||
                    gender.models==null || gender.faces==null || gender.hairs==null)throw new InvalidDataException("Invalid gender catalog.");
                Names(gender.models);Names(gender.faces);Names(gender.hairs);
            }
            foreach(var item in source.items)
                if(item==null || item.itemId>ushort.MaxValue || item.partType>ushort.MaxValue || item.modelIndex>ushort.MaxValue ||
                    item.weaponType>ushort.MaxValue || !items.TryAdd(item.itemId,item))throw new InvalidDataException("Invalid or duplicate appearance item.");
        }
        static void Names(string[] names) {
            foreach(var name in names)if(string.IsNullOrWhiteSpace(name) || name.Contains('/') || name.Contains('\\') || name.Contains(':') ||
                name=="." || name=="..")throw new InvalidDataException("Invalid resource name in appearance catalog.");
        }
        public bool TryGender(uint gender,out AppearanceGender value) => genders.TryGetValue(gender,out value);
        public bool TryItem(uint id,out AppearanceItem value) => items.TryGetValue(id,out value);
        public bool TryItemModel(uint gender,uint itemId,out string model)
        {
            model=null;
            if(!genders.TryGetValue(gender,out var group) || !items.TryGetValue(itemId,out var item) ||
                item.partType==ushort.MaxValue || item.modelIndex>=group.models.Length)return false;
            model=group.models[item.modelIndex];return true;
        }
        // This lookup does not decide skin/avatar priority, attachment or part replacement.
        public bool TryFace(uint gender,uint face,out string model) {
            model=null;if(!genders.TryGetValue(gender,out var group) || face>=group.faces.Length)return false;
            model=group.faces[face];return true;
        }
        public bool TryHair(uint gender,uint hair,out string model) {
            model=null;if(!genders.TryGetValue(gender,out var group) || hair>=group.hairs.Length)return false;
            model=group.hairs[hair];return true;
        }
    }
}
