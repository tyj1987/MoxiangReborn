using NUnit.Framework;
using UnityEditor;
using UnityEngine;

namespace Moxiang.Tests
{
    public class ImportedMap10Tests
    {
        [Test]
        public void PackedMap2ImportsOriginalGridAndCanonicalArrivalWithinBounds()
        {
            var asset = AssetDatabase.LoadAssetAtPath<ImportedHeightField>("Assets/Moxiang/Derived/Map2/Map2.mxhasset");
            Assert.That(asset, Is.Not.Null);
            Assert.That(asset.descriptor.sourceSha256, Is.EqualTo("d49dd4b864f09e3880124d31633d1b0860175805a3273284988c2b421d9e31e2"));
            Assert.That(asset.heights.Length, Is.EqualTo(513 * 513));
            Assert.That(asset.descriptor.tiles.Length, Is.EqualTo(256 * 256));
            Assert.That(asset.inspectionMesh.vertexCount, Is.EqualTo(asset.heights.Length));
            var arrival = new MapCoordinates(asset.descriptor.width, asset.descriptor.depth)
                .ToScene(new Vector3(7211, 0, 43329));
            var bounds = asset.inspectionMesh.bounds;
            Assert.That(arrival.x, Is.InRange(bounds.min.x, bounds.max.x));
            Assert.That(arrival.z, Is.InRange(bounds.min.z, bounds.max.z));
            Assert.That(asset.IsReleaseReady, Is.False);
        }

        [Test]
        public void PackedMap10RetainsHeightGridAndSourceIdentity()
        {
            var asset = AssetDatabase.LoadAssetAtPath<ImportedHeightField>("Assets/Moxiang/Derived/Map10/Map10.mxhasset");
            Assert.That(asset, Is.Not.Null);
            Assert.That(asset.descriptor.sourceSha256, Is.EqualTo("a774fc78a439afe0b435b88a562ede471af013d731b7f10c1c1f2a402e2aa454"));
            Assert.That(asset.heights.Length, Is.EqualTo(513 * 513));
            Assert.That(asset.descriptor.tiles.Length, Is.EqualTo(256 * 256));
            Assert.That(asset.descriptor.textureNames.Length, Is.EqualTo(37));
            Assert.That(asset.IsReleaseReady, Is.False);
        }

        [Test]
        public void PackedMap10BoundsUseGameCoordinatesAndUpwardWinding()
        {
            var asset = AssetDatabase.LoadAssetAtPath<ImportedHeightField>("Assets/Moxiang/Derived/Map10/Map10.mxhasset");
            Assert.That(asset, Is.Not.Null);
            var mesh = asset.inspectionMesh;
            Assert.That(mesh.bounds.min.x, Is.EqualTo(-25.6f).Within(0.0001f));
            Assert.That(mesh.bounds.max.x, Is.EqualTo(25.6f).Within(0.0001f));
            Assert.That(mesh.bounds.min.z, Is.EqualTo(-25.6f).Within(0.0001f));
            Assert.That(mesh.bounds.max.z, Is.EqualTo(25.6f).Within(0.0001f));
            Assert.That(mesh.vertexCount, Is.EqualTo(asset.heights.Length));
            foreach (var normal in mesh.normals) Assert.That(normal.y, Is.GreaterThanOrEqualTo(0));
        }
    }
}
