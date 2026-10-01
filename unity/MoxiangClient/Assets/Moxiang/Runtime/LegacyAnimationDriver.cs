using System;
using UnityEngine;

namespace Moxiang
{
    // One scene driver broadcasts the same Executive increment to all characters.
    public sealed class LegacyAnimationDriver : MonoBehaviour
    {
        public uint gameFps=30;
        public event Action<uint> FrameAdvanced;
        LegacyFrameClock clock;
        void OnEnable() { clock=new LegacyFrameClock(unchecked((uint)Environment.TickCount),gameFps); }
        void Update() {
            uint increment=clock.Consume(unchecked((uint)Environment.TickCount));
            if(increment!=0)FrameAdvanced?.Invoke(increment);
        }
    }
}
