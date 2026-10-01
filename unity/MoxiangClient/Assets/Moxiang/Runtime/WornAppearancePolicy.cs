using System;
using System.IO;

namespace Moxiang
{
    public sealed class WornAppearanceContext
    {
        // Effective Avatar array AFTER bNoAvatarView has been applied (23 entries).
        // No default zeros: that would incorrectly hide ordinary clothing.
        public ushort[] Avatar;
        // Optional resolved AVATARITEM::Item row for Avatar[6], when one exists.
        public ushort[] AvatarDressParts;
        public ushort SkinHat,SkinMask,SkinDress,SkinShoes;
        public bool FullMoonEvent;
    }
    public sealed class WornAppearanceDecision
    {
        public bool ApplyModel,HideHands,HideFeet;
        public string HiddenHairModel;
        public AppearancePartRule Operation;
    }
    public static class WornAppearancePolicy
    {
        public static WornAppearanceDecision Evaluate(AppearanceIndex catalog,uint gender,int slot,uint itemId,WornAppearanceContext context)
        {
            if(catalog==null)throw new ArgumentNullException(nameof(catalog));
            if(gender>1 || slot<0 || slot>=10 || context?.Avatar==null || context.Avatar.Length!=23 ||
                (context.AvatarDressParts!=null && context.AvatarDressParts.Length!=23))
                throw new InvalidDataException("Incomplete worn appearance policy input.");
            var result=new WornAppearanceDecision();var avatar=context.Avatar;
            if(itemId==0)return result;
            // Keep the original n + eAvatar_Weared_Hat indexing, including dress slot 2 -> 16.
            if((slot<3 && slot!=1 && avatar[slot+14]==0) || (slot==6 && avatar[14]==0))return result;
            if(!catalog.TryItem(itemId,out var item))throw new InvalidDataException("Unknown worn item.");
            uint part=item.partType;
            if(part==65535)return result;
            if(gender==1 && part==0)part=6;
            // This side effect precedes the headgear suppression checks in the source.
            if(part==7) {result.HiddenHairModel=gender==0 ? "NULLHAIR_M.MOD" : "NULLHAIR_W.MOD";part=6;}
            var dress=context.AvatarDressParts;
            if(part==6) {
                if((avatar[6]>0 && dress!=null && (dress[0]==0 || dress[14]==0)) ||
                    avatar[1]!=0 || avatar[3]!=0 || avatar[4]!=0 || context.SkinHat!=0 || context.SkinMask!=0 || context.FullMoonEvent)
                    return result;
            } else if(part==3) {
                // The glove/weapon skin substitution takes the dedicated ChangePart
                // branch, before ordinary dress/foot masking. Do not rerun head remapping.
                if(avatar[18]>1 && item.weaponType>0) {
                    if(!catalog.TryItem(avatar[18],out item) || !catalog.TryItemModel(gender,avatar[18],out var replacement))
                        throw new InvalidDataException("Unresolved glove appearance override.");
                    if(item.partType>4)throw new InvalidDataException("Unsupported glove override part.");
                    result.Operation=new AppearancePartRule(AppearanceOperationKind.ReplacePart,item.partType,replacement);
                    result.ApplyModel=true;return result;
                }
            } else if(part==5) {
                // This pass does not render weapons; the separate weapon pass owns it.
                return result;
            } else {
                if(avatar[6]>0 && dress!=null) {
                    result.HideHands=dress[11]==0;result.HideFeet=dress[9]==0;
                    if(part==4 && (avatar[9]==0 || context.SkinShoes!=0))return result;
                }
                if(context.SkinDress!=0)return result;
            }
            result.Operation=AppearancePartRule.Resolve(catalog,gender,itemId);
            result.ApplyModel=true;return result;
        }
    }
}
