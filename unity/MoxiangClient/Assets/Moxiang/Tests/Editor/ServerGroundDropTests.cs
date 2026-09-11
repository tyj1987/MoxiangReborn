using NUnit.Framework;
using UnityEngine;
using Moxiang;

public sealed class ServerGroundDropTests
{
    [Test]
    public void StoresServerIdentityAndCount()
    {
        var go = new GameObject("drop");
        var drop = go.AddComponent<ServerGroundDrop>();
        drop.Initialize(41u, 9001u, 3);
        Assert.That(drop.ObjectId, Is.EqualTo(41u));
        Assert.That(drop.ItemId, Is.EqualTo(9001u));
        Assert.That(drop.Count, Is.EqualTo((ushort)3));
        Object.DestroyImmediate(go);
    }
}
