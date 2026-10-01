using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace Moxiang
{
    [Serializable] public sealed class CharacterMotion {
        public uint motionNumber;
        public ImportedMotion source;
    }
    public sealed class CharacterAnimation : MonoBehaviour
    {
        public LegacyAnimationDriver driver;
        public AnimatedModelPart[] skinnedParts;
        public AnimatedModelPart face;
        public AnimatedModelPart headSkeleton;
        public CharacterMotion[] motions;
        public uint baseMotion=1;
        LegacyAnimationDriver subscribedDriver;
        LegacyAnimationState state;
        readonly Dictionary<uint,LegacyMotionSampler> samplers=new Dictionary<uint,LegacyMotionSampler>();
        uint headIndex;
        public uint CurrentFrame => state?.Frame ?? 0;

        void Initialize()
        {
            if(state!=null)return;
            if(motions==null || skinnedParts==null || headSkeleton==null || headSkeleton.source==null || face==null)
                throw new InvalidDataException("Incomplete character animation binding.");
            bool found=false;
            foreach(var bone in headSkeleton.source.descriptor.bones)
                if(bone.name=="Bip01 Head") {if(found)throw new InvalidDataException("Duplicate head node."); headIndex=bone.index;found=true;}
            if(!found)throw new InvalidDataException("Missing head attachment node.");
            var ranges=new Dictionary<uint,LegacyMotionRange>();
            var pending=new Dictionary<uint,LegacyMotionSampler>();
            foreach(var motion in motions) {
                if(motion?.source==null)throw new InvalidDataException("Missing character motion.");
                ranges.Add(motion.motionNumber,new LegacyMotionRange(motion.source.descriptor.lastFrame));
                pending.Add(motion.motionNumber,new LegacyMotionSampler(motion.source.descriptor));
            }
            foreach(var part in skinnedParts)if(part==null || part.source==null)throw new InvalidDataException("Missing skinned part.");
            var next=new LegacyAnimationState(ranges,baseMotion);
            samplers.Clear(); foreach(var entry in pending)samplers.Add(entry.Key,entry.Value);
            state=next;
        }
        void OnEnable()
        {
            if(driver==null)return;
            Initialize(); subscribedDriver=driver; subscribedDriver.FrameAdvanced+=Advance;
            Sample();
        }
        void OnDisable()
        {
            if(subscribedDriver!=null)subscribedDriver.FrameAdvanced-=Advance;
            subscribedDriver=null;
        }
        public void ChangeMotion(uint motion,bool loop=true,bool forceRestart=false)
        {
            Initialize(); state.ChangeMotion(motion,loop,forceRestart); Sample();
        }
        public void Advance(uint increment)
        {
            Initialize(); state.Advance(increment); Sample();
        }
        void Sample()
        {
            var sampler=samplers[state.Motion];
            var head=sampler.Sample(headSkeleton.source.descriptor.bones,state.Frame)[headIndex];
            foreach(var part in skinnedParts)part.SampleFrame(sampler,state.Frame);
            face.SampleFrame(sampler,state.Frame,head);
        }
    }
}
