using NUnit.Framework;
using UnityEngine;
using Moxiang;

public sealed class ServerEntityHealthTests
{
    [Test]
    public void StoresAuthoritativeLifeWithoutClientSideDamageRules()
    {
        var go = new GameObject("entity");
        var health = go.AddComponent<ServerEntityHealth>();
        health.SetCurrentLife(321u);
        Assert.That(health.CurrentLife, Is.EqualTo(321u));
        health.SetCurrentLife(0u);
        Assert.That(health.CurrentLife, Is.EqualTo(0u));
        health.SetCurrentShield(17u);
        Assert.That(health.CurrentShield, Is.EqualTo(17u));
        Object.DestroyImmediate(go);
    }
}
