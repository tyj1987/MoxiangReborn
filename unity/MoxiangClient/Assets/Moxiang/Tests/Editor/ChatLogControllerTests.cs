using NUnit.Framework;
using UnityEngine;
using Moxiang;

public sealed class ChatLogControllerTests
{
    [Test]
    public void StartsEmptyWithBoundedCapacity()
    {
        var go = new GameObject("chat-log");
        var log = go.AddComponent<ChatLogController>();
        Assert.That(log.Messages, Is.Empty);
        log.capacity = 0;
        Assert.That(log.capacity, Is.EqualTo(0));
        Object.DestroyImmediate(go);
    }
}
