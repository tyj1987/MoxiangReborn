using NUnit.Framework;
using UnityEngine;

namespace Moxiang.Tests
{
    public class Map10InputControllerTests
    {
        [Test]
        public void Map10CenterConvertsToLegacyCenter()
        {
            var map = new MapCoordinates(51200, 102400);
            Vector3 scene = map.ToScene(new Vector3(25600, 0, 51200));
            Vector3 game = map.ToGame(scene);
            Assert.That(game.x, Is.EqualTo(25600).Within(0.01));
            Assert.That(game.z, Is.EqualTo(51200).Within(0.01));
        }

        [Test]
        public void ControllerExposesMap10Defaults()
        {
            var go = new GameObject();
            try
            {
                var controller = go.AddComponent<Map10InputController>();
                Assert.That(controller.mapWidth, Is.EqualTo(51200));
                Assert.That(controller.mapDepth, Is.EqualTo(102400));
                Assert.That(controller.stopWithRightClick, Is.True);
            }
            finally { Object.DestroyImmediate(go); }
        }

        [Test]
        public void EntityClickSuppressesGroundMoveOnlyInFrontOfGround()
        {
            var entity = new GameObject("NPC click test");
            var child = new GameObject("NPC collider");
            try {
                entity.transform.position = new Vector3(2000, 2000, 2000);
                child.transform.SetParent(entity.transform, false);
                child.AddComponent<BoxCollider>();
                var identity = entity.AddComponent<TargetSelectable>();
                identity.objectId = 77;
                identity.isNpc = true;
                Physics.SyncTransforms();
                var ray = new Ray(entity.transform.position + Vector3.back * 5, Vector3.forward);
                Assert.That(Map10InputController.EntityOwnsClick(ray, 10), Is.True);
                Assert.That(Map10InputController.EntityOwnsClick(ray, 2), Is.False);
                identity.objectId = 0;
                Assert.That(Map10InputController.EntityOwnsClick(ray, 10), Is.False);
            }
            finally { Object.DestroyImmediate(entity); }
        }
    }
}
