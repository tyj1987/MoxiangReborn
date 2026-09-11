using NUnit.Framework;
using UnityEngine;

namespace Moxiang.Tests
{
    public sealed class SkillHotkeyControllerTests
    {
        [Test]
        public void ResolvesOnlyBoundNonZeroSkill()
        {
            var keys = new[] { KeyCode.Alpha1, KeyCode.Alpha2 };
            var ids = new uint[] { 401, 0 };
            Assert.That(SkillHotkeyController.TryResolveSkill(KeyCode.Alpha1, keys, ids, out var id), Is.True);
            Assert.That(id, Is.EqualTo(401u));
            Assert.That(SkillHotkeyController.TryResolveSkill(KeyCode.Alpha2, keys, ids, out _), Is.False);
        }

        [Test]
        public void MismatchedArraysAndUnknownKeysAreSafe()
        {
            Assert.That(SkillHotkeyController.TryResolveSkill(KeyCode.Alpha1,
                new[] { KeyCode.Alpha1 }, new uint[] { 9, 10 }, out var id), Is.True);
            Assert.That(id, Is.EqualTo(9u));
            Assert.That(SkillHotkeyController.TryResolveSkill(KeyCode.Alpha9,
                new[] { KeyCode.Alpha1 }, new uint[] { 9 }, out _), Is.False);
            Assert.That(SkillHotkeyController.TryResolveSkill(KeyCode.Alpha1, null, null, out _), Is.False);
        }
    }
}
