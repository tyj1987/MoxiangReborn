using System;
using Effekseer;
using UnityEngine;

namespace Moxiang.Vfx
{
    [Serializable]
    public sealed class SkillVfxBinding
    {
        public uint skillId;
        public EffekseerEffectAsset castEffect;
        public EffekseerEffectAsset impactEffect;
        public Vector3 castOffset = new Vector3(0f, 1f, 0f);
        public Vector3 impactOffset = new Vector3(0f, 1f, 0f);
        [Min(0.01f)] public float scale = 1f;
    }

    /// <summary>
    /// Client-only visual bridge for server-authoritative skill requests.
    /// It never sends gameplay messages and never changes hit or damage state.
    /// </summary>
    public sealed class SkillVfxController : MonoBehaviour
    {
        public SkillHotkeyController skillHotkeys;
        public Transform castOrigin;
        public SkillVfxBinding[] bindings = Array.Empty<SkillVfxBinding>();

        public event Action<uint> EffectPlayed;

        private void Reset()
        {
            skillHotkeys = GetComponentInParent<SkillHotkeyController>();
            castOrigin = transform;
        }

        private void OnEnable()
        {
            if (skillHotkeys == null)
                skillHotkeys = GetComponentInParent<SkillHotkeyController>();

            if (skillHotkeys != null)
                skillHotkeys.SkillRequested += HandleSkillRequested;
        }

        private void OnDisable()
        {
            if (skillHotkeys != null)
                skillHotkeys.SkillRequested -= HandleSkillRequested;
        }

        public bool TryGetBinding(uint skillId, out SkillVfxBinding binding)
        {
            if (bindings != null)
            {
                for (int i = 0; i < bindings.Length; ++i)
                {
                    var candidate = bindings[i];
                    if (candidate != null && candidate.skillId == skillId)
                    {
                        binding = candidate;
                        return true;
                    }
                }
            }

            binding = null;
            return false;
        }

        public bool Play(uint skillId)
        {
            if (!TryGetBinding(skillId, out var binding) ||
                (binding.castEffect == null && binding.impactEffect == null))
                return false;

            Vector3 origin = (castOrigin != null ? castOrigin.position : transform.position) + binding.castOffset;
            Vector3 target = ResolveTargetPosition(origin) + binding.impactOffset;
            Quaternion rotation = EvaluateRotation(origin, target, transform.rotation);
            Vector3 effectScale = Vector3.one * Mathf.Max(0.01f, binding.scale);

            if (binding.castEffect != null)
            {
                var castHandle = EffekseerSystem.PlayEffect(binding.castEffect, origin);
                castHandle.SetRotation(rotation);
                castHandle.SetScale(effectScale);
            }

            if (binding.impactEffect != null)
            {
                var impactHandle = EffekseerSystem.PlayEffect(binding.impactEffect, target);
                impactHandle.SetRotation(rotation);
                impactHandle.SetScale(effectScale);
            }

            EffectPlayed?.Invoke(skillId);
            return true;
        }

        public Vector3 ResolveTargetPosition(Vector3 fallbackOrigin)
        {
            var selection = skillHotkeys == null ? null : skillHotkeys.targetSelection;
            var selected = selection == null ? null : selection.selected;
            if (selected != null && selected.gameObject.activeInHierarchy)
                return selected.transform.position;

            Vector3 forward = castOrigin != null ? castOrigin.forward : transform.forward;
            if (forward.sqrMagnitude < 0.0001f)
                forward = Vector3.forward;
            return fallbackOrigin + forward.normalized * 2f;
        }

        public static Quaternion EvaluateRotation(Vector3 origin, Vector3 target, Quaternion fallback)
        {
            Vector3 direction = target - origin;
            if (direction.sqrMagnitude < 0.0001f)
                return fallback;

            return Quaternion.LookRotation(direction.normalized, Vector3.up);
        }

        private void HandleSkillRequested(uint skillId)
        {
            Play(skillId);
        }
    }
}
