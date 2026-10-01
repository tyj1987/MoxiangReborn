using System;
using System.IO;

namespace Moxiang
{
    public enum AppearanceOperationKind { None, ReplacePart, AttachHead, SeparateWeapon }
    public readonly struct AppearancePartRule
    {
        public readonly AppearanceOperationKind Kind;
        public readonly uint PartIndex;
        public readonly string Model, HiddenHairModel;
        public string AttachmentNode => Kind==AppearanceOperationKind.AttachHead ? "Bip01 Head" : null;
        internal AppearancePartRule(AppearanceOperationKind kind,uint part,string model,string hiddenHair=null)
        {Kind=kind;PartIndex=part;Model=model;HiddenHairModel=hiddenHair;}

        // Apply only AFTER the worn/skin/avatar priority pass admits this item.
        // Original active AppearanceManager.cpp:1000-1017,1048-1050,1081-1126.
        public static AppearancePartRule Resolve(AppearanceIndex catalog,uint gender,uint itemId)
        {
            if(catalog==null)throw new ArgumentNullException(nameof(catalog));
            if(gender>1)throw new InvalidDataException("Unsupported gender.");
            if(!catalog.TryItem(itemId,out var item))throw new InvalidDataException("Unknown appearance item.");
            if(item.partType==ushort.MaxValue)return new AppearancePartRule(AppearanceOperationKind.None,0,null);
            if(!catalog.TryItemModel(gender,itemId,out var model))throw new InvalidDataException("Unresolved item model index.");
            uint part=item.partType;string hiddenHair=null;
            if(gender==1 && part==0)part=6;
            if(part==7) {hiddenHair=gender==0 ? "NULLHAIR_M.MOD" : "NULLHAIR_W.MOD";part=6;}
            if(part==6)return new AppearancePartRule(AppearanceOperationKind.AttachHead,part,model,hiddenHair);
            // The worn-part pass excludes type 5. Weapon-specific code must decide
            // attachment nodes and visibility; never append it as a body mesh.
            if(part==5)return new AppearancePartRule(AppearanceOperationKind.SeparateWeapon,part,model);
            if(part<=4)return new AppearancePartRule(AppearanceOperationKind.ReplacePart,part,model);
            throw new InvalidDataException("Unsupported appearance part type.");
        }
    }
}
