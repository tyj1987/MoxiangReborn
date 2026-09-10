using System;
using NUnit.Framework;
using UnityEngine;

namespace Moxiang.Tests
{
    public class MapCoordinatesTests
    {
        [Test]
        public void EachMapUsesItsActualCenter()
        {
            var map = new MapCoordinates(50000, 72000);
            Assert.That(map.ToScene(new Vector3(25000, 0, 36000)), Is.EqualTo(Vector3.zero));
            Assert.That(map.ToScene(new Vector3(0, 1000, 0)).y, Is.EqualTo(1f).Within(0.00001f));
        }

        [Test]
        public void ClickRoundTripKeepsGameplayCoordinates()
        {
            var map = new MapCoordinates(51200, 102400);
            var point = new Vector3(12345, 875, 73520);
            Assert.That(Vector3.Distance(map.ToGame(map.ToScene(point)), point), Is.LessThan(0.01f));
        }

        [Test]
        public void RejectsInvalidMapAndPosition()
        {
            Assert.Throws<ArgumentOutOfRangeException>(() => new MapCoordinates(float.NaN, 100));
            Assert.Throws<ArgumentOutOfRangeException>(() => new MapCoordinates(0, 100));
            Assert.Throws<ArgumentOutOfRangeException>(() => new MapCoordinates(100, 100).ToGame(new Vector3(float.PositiveInfinity, 0, 0)));
        }
    }
}
