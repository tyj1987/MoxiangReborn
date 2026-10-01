using System.Collections.Generic;
using System.IO;
using System.Linq;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class ImportedModelTests
    {
        [Test]
        public void RigidAttachmentUsesInverseSourceThenHeadMatrix()
        {
            var source = new ModelMesh {transform=new float[] {1,0,0,0,0,1,0,0,0,0,1,0,100,0,0,1},
                positions=new[] {new Vector3(102,0,0)},physique=new ModelPhysique[0]};
            var head = Matrix4x4.TRS(new Vector3(0,10,0),Quaternion.Euler(0,0,90),Vector3.one);
            Assert.That(Vector3.Distance(LegacyModelGeometry.AttachedPositions(source,head)[0],new Vector3(0,0.012f,0)),Is.LessThan(1e-7f));
            source.transform=new float[16];
            Assert.Throws<InvalidDataException>(()=>LegacyModelGeometry.AttachedPositions(source,head));
        }

        [Test]
        public void UnskinnedSourceVerticesAreAlreadyWorldSpace()
        {
            var mesh = new ModelMesh { transform = new float[] {1,0,0,0, 0,1,0,0, 0,0,1,0, 100,200,300,1},
                positions = new[] {new Vector3(3,4,5)}, physique = new ModelPhysique[0] };
            Assert.That(Vector3.Distance(LegacyModelGeometry.Positions(mesh, new Dictionary<uint, Matrix4x4>())[0],
                new Vector3(0.003f,0.004f,0.005f)), Is.LessThan(0.00000001f));
        }
        [Test]
        public void SkinUsesPerInfluenceOffsetsAndLegacyMatrixLayout()
        {
            var identity = new float[] {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            var moved = (float[])identity.Clone(); moved[12] = 10;
            var mesh = new ModelMesh { transform = identity, positions = new[] { new Vector3(999,999,999) },
                physique = new[] { new ModelPhysique { influences = new[] {
                    new ModelInfluence { boneIndex = 1, weight = 0.25f, offset = new Vector3(2,0,0) },
                    new ModelInfluence { boneIndex = 2, weight = 0.75f, offset = new Vector3(4,0,0) }
                } } } };
            var bones = new Dictionary<uint, Matrix4x4> { {1, LegacyModelGeometry.Matrix(identity)}, {2, LegacyModelGeometry.Matrix(moved)} };
            Assert.That(LegacyModelGeometry.Positions(mesh, bones)[0].x, Is.EqualTo(0.011f).Within(0.000001f));
            bones.Remove(2);
            Assert.Throws<InvalidDataException>(() => LegacyModelGeometry.Positions(mesh, bones));
        }

        [TestCase("m_body01", 355, 96)]
        [TestCase("m_face01", 118, 0)]
        [TestCase("m_hair01", 212, 96)]
        [TestCase("m_hand01", 94, 96)]
        [TestCase("m_shoes01", 68, 96)]
        public void RealPartImportsMeshTextureAndRetainsSourceSkin(string name, int vertices, int bones)
        {
            string path = "Assets/Moxiang/Derived/Man/" + name + ".mxhmodel";
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            Assert.That(prefab, Is.Not.Null);
            var data = AssetDatabase.LoadAllAssetsAtPath(path).OfType<ImportedModel>().Single();
            Assert.That(data.descriptor.bones.Length, Is.EqualTo(bones));
            Assert.That(data.descriptor.releaseReady, Is.False);
            var filter = prefab.GetComponentInChildren<MeshFilter>();
            Assert.That(filter.sharedMesh.vertexCount, Is.EqualTo(vertices));
            Assert.That(filter.sharedMesh.normals.Length, Is.EqualTo(vertices));
            foreach (var material in prefab.GetComponentInChildren<MeshRenderer>().sharedMaterials)
                Assert.That(material.GetTexture("_BaseMap"), Is.Not.Null);
        }
    }
}
