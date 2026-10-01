using System.IO;
using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class AppearancePartRuleTests
    {
        static AppearanceIndex Catalog(uint part) => new AppearanceIndex(new AppearanceDescriptor {
            schemaVersion=1,kind="appearance-catalog",profileId="unity-remaster-v1",
            genders=new[] {
                new AppearanceGender {gender=0,baseObject="man.chx",models=new[]{"male.mod"},faces=new string[0],hairs=new string[0]},
                new AppearanceGender {gender=1,baseObject="woman.chx",models=new[]{"female.mod"},faces=new string[0],hairs=new string[0]} },
            items=new[]{new AppearanceItem {itemId=100,partType=part,modelIndex=0}} });
        [Test]
        public void MaleHairReplacesPartButFemaleHeadwearAttachesToHead()
        {
            var male=AppearancePartRule.Resolve(Catalog(0),0,100);
            Assert.That(male.Kind,Is.EqualTo(AppearanceOperationKind.ReplacePart));Assert.That(male.PartIndex,Is.Zero);
            var female=AppearancePartRule.Resolve(Catalog(0),1,100);
            Assert.That(female.Kind,Is.EqualTo(AppearanceOperationKind.AttachHead));
            Assert.That(female.AttachmentNode,Is.EqualTo("Bip01 Head"));Assert.That(female.Model,Is.EqualTo("female.mod"));
        }
        [TestCase(0u,"NULLHAIR_M.MOD")]
        [TestCase(1u,"NULLHAIR_W.MOD")]
        public void TypeSevenRetainsRequiredHairReplacement(uint gender,string replacement)
        {
            var rule=AppearancePartRule.Resolve(Catalog(7),gender,100);
            Assert.That(rule.Kind,Is.EqualTo(AppearanceOperationKind.AttachHead));Assert.That(rule.HiddenHairModel,Is.EqualTo(replacement));
        }
        [Test]
        public void WeaponIsNotAppendedAsAnOrdinaryBodyPart()
        {
            Assert.That(AppearancePartRule.Resolve(Catalog(5),0,100).Kind,Is.EqualTo(AppearanceOperationKind.SeparateWeapon));
            Assert.That(AppearancePartRule.Resolve(Catalog(65535),0,100).Kind,Is.EqualTo(AppearanceOperationKind.None));
            Assert.Throws<InvalidDataException>(()=>AppearancePartRule.Resolve(Catalog(8),0,100));
            Assert.Throws<InvalidDataException>(()=>AppearancePartRule.Resolve(Catalog(2),0,999));
        }
    }
}
