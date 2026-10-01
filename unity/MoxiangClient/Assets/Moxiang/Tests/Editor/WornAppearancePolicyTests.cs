using System.IO;
using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class WornAppearancePolicyTests
    {
        static WornAppearanceContext Context() {
            var value=new WornAppearanceContext {Avatar=new ushort[23]};
            for(int i=12;i<23;++i)value.Avatar[i]=1;
            return value;
        }
        static AppearanceIndex Catalog(uint part,uint weapon=0) => new AppearanceIndex(new AppearanceDescriptor {
            schemaVersion=1,kind="appearance-catalog",profileId="unity-remaster-v1",
            genders=new[]{new AppearanceGender {gender=0,baseObject="man.chx",models=new[]{"item.mod"},faces=new string[0],hairs=new string[0]}},
            items=new[]{new AppearanceItem {itemId=100,partType=part,weaponType=weapon},new AppearanceItem {itemId=200,partType=3}} });
        [Test]
        public void WornFlagsAndSkinDressPreventReplacement()
        {
            var context=Context();var catalog=Catalog(2);
            Assert.That(WornAppearancePolicy.Evaluate(catalog,0,2,100,context).ApplyModel,Is.True);
            context.Avatar[16]=0;
            Assert.That(WornAppearancePolicy.Evaluate(catalog,0,2,100,context).ApplyModel,Is.False);
            context.Avatar[16]=1;context.SkinDress=1;
            Assert.That(WornAppearancePolicy.Evaluate(catalog,0,2,100,context).ApplyModel,Is.False);
        }
        [Test]
        public void HairHidingOccursEvenWhenHeadAttachmentIsSuppressed()
        {
            var context=Context();context.SkinMask=12;
            var decision=WornAppearancePolicy.Evaluate(Catalog(7),0,0,100,context);
            Assert.That(decision.ApplyModel,Is.False);Assert.That(decision.HiddenHairModel,Is.EqualTo("NULLHAIR_M.MOD"));
        }
        [Test]
        public void DressHidesHandsAndFeetBeforeSuppressingShoeReplacement()
        {
            var context=Context();context.Avatar[6]=500;context.AvatarDressParts=new ushort[23];
            var decision=WornAppearancePolicy.Evaluate(Catalog(4),0,3,100,context);
            Assert.That(decision.ApplyModel,Is.False);Assert.That(decision.HideHands,Is.True);Assert.That(decision.HideFeet,Is.True);
        }
        [Test]
        public void GloveOverrideBypassesOrdinaryDressSuppressionAndWeaponsStaySeparate()
        {
            var context=Context();context.SkinDress=1;context.Avatar[18]=200;
            var decision=WornAppearancePolicy.Evaluate(Catalog(3,2),0,1,100,context);
            Assert.That(decision.ApplyModel,Is.True);Assert.That(decision.Operation.PartIndex,Is.EqualTo(3));
            Assert.That(WornAppearancePolicy.Evaluate(Catalog(5),0,1,100,context).ApplyModel,Is.False);
            Assert.Throws<InvalidDataException>(()=>WornAppearancePolicy.Evaluate(Catalog(2),0,2,100,new WornAppearanceContext()));
        }
    }
}
