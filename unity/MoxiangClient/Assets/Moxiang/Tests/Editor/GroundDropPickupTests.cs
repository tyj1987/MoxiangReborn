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
}
