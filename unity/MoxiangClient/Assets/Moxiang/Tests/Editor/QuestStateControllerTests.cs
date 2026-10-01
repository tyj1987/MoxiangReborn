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

    [Test]
    public void RetainsMultipleServerQuestUpdates()
    {
        var go = new GameObject("quest");
        var state = go.AddComponent<QuestStateController>();
        state.ApplyQuestUpdate(57, 6);
        state.ApplyQuestUpdate(60, 6);
        Assert.That(state.ActiveQuests.Count, Is.EqualTo(2));
        Assert.That(state.ActiveQuests[57], Is.EqualTo(6u));
        Assert.That(state.QuestId, Is.EqualTo(60u));
        state.Clear();
        Assert.That(state.ActiveQuests, Is.Empty);
        Object.DestroyImmediate(go);
    }
}
