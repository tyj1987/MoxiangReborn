using NUnit.Framework;
using UnityEngine;
using Moxiang;

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
        pickup.Confirmed();
        Assert.That(state.ObjectId, Is.EqualTo(41u));
        Object.DestroyImmediate(go);
    }
}
