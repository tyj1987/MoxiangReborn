using NUnit.Framework;
using UnityEditor;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class MapVisualControllerTests
    {
        [Test]
        public void SavedConnectionSceneHasNoActiveFixedTerrain()
        {
            var scene = UnityEditor.SceneManagement.EditorSceneManager.OpenScene(
                "Assets/Moxiang/Scenes/ConnectionValidation.unity",
                UnityEditor.SceneManagement.OpenSceneMode.Additive);
            try
            {
                int fixedTerrainCount = 0;
                foreach (var root in scene.GetRootGameObjects())
                {
                    var source = PrefabUtility.GetCorrespondingObjectFromSource(root);
                    string path = AssetDatabase.GetAssetPath(source);
                    if (!path.EndsWith(".mxhterrain")) continue;
                    fixedTerrainCount++;
                    Assert.That(root.activeSelf, Is.False, "Fixed terrain must not survive runtime map switches: " + path);
                }
                Assert.That(fixedTerrainCount, Is.GreaterThan(0), "Expected the preserved inspection terrain instance.");
            }
            finally { UnityEditor.SceneManagement.EditorSceneManager.CloseScene(scene, true); }
        }

        [Test]
        public void ImportedMap10AndMap2SwitchTheirActualTerrainAndClickMesh()
        {
            var owner = new GameObject("ImportedMapSwitchTest");
            try
            {
                var controller = owner.AddComponent<MapVisualController>();
                controller.movement = owner.AddComponent<Map10InputController>();
                MapVisualController.Entry Load(ushort number)
                {
                    string root = "Assets/Moxiang/Derived/Map" + number + "/Map" + number;
                    return new MapVisualController.Entry { mapNumber = number,
                        prefab = AssetDatabase.LoadAssetAtPath<GameObject>(root + ".mxhterrain"),
                        heightField = AssetDatabase.LoadAssetAtPath<ImportedHeightField>(root + ".mxhasset") };
                }
                controller.maps = new[] { Load(10), Load(2) };
                var snapshot = new CoreSnapshot { state = CoreState.InGame, sessionGeneration = 1, mapGeneration = 1,
                    game = new CoreGame { mapNumber = 10 } };
                controller.Observe(snapshot);
                Assert.That(controller.IsReady, Is.True);
                var previous = controller.ActiveRoot;
                snapshot.game.mapNumber = 2; snapshot.mapGeneration = 2;
                controller.Observe(snapshot);
                Assert.That(controller.IsReady, Is.True);
                Assert.That(previous == null, Is.True);
                Assert.That(controller.ActiveRoot.name, Is.EqualTo(controller.maps[1].prefab.name + "(Clone)"));
                Assert.That(controller.ActiveRoot.transform.childCount, Is.EqualTo(64));
                Assert.That(((MeshCollider)controller.movement.mapCollider).sharedMesh,
                    Is.SameAs(controller.maps[1].heightField.inspectionMesh));
            }
            finally { Object.DestroyImmediate(owner); }
        }

        [Test]
        public void MapGenerationSwitchReplacesColliderAndMissingDestinationClearsOldWorld()
        {
            var owner = new GameObject("MapVisualTest");
            var prefab = new GameObject("TestVisualPrefab");
            try
            {
                var controller = owner.AddComponent<MapVisualController>();
                controller.movement = owner.AddComponent<Map10InputController>();
                var field = AssetDatabase.LoadAssetAtPath<ImportedHeightField>("Assets/Moxiang/Derived/Map10/Map10.mxhasset");
                controller.maps = new[] { new MapVisualController.Entry { mapNumber = 10, prefab = prefab, heightField = field } };
                var snapshot = new CoreSnapshot { state = CoreState.InGame, sessionGeneration = 1, mapGeneration = 1,
                    game = new CoreGame { mapNumber = 10 } };
                controller.Observe(snapshot);
                Assert.That(controller.IsReady, Is.True);
                var first = controller.ActiveRoot;
                Assert.That(controller.movement.mapDepth, Is.EqualTo(field.descriptor.depth));
                Assert.That(controller.movement.mapCollider, Is.Not.Null);
                controller.Observe(snapshot);
                Assert.That(controller.ActiveRoot, Is.SameAs(first));
                snapshot.mapGeneration = 2;
                controller.Observe(snapshot);
                Assert.That(first == null, Is.True);
                Assert.That(controller.IsReady, Is.True);
                snapshot.game.mapNumber = 2; snapshot.mapGeneration = 3;
                controller.Observe(snapshot);
                Assert.That(controller.IsReady, Is.False);
                Assert.That(controller.ActiveRoot, Is.Null);
                Assert.That(controller.movement.mapCollider, Is.Null);
                snapshot.game.mapNumber = 10; snapshot.mapGeneration = 4;
                controller.Observe(snapshot);
                Assert.That(controller.IsReady, Is.True);
                snapshot.state = CoreState.Idle;
                controller.Observe(snapshot);
                Assert.That(controller.ActiveRoot, Is.Null);
                Assert.That(controller.IsReady, Is.False);
            }
            finally { Object.DestroyImmediate(owner); Object.DestroyImmediate(prefab); }
        }
    }
}
