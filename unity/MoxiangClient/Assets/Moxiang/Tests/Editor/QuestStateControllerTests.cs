using NUnit.Framework;
using UnityEngine;
using Moxiang;

public sealed class QuestStateControllerTests
{
    [Test]
    public void StartsWithNoClientSideQuestState()
    {
        var go = new GameObject("quest");
        var state = go.AddComponent<QuestStateController>();
        Assert.That(state.QuestId, Is.EqualTo(0u));
        Assert.That(state.State, Is.EqualTo(0u));
        Object.DestroyImmediate(go);
    }
}
