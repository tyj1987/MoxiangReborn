using System.IO;
using NUnit.Framework;
using UnityEditor;

namespace Moxiang.Tests
{
    public sealed class AppearanceIndexTests
    {
        [Test]
        public void RealCatalogImportsAllItemsAndKeepsGenderAndFaceIndices()
        {
            var asset=AssetDatabase.LoadAssetAtPath<ImportedAppearance>("Assets/Moxiang/Derived/appearance-v1.mxhappearance");
            Assert.That(asset,Is.Not.Null);Assert.That(asset.Index.ItemCount,Is.EqualTo(9887));
            Assert.That(asset.Index.TryFace(0,0,out var face),Is.True);Assert.That(face.ToLowerInvariant(),Is.EqualTo("m_face01.mod"));
            Assert.That(asset.Index.TryFace(1,0,out var female),Is.True);Assert.That(female,Is.Not.EqualTo(face));
            Assert.That(asset.Index.TryFace(0,5,out _),Is.False);
            Assert.That(asset.Index.TryHair(255,0,out _),Is.False);
        }
        [Test]
        public void MissingModelsNeverFallbackAndDuplicateItemsAreRejected()
        {
            var data=new AppearanceDescriptor {schemaVersion=1,kind="appearance-catalog",profileId="unity-remaster-v1",
                genders=new[] {new AppearanceGender {gender=0,baseObject="man.chx",models=new[] {"body.mod"},faces=new[] {"face.mod"},hairs=new[] {"hair.mod"}}},
                items=new[] {new AppearanceItem {itemId=1,partType=2,modelIndex=0},new AppearanceItem {itemId=2,partType=2,modelIndex=1},
                    new AppearanceItem {itemId=3,partType=65535,modelIndex=0}}};
            var index=new AppearanceIndex(data);
            Assert.That(index.TryItemModel(0,1,out var model),Is.True);Assert.That(model,Is.EqualTo("body.mod"));
            Assert.That(index.TryItemModel(0,2,out _),Is.False);Assert.That(index.TryItemModel(0,3,out _),Is.False);
            Assert.That(index.TryItemModel(0,999,out _),Is.False);
            data.items=new[] {data.items[0],data.items[0]};Assert.Throws<InvalidDataException>(()=>new AppearanceIndex(data));
        }
    }
}
