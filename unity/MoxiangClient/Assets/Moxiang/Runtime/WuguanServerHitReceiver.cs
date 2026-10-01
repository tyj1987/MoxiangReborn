using System;
using UnityEngine;
namespace Moxiang
{
    /// <summary>Consumes confirmed native SkillHit events; never sends damage or changes health.</summary>
    [DisallowMultipleComponent]
    public sealed class WuguanServerHitReceiver : MonoBehaviour
    {
        public WuguanTrainingDummyTarget target;
        public AudioSource impactAudio;
        public ParticleSystem impactParticles;
        public uint ObjectId { get; private set; }
        public ulong SessionGeneration { get; private set; }
        public ulong MapGeneration { get; private set; }
        public ulong LastSequence { get; private set; }
        public uint LastDamage { get; private set; }
        public uint LastHitKind { get; private set; }
        public int AcceptedHitCount { get; private set; }
        // SingleResult does not carry attacker, skill id, direction or world hit point.
        public bool HasAuthoritativeDirection => false;
        public event Action<CoreEvent> ServerHitConfirmed;
        private TargetSelectable identity;
        private Transform pulseVisual;
        private Vector3 restScale;
        private float elapsed=-1f;
        private const float Duration=.18f;

        public bool BindServerEntity(CoreEvent added,TargetSelectable selectable)
        {
            ResetBinding();
            if(added.type!=NativeClient.EventMonsterAdded||added.result!=CoreResult.Ok||
               added.state!=CoreState.InGame||added.argument0==0||added.sessionGeneration==0||
               added.mapGeneration==0||!selectable||selectable.isNpc||selectable.objectId!=added.argument0)
                return false;
            identity=selectable;ObjectId=added.argument0;SessionGeneration=added.sessionGeneration;
            MapGeneration=added.mapGeneration;LastSequence=added.sequence;
            if(!target)target=GetComponent<WuguanTrainingDummyTarget>();
            return true;
        }

        public bool AcceptServerHit(CoreEvent hit)
        {
            if(!isActiveAndEnabled||ObjectId==0||!identity||identity.objectId!=ObjectId||identity.isNpc||
               hit.type!=NativeClient.EventSkillHit||hit.result!=CoreResult.Ok||hit.state!=CoreState.InGame||
               hit.argument0!=ObjectId||hit.sessionGeneration!=SessionGeneration||hit.mapGeneration!=MapGeneration||
               hit.sequence<=LastSequence||hit.argument1==0||hit.argument1>int.MaxValue)
                return false;
            LastSequence=hit.sequence;LastDamage=hit.argument1;LastHitKind=hit.reserved0;AcceptedHitCount++;
            RestorePulse();
            if(target&&target.visual){pulseVisual=target.visual;restScale=pulseVisual.localScale;elapsed=0;}
            if(impactAudio&&impactAudio.clip)impactAudio.Play();
            if(impactParticles)impactParticles.Play(true);
            ServerHitConfirmed?.Invoke(hit);
            return true;
        }
        private void Update(){AdvancePresentation(Time.deltaTime);}
        public void AdvancePresentation(float deltaTime)
        {
            if(elapsed<0||!pulseVisual||float.IsNaN(deltaTime)||float.IsInfinity(deltaTime)||deltaTime<0)return;
            elapsed=Mathf.Min(Duration,elapsed+deltaTime);
            if(elapsed>=Duration){RestorePulse();return;}
            // Symmetric compression communicates impact without inventing an absent hit direction.
            float pulse=1f-.025f*Mathf.Sin(Mathf.PI*elapsed/Duration);
            pulseVisual.localScale=restScale*pulse;
        }
        private void RestorePulse()
        {
            if(pulseVisual)pulseVisual.localScale=restScale;
            pulseVisual=null;elapsed=-1;
        }
        public void ResetBinding()
        {
            RestorePulse();identity=null;ObjectId=0;SessionGeneration=0;MapGeneration=0;LastSequence=0;
            LastDamage=0;LastHitKind=0;AcceptedHitCount=0;
        }
        private void OnDisable(){ResetBinding();}
    }
}
