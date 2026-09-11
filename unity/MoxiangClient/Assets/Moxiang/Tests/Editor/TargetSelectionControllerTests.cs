using NUnit.Framework;
using UnityEngine;
using Moxiang;

public sealed class TargetSelectionControllerTests
{
    [Test]
    public void RejectsMissingOrZeroIdentity()
    {
        var go = new GameObject("selector");
        var selector = go.AddComponent<TargetSelectionController>();
        Assert.That(selector.TryGetTarget(out var id, out _), Is.False);
        Assert.That(id, Is.EqualTo(0u));
        Object.DestroyImmediate(go);
    }

    [Test]
    public void ReturnsStableObjectIdentityAndPosition()
    {
        var targetGo = new GameObject("target");
        targetGo.transform.position = new Vector3(12, 0, 34);
        var target = targetGo.AddComponent<TargetSelectable>();
        target.objectId = 77;
        var selectorGo = new GameObject("selector");
        var selector = selectorGo.AddComponent<TargetSelectionController>();
        selector.selected = target;
        Assert.That(selector.TryGetTarget(out var id, out var position), Is.True);
        Assert.That(id, Is.EqualTo(77u));
        Assert.That(position, Is.EqualTo(new Vector2(12, 34)));
        Object.DestroyImmediate(selectorGo);
        Object.DestroyImmediate(targetGo);
    }
}
