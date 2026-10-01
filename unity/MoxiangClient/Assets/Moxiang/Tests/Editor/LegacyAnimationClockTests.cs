using System.Collections.Generic;
using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class LegacyAnimationClockTests
    {
        static LegacyAnimationState State() => new LegacyAnimationState(new Dictionary<uint,LegacyMotionRange> {
            {1,new LegacyMotionRange(59)}, {2,new LegacyMotionRange(9,2,8)} },1);

        [Test]
        public void GlobalClockUsesIntegerMillisecondsAndUnsignedTickDifference()
        {
            var clock=new LegacyFrameClock(100,30);
            Assert.That(clock.Consume(132),Is.Zero);
            Assert.That(clock.Consume(133),Is.EqualTo(1));
            Assert.That(clock.Consume(199),Is.EqualTo(2));
            Assert.That(clock.Consume(199),Is.Zero);
            var wrapped=new LegacyFrameClock(uint.MaxValue-10,30);
            Assert.That(wrapped.Consume(22),Is.EqualTo(1));
        }
        [Test]
        public void LoopDisplaysLastFrameAndDiscardsOvershoot()
        {
            var state=State(); state.Advance(59);
            Assert.That(state.Frame,Is.EqualTo(59)); Assert.That(state.EndMotion,Is.False);
            state.Advance(1); Assert.That(state.Frame,Is.Zero); Assert.That(state.EndMotion,Is.True);
            state.Advance(125); Assert.That(state.Frame,Is.Zero);
            state.Advance(1); Assert.That(state.Frame,Is.EqualTo(1)); Assert.That(state.EndMotion,Is.False);
        }
        [Test]
        public void SameMotionKeepsFrameAndPauseWhileCustomRestartResets()
        {
            var state=State(); state.Advance(10); state.Paused=true;
            state.ChangeMotion(1,true); state.Advance(3);
            Assert.That(state.Frame,Is.EqualTo(10)); Assert.That(state.Paused,Is.True);
            state.ChangeMotion(1,true,true); Assert.That(state.Frame,Is.Zero); Assert.That(state.Paused,Is.False);
        }
        [Test]
        public void NonLoopingMotionReturnsToBaseAtItsConfiguredBoundary()
        {
            var state=State(); state.ChangeMotion(2,false);
            Assert.That(state.Frame,Is.EqualTo(2)); state.Advance(6);
            Assert.That(state.Frame,Is.EqualTo(8)); state.Advance(1);
            Assert.That(state.Motion,Is.EqualTo(1)); Assert.That(state.Frame,Is.Zero);
            Assert.That(state.EndMotion,Is.True);
        }
    }
}
