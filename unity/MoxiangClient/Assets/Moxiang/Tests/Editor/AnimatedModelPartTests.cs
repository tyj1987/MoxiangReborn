using NUnit.Framework;
using UnityEditor;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class AnimatedModelPartTests
    {
        [Test]
        public void IndependentInstancesNeverDeformSharedSourceAndReleaseTheirMeshes()
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Derived/Man/m_body01.mxhmodel");
            var motion = AssetDatabase.LoadAssetAtPath<ImportedMotion>("Assets/Moxiang/Derived/Man/Motions/m001.mxhmotion");
            var sampler = new LegacyMotionSampler(motion.descriptor);
            var source = prefab.GetComponentInChildren<MeshFilter>().sharedMesh;
            var before = source.vertices;
            GameObject a=null,b=null;
            try {
                a=Object.Instantiate(prefab); b=Object.Instantiate(prefab);
                a.GetComponent<AnimatedModelPart>().SampleFrame(sampler,0);
                b.GetComponent<AnimatedModelPart>().SampleFrame(sampler,30);
                var meshA=a.GetComponentInChildren<MeshFilter>().sharedMesh;
                var meshB=b.GetComponentInChildren<MeshFilter>().sharedMesh;
                Assert.That(meshA,Is.Not.SameAs(source)); Assert.That(meshB,Is.Not.SameAs(source));
                Assert.That(meshA,Is.Not.SameAs(meshB));
                CollectionAssert.AreEqual(before,source.vertices);
                var poseB=meshB.vertices;
                a.GetComponent<AnimatedModelPart>().SampleFrame(sampler,59);
                CollectionAssert.AreEqual(poseB,meshB.vertices);
                var component=a.GetComponent<AnimatedModelPart>();
                component.enabled=false;
                Assert.That(meshA==null,Is.True);
                component.enabled=true;
                component.SampleFrame(sampler,10);
                meshA=a.GetComponentInChildren<MeshFilter>().sharedMesh;
                Object.DestroyImmediate(a.GetComponent<AnimatedModelPart>());
                Assert.That(meshA==null,Is.True);
                Assert.That(a.GetComponentInChildren<MeshFilter>().sharedMesh,Is.SameAs(source));
                Object.DestroyImmediate(b); b=null;
                Assert.That(meshB==null,Is.True);
            }
            finally { if(a!=null)Object.DestroyImmediate(a); if(b!=null)Object.DestroyImmediate(b); }
        }
    }
}
