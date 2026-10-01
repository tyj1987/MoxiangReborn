using NUnit.Framework;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class GroundDropLabelTests
    {
        [TestCase(600, 5f)]
        [TestCase(600, 38.08341f)]
        [TestCase(600, 71.98346f)]
        [TestCase(768, 150f)]
        [TestCase(1080, 5f)]
        [TestCase(1440, 72f)]
        [TestCase(2160, 299f)]
        public void PerspectiveLabelKeepsReadablePixelSize(int height, float depth)
        {
            float scale = GroundDropLabel.WorldUnitsPerPixel(depth, 60, height, false, 5);
            float pixelsPerUnit = height / (2f * depth * Mathf.Tan(30 * Mathf.Deg2Rad));
            Assert.That(160 * scale * pixelsPerUnit, Is.EqualTo(160).Within(.001));
            Assert.That(36 * scale * pixelsPerUnit, Is.EqualTo(36).Within(.001));
            Assert.That(17 * scale * pixelsPerUnit, Is.EqualTo(17).Within(.001));
        }

        [TestCase(600, 5f)]
        [TestCase(1440, 72f)]
        public void OrthographicSizeIsIndependentOfDistance(int height, float size)
        {
            float near = GroundDropLabel.WorldUnitsPerPixel(5, 60, height, true, size);
            float far = GroundDropLabel.WorldUnitsPerPixel(200, 60, height, true, size);
            Assert.That(near, Is.EqualTo(far));
            Assert.That(36 * near * height / (2 * size), Is.EqualTo(36).Within(.001));
        }

        [Test]
        public void InvalidProjectionCannotProduceAnInteractiveScale()
        {
            Assert.That(GroundDropLabel.WorldUnitsPerPixel(0, 60, 600, false, 5), Is.Zero);
            Assert.That(GroundDropLabel.WorldUnitsPerPixel(-1, 60, 600, false, 5), Is.Zero);
            Assert.That(GroundDropLabel.WorldUnitsPerPixel(float.NaN, 60, 600, false, 5), Is.Zero);
            Assert.That(GroundDropLabel.WorldUnitsPerPixel(50, 180, 600, false, 5), Is.Zero);
            Assert.That(GroundDropLabel.WorldUnitsPerPixel(50, 60, 0, false, 5), Is.Zero);
            Assert.That(GroundDropLabel.WorldUnitsPerPixel(50, 60, 600, true, -1), Is.Zero);
        }

        [Test]
        public void OcclusionIgnoresOwnedColliderButNotAnInterveningObject()
        {
            var cameraObject = new GameObject("pickup-test-camera");
            var owner = new GameObject("drop", typeof(BoxCollider));
            var obstacle = new GameObject("occluder", typeof(BoxCollider));
            try
            {
                var origin = new Vector3(1000, 1000, 1000);
                cameraObject.transform.position = origin;
                var camera = cameraObject.AddComponent<Camera>();
                var anchor = origin + Vector3.forward * 10;
                owner.transform.position = anchor;
                obstacle.transform.position = origin + Vector3.forward * 5;
                Physics.SyncTransforms();
                Assert.That(GroundDropLabel.IsOccluded(camera, anchor, owner.transform), Is.True);
                obstacle.transform.position += Vector3.right * 10;
                Physics.SyncTransforms();
                Assert.That(GroundDropLabel.IsOccluded(camera, anchor, owner.transform), Is.False);
            }
            finally
            {
                Object.DestroyImmediate(cameraObject);
                Object.DestroyImmediate(owner);
                Object.DestroyImmediate(obstacle);
            }
        }
    }
}
