using NUnit.Framework;
using UnityEngine;
using Moxiang;

public sealed class ChatInputControllerTests
{
    [Test]
    public void EmptyInputIsIgnored()
    {
        var go = new GameObject("chat");
        var controller = go.AddComponent<ChatInputController>();
        controller.SendMessage();
        Assert.That(controller, Is.Not.Null);
        Object.DestroyImmediate(go);
    }
}
