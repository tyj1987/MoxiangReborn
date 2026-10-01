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
        var registry = targetGo.AddComponent<ServerEntityRegistry>();
        registry.mapWidth = 51200;
        registry.mapDepth = 102400;
        targetGo.transform.position = new MapCoordinates(registry.mapWidth, registry.mapDepth).ToScene(new Vector3(26175, 0, 25425));
        var target = targetGo.AddComponent<TargetSelectable>();
        target.objectId = 77;
        var selectorGo = new GameObject("selector");
        var selector = selectorGo.AddComponent<TargetSelectionController>();
        selector.selected = target;
        Assert.That(selector.TryGetTarget(out var id, out var position), Is.True);
        Assert.That(id, Is.EqualTo(77u));
        Assert.That(position.x, Is.EqualTo(26175).Within(0.01f));
        Assert.That(position.y, Is.EqualTo(25425).Within(0.01f));
        targetGo.SetActive(false);
        Assert.That(selector.TryGetTarget(out id, out _), Is.False);
        Assert.That(id, Is.Zero);
        Object.DestroyImmediate(selectorGo);
        Object.DestroyImmediate(targetGo);
    }

    [Test]
    public void SelectUsesMaterializedEntityAndRaisesOfficialSelectionEvent()
    {
        var registryGo = new GameObject("registry");
        var registry = registryGo.AddComponent<ServerEntityRegistry>();
        var targetGo = new GameObject("npc");
        targetGo.transform.SetParent(registryGo.transform, false);
        targetGo.transform.position = new MapCoordinates(registry.mapWidth, registry.mapDepth).ToScene(new Vector3(3500, 0, 46900));
        var target = targetGo.AddComponent<TargetSelectable>();
        target.objectId = 572; target.isNpc = true;
        var selectorGo = new GameObject("selector");
        var selector = selectorGo.AddComponent<TargetSelectionController>();
        uint observedId = 0;
        selector.TargetSelected += (id, _) => observedId = id;

        Assert.That(selector.Select(target), Is.True);
        Assert.That(selector.selected, Is.SameAs(target));
        Assert.That(observedId, Is.EqualTo(572));

        Object.DestroyImmediate(selectorGo);
        Object.DestroyImmediate(registryGo);
    }
}
