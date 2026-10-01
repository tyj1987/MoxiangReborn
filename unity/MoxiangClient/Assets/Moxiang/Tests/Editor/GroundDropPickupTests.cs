using NUnit.Framework;
using UnityEngine;
using Moxiang;
using System.Reflection;
using UnityEditor;

public sealed class GroundDropPickupTests
{
    [Test]
    public void PickupComponentKeepsDropReference()
    {
        var go = new GameObject("drop");
        var state = go.AddComponent<ServerGroundDrop>();
        var pickup = go.AddComponent<GroundDropPickup>();
        pickup.drop = state;
        Assert.That(pickup.drop, Is.SameAs(state));
        Object.DestroyImmediate(go);
    }

    [Test]
    public void ConfirmationClearsPendingStateWithoutChangingDropIdentity()
    {
        var go = new GameObject("drop");
        var state = go.AddComponent<ServerGroundDrop>();
        state.Initialize(41u, 9001u, 1);
        var pickup = go.AddComponent<GroundDropPickup>();
        pickup.drop = state;
        typeof(GroundDropPickup).GetField("pending", BindingFlags.Instance | BindingFlags.NonPublic).SetValue(pickup, true);
        Assert.That(pickup.Pending, Is.True);
        pickup.Confirmed();
        Assert.That(pickup.Pending, Is.False);
        Assert.That(state.ObjectId, Is.EqualTo(41u));
        Object.DestroyImmediate(go);
    }

    [Test]
    public void InvalidOrAlreadyPendingPickupDoesNotSendAgain()
    {
        var go = new GameObject("drop");
        try
        {
            var pickup = go.AddComponent<GroundDropPickup>();
            Assert.That(pickup.RequestPickup(), Is.EqualTo(CoreResult.NotReady));
            typeof(GroundDropPickup).GetField("pending", BindingFlags.Instance | BindingFlags.NonPublic).SetValue(pickup, true);
            Assert.That(pickup.RequestPickup(), Is.EqualTo(CoreResult.NotReady));
            Assert.That(pickup.Pending, Is.True);
        }
        finally { Object.DestroyImmediate(go); }
    }

    [Test]
    public void CommittedDropPrefabHasClickableLabelAndFont()
    {
        var prefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Scenes/GroundDropLabel.prefab");
        Assert.That(prefab, Is.Not.Null);
        Assert.That(prefab.GetComponent<Collider>(), Is.Not.Null);
        Assert.That(prefab.GetComponent<GroundDropLabel>().font, Is.Not.Null);
        Assert.That(prefab.GetComponent<GroundDropPickup>().drop, Is.SameAs(prefab.GetComponent<ServerGroundDrop>()));
        Assert.That(AssetDatabase.TryGetGUIDAndLocalFileIdentifier(prefab, out string guid, out long localId), Is.True);
        Assert.That(guid, Is.EqualTo("9418879f64c84d3bbcef91b20797458a"));
        Assert.That(localId, Is.Not.EqualTo(100100000L), "The prefab asset's reserved ID cannot identify its root GameObject.");
        var scene = System.IO.File.ReadAllText("Assets/Moxiang/Scenes/ConnectionValidation.unity");
        Assert.That(scene, Does.Contain($"groundDropPrefab: {{fileID: {localId}, guid: {guid}, type: 3}}"));
        var instance = (GameObject)PrefabUtility.InstantiatePrefab(prefab);
        try
        {
            Assert.That(instance, Is.Not.Null);
            Assert.That(PrefabUtility.GetCorrespondingObjectFromSource(instance), Is.SameAs(prefab));
            Assert.That(instance.GetComponent<GroundDropPickup>().drop, Is.SameAs(instance.GetComponent<ServerGroundDrop>()));
        }
        finally { if (instance != null) Object.DestroyImmediate(instance); }
    }

    [Test]
    public void MissingReplyTimesOutWithoutDeletingTheDrop()
    {
        var go = new GameObject("drop");
        try
        {
            var pickup = go.AddComponent<GroundDropPickup>();
            var flags = BindingFlags.Instance | BindingFlags.NonPublic;
            typeof(GroundDropPickup).GetField("pending", flags).SetValue(pickup, true);
            typeof(GroundDropPickup).GetField("pendingSince", flags).SetValue(pickup, Time.unscaledTime - 6f);
            typeof(GroundDropPickup).GetMethod("Update", flags).Invoke(pickup, null);
            Assert.That(pickup.Pending, Is.False);
            Assert.That(go.activeSelf, Is.True);
        }
        finally { Object.DestroyImmediate(go); }
    }
}
