using System.Reflection;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class ServerMonsterAnimationTests
    {
        [Test]
        public void OriginalMonsterMotionDeformsMeshWithoutDrivingAuthoritativeRoot()
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Derived/Monster/M073/l073_lod2.mxhmodel");
            Assert.That(prefab, Is.Not.Null);
            var instance = Object.Instantiate(prefab, new Vector3(12, 3, 45), Quaternion.identity);
            var driverGo = new GameObject("driver");
            var driver = driverGo.AddComponent<LegacyAnimationDriver>();
            var motions = new ImportedMotion[12];
            for (int i = 0; i < motions.Length; ++i)
            {
                motions[i] = AssetDatabase.LoadAssetAtPath<ImportedMotion>($"Assets/Moxiang/Derived/Monster/M073/Motions/l073_{i + 1:00}.mxhmotion");
                Assert.That(motions[i], Is.Not.Null);
            }
            var animation = instance.AddComponent<ServerMonsterAnimation>();
            animation.Initialize(driver, motions);
            var root = instance.transform.position;
            var advance = typeof(ServerMonsterAnimation).GetMethod("Advance", BindingFlags.Instance | BindingFlags.NonPublic);

            animation.PlayMove();
            advance.Invoke(animation, new object[] { 5u });
            Assert.That(animation.CurrentMotion, Is.EqualTo(ServerMonsterAnimation.MoveMotion));
            Assert.That(instance.transform.position, Is.EqualTo(root));

            animation.PlayHit();
            Assert.That(animation.CurrentMotion, Is.EqualTo(ServerMonsterAnimation.HitMotion));
            advance.Invoke(animation, new object[] { 2u });
            Assert.That(instance.transform.position, Is.EqualTo(root));

            animation.PlayDeath();
            advance.Invoke(animation, new object[] { 1000u });
            Assert.That(instance.activeSelf, Is.False);
            Assert.That(instance.transform.position, Is.EqualTo(root));

            Object.DestroyImmediate(instance);
            Object.DestroyImmediate(driverGo);
        }
    }
}
