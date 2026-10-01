using System;
using System.Collections.Generic;

namespace Moxiang
{
    public sealed class LegacyFrameClock
    {
        readonly uint epoch, stepMilliseconds;
        uint previousFrame;
        public LegacyFrameClock(uint initialTick, uint gameFps)
        {
            if (gameFps == 0 || gameFps > 1000) throw new ArgumentOutOfRangeException(nameof(gameFps));
            epoch = initialTick; stepMilliseconds = 1000 / gameFps;
        }
        public uint Consume(uint now)
        {
            uint frame = unchecked(now-epoch)/stepMilliseconds;
            uint increment = unchecked(frame-previousFrame);
            previousFrame = frame;
            return increment;
        }
    }

    public readonly struct LegacyMotionRange
    {
        public readonly uint Start, Last;
        public LegacyMotionRange(uint lastFrame, uint startFrame=0, uint endFrame=65535)
        {
            Start=startFrame; Last=Math.Min(lastFrame,endFrame);
            if (Start>Last) throw new ArgumentOutOfRangeException(nameof(startFrame));
        }
    }

    // Receives the Executive's shared integer increment; it never derives time
    // from ANM frameSpeed or from reported animation duration.
    public sealed class LegacyAnimationState
    {
        readonly Dictionary<uint,LegacyMotionRange> motions;
        public uint Motion {get;private set;}
        public uint Frame {get;private set;}
        public uint BaseMotion {get;set;}
        public bool Loop {get;private set;}
        public bool Paused {get;set;}
        public bool EndMotion {get;private set;}
        public LegacyAnimationState(IReadOnlyDictionary<uint,LegacyMotionRange> ranges,uint baseMotion)
        {
            if(ranges==null || !ranges.ContainsKey(baseMotion))throw new ArgumentException("Missing base motion.");
            motions=new Dictionary<uint,LegacyMotionRange>();
            foreach(var entry in ranges)motions.Add(entry.Key,entry.Value);
            BaseMotion=baseMotion;
            ChangeMotion(baseMotion,true,true);
        }
        public void ChangeMotion(uint motion,bool loop,bool forceRestart=false)
        {
            if(!motions.TryGetValue(motion,out var range))throw new ArgumentException("Unknown motion.");
            if(Motion!=motion || forceRestart) { Motion=motion; Frame=range.Start; Paused=false; }
            Loop=loop;
        }
        public void Advance(uint increment)
        {
            if(increment==0 || Paused)return;
            uint candidate=unchecked(Frame+increment);
            if(candidate>motions[Motion].Last) {
                EndMotion=true;
                if(Loop)Frame=motions[Motion].Start;
                else ChangeMotion(BaseMotion,true);
            } else { Frame=candidate; EndMotion=false; }
        }
    }

}
