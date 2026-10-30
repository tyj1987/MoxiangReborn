using System.Collections.Generic;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class CharacterAnimationTests
    {
        [Test]
        public void RealCompositeUsesClockFrameForBodyAndAttachedFace()
        {
            var owner=new GameObject("CharacterAnimationTest");
            try {
                AnimatedModelPart Part(string name) => Object.Instantiate(AssetDatabase.LoadAssetAtPath<GameObject>(
                    "Assets/Moxiang/Derived/Man/"+name+".mxhmodel"),owner.transform).GetComponent<AnimatedModelPart>();
                var body=Part("m_body01"); var face=Part("m_face01");
                var animation=owner.AddComponent<CharacterAnimation>();
                animation.skinnedParts=new[] {body,Part("m_hair01"),Part("m_hand01"),Part("m_shoes01")};
                animation.face=face; animation.headSkeleton=body;
                animation.motions=new[] {new CharacterMotion {motionNumber=1,source=AssetDatabase.LoadAssetAtPath<ImportedMotion>(
                    "Assets/Moxiang/Derived/Man/Motions/m001.mxhmotion")}};
                animation.Advance(30);
                Assert.That(animation.CurrentFrame,Is.EqualTo(30));
                var sampler=new LegacyMotionSampler(animation.motions[0].source.descriptor);
                uint index=0;foreach(var bone in body.source.descriptor.bones)if(bone.name=="Bip01 Head")index=bone.index;
                var expected=LegacyModelGeometry.AttachedPositions(face.source.descriptor.meshes[0],sampler.Sample(body.source.descriptor.bones,30)[index]);
                CollectionAssert.AreEqual(expected,face.GetComponentInChildren<MeshFilter>().sharedMesh.vertices);
                animation.ChangeMotion(1);Assert.That(animation.CurrentFrame,Is.EqualTo(30));
                animation.Advance(30);Assert.That(animation.CurrentFrame,Is.Zero);
            } finally {Object.DestroyImmediate(owner);}
        }
    }
}
