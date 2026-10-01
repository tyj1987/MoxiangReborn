using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Samples original monster ANM data without moving the authoritative root.</summary>
    public sealed class ServerMonsterAnimation : MonoBehaviour
    {
        public const uint IdleMotion = 1;
        public const uint MoveMotion = 2;
        public const uint AttackMotion = 4;
        public const uint HitMotion = 8;
        public const uint DeathMotion = 9;

        LegacyAnimationDriver driver;
        AnimatedModelPart[] parts;
        LegacyAnimationState state;
        readonly Dictionary<uint, LegacyMotionSampler> samplers = new Dictionary<uint, LegacyMotionSampler>();
        bool disableAfterMotion;

        public uint CurrentMotion => state?.Motion ?? 0;
        public uint CurrentFrame => state?.Frame ?? 0;

        public void Initialize(LegacyAnimationDriver sourceDriver, ImportedMotion[] motions)
        {
            if (sourceDriver == null || motions == null || motions.Length != 12)
                throw new InvalidDataException("Monster animation requires one driver and 12 original motions.");
            driver = sourceDriver;
            parts = GetComponentsInChildren<AnimatedModelPart>();
            if (parts.Length == 0) throw new InvalidDataException("Monster animation has no imported model part.");
            var ranges = new Dictionary<uint, LegacyMotionRange>();
            for (int i = 0; i < motions.Length; ++i)
            {
                if (motions[i] == null) throw new InvalidDataException("Missing original monster motion.");
                uint number = checked((uint)i + 1);
                ranges.Add(number, new LegacyMotionRange(motions[i].descriptor.lastFrame));
                samplers.Add(number, new LegacyMotionSampler(motions[i].descriptor));
            }
            state = new LegacyAnimationState(ranges, IdleMotion);
            driver.FrameAdvanced += Advance;
            Sample();
        }

        public void PlayIdle() => Change(IdleMotion, true);
        public void PlayMove() => Change(MoveMotion, true);
        public void PlayAttack() => Change(AttackMotion, false);
        public void PlayHit() => Change(HitMotion, false);
        public void PlayDeath() { disableAfterMotion = true; Change(DeathMotion, false); }

        void Change(uint motion, bool loop)
        {
            if (state == null) return;
            state.ChangeMotion(motion, loop, true);
            Sample();
        }

        void Advance(uint increment)
        {
            if (state == null) return;
            state.Advance(increment);
            if (disableAfterMotion && state.EndMotion) { gameObject.SetActive(false); return; }
            Sample();
        }

        void Sample()
        {
            var sampler = samplers[state.Motion];
            foreach (var part in parts) part.SampleFrame(sampler, state.Frame);
        }

        void OnDestroy()
        {
            if (driver != null) driver.FrameAdvanced -= Advance;
        }
    }
}
