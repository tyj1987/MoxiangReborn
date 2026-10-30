using System;
using System.IO;
using System.Linq;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class ImportedMotionTests
    {
        [Serializable] sealed class MathCase { public float[] a, b, quaternion, matrix; public float alpha; }
        [Serializable] sealed class MathFixture { public MathCase[] cases; }
        static ModelBone Bone(uint id, uint parent, string name, Vector3 position) => new ModelBone {
            index = id, parentIndex = parent, name = name, position = position,
            scale = Vector3.one, rotationAxis = Vector3.up };
        static MotionDescriptor Motion(params MotionTrack[] tracks) => new MotionDescriptor {
            schemaVersion = 1, kind = "motion", lastFrame = 10, objects = tracks };

        [Test]
        public void InterpolationComposesParentBeforeChildAndClampsEndpoints()
        {
            var sampler = new LegacyMotionSampler(Motion(new MotionTrack { name = "root", positions = new[] {
                new MotionVectorKey { frame = 0, value = Vector3.zero },
                new MotionVectorKey { frame = 10, value = new Vector3(10,0,0) } }, rotations = new[] {
                new MotionRotationKey { frame = 0, value = new Quaternion(0,0,Mathf.Sqrt(0.5f),Mathf.Sqrt(0.5f)) },
                new MotionRotationKey { frame = 10, value = new Quaternion(0,0,Mathf.Sqrt(0.5f),Mathf.Sqrt(0.5f)) } } }));
            var bones = new[] { Bone(2,1,"child",Vector3.right), Bone(1,uint.MaxValue,"root",Vector3.zero) };
            var point = sampler.Sample(bones, 5)[2].MultiplyPoint3x4(Vector3.zero);
            Assert.That(Vector3.Distance(point, new Vector3(5,-1,0)), Is.LessThan(0.00001f));
            Assert.That(Vector3.Distance(sampler.Sample(bones,-10)[2].MultiplyPoint3x4(Vector3.zero), Vector3.down), Is.LessThan(0.00001f));
            Assert.That(Vector3.Distance(sampler.Sample(bones,20)[2].MultiplyPoint3x4(Vector3.zero), new Vector3(10,-1,0)), Is.LessThan(0.00001f));
        }

        [Test]
        public void RotationMatchesOriginalX86DllOracle()
        {
            var fixture = JsonUtility.FromJson<MathFixture>(File.ReadAllText("Assets/Moxiang/Tests/Editor/legacy-motion-math.json"));
            Assert.That(fixture.cases.Length, Is.EqualTo(36));
            foreach (var value in fixture.cases) {
                Quaternion Q(float[] v) => new Quaternion(v[0],v[1],v[2],v[3]);
                var sampler = new LegacyMotionSampler(Motion(new MotionTrack {name="root",rotations=new[] {
                    new MotionRotationKey {frame=0,value=Q(value.a)},
                    new MotionRotationKey {frame=10,value=Q(value.b)} } }));
                var actual = sampler.Sample(new[] {Bone(1,uint.MaxValue,"root",Vector3.zero)},value.alpha*10)[1];
                var expected = LegacyModelGeometry.Matrix(value.matrix);
                for (int row=0;row<4;++row) for(int col=0;col<4;++col)
                    Assert.That(actual[row,col],Is.EqualTo(expected[row,col]).Within(0.00001f),
                        "Original DLL matrix mismatch at " + row + "," + col);
            }
        }

        [Test]
        public void OppositeQuaternionSignsFollowTheSameRotation()
        {
            var q = new Quaternion(0,0,Mathf.Sqrt(0.5f),Mathf.Sqrt(0.5f));
            var sampler = new LegacyMotionSampler(Motion(new MotionTrack { name="root", rotations=new[] {
                new MotionRotationKey {frame=0,value=q},
                new MotionRotationKey {frame=10,value=new Quaternion(-q.x,-q.y,-q.z,-q.w)} } }));
            var result = sampler.Sample(new[] {Bone(1,uint.MaxValue,"root",Vector3.zero)},5)[1].MultiplyPoint3x4(Vector3.right);
            Assert.That(Vector3.Distance(result,Vector3.down),Is.LessThan(0.00001f));
        }

        [Test]
        public void InvalidHierarchyAndUnsupportedTracksFailExplicitly()
        {
            var sampler = new LegacyMotionSampler(Motion());
            Assert.Throws<InvalidDataException>(() => sampler.Sample(new[] {Bone(1,2,"a",Vector3.zero)},0));
            Assert.Throws<InvalidDataException>(() => sampler.Sample(new[] {Bone(1,2,"a",Vector3.zero),Bone(2,1,"b",Vector3.zero)},0));
            Assert.Throws<InvalidDataException>(() => sampler.Sample(Array.Empty<ModelBone>(),float.NaN));
            Assert.Throws<NotSupportedException>(() => new LegacyMotionSampler(Motion(new MotionTrack {name="mesh",meshAnimationKeyCount=1})));
            Assert.Throws<NotSupportedException>(() => new LegacyMotionSampler(Motion(new MotionTrack {name="scale",scales=new[] {new MotionVectorKey()}})));
        }

        [Test]
        public void RealMotionImportsAndDeformsBodyAtSourceFrames()
        {
            var motion = AssetDatabase.LoadAssetAtPath<ImportedMotion>("Assets/Moxiang/Derived/Man/Motions/m001.mxhmotion");
            Assert.That(motion, Is.Not.Null);
            Assert.That(motion.descriptor.objects.Length, Is.EqualTo(116));
            Assert.That(motion.descriptor.releaseReady, Is.False);
            var model = AssetDatabase.LoadAllAssetsAtPath("Assets/Moxiang/Derived/Man/m_body01.mxhmodel").OfType<ImportedModel>().Single().descriptor;
            var sampler = new LegacyMotionSampler(motion.descriptor);
            var first = LegacyModelGeometry.Positions(model.meshes[0],sampler.Sample(model.bones,0));
            var middle = LegacyModelGeometry.Positions(model.meshes[0],sampler.Sample(model.bones,30));
            var last = LegacyModelGeometry.Positions(model.meshes[0],sampler.Sample(model.bones,59));
            Assert.That(first.Length, Is.EqualTo(355));
            Assert.That(first.Where((v,i)=>Vector3.Distance(v,middle[i]) > 0.000001f).Count(), Is.GreaterThan(0));
            Assert.That(last.All(v=>float.IsFinite(v.x)&&float.IsFinite(v.y)&&float.IsFinite(v.z)), Is.True);
        }
    }
}
