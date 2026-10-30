using UnityEngine;

namespace Moxiang
{
    /// <summary>
    /// Stable runtime attachment contract shared by remastered melee weapons.
    /// Gameplay and VFX code should bind through this component instead of
    /// searching arbitrary imported FBX hierarchy names at runtime.
    /// </summary>
    public sealed class WeaponSocketRig : MonoBehaviour
    {
        [SerializeField] private Transform grip;
        [SerializeField] private Transform trailBase;
        [SerializeField] private Transform trailTip;
        [SerializeField] private Transform primaryFx;

        public Transform Grip => grip;
        public Transform TrailBase => trailBase;
        public Transform TrailTip => trailTip;
        public Transform PrimaryFx => primaryFx;

        public void Configure(
            Transform gripTransform,
            Transform trailBaseTransform,
            Transform trailTipTransform,
            Transform primaryFxTransform)
        {
            grip = gripTransform;
            trailBase = trailBaseTransform;
            trailTip = trailTipTransform;
            primaryFx = primaryFxTransform;
        }

        public bool ValidateContract(out string error)
        {
            if (!grip)
            {
                error = "Weapon grip socket is missing.";
                return false;
            }

            if (!trailBase)
            {
                error = "Weapon trail base socket is missing.";
                return false;
            }

            if (!trailTip)
            {
                error = "Weapon trail tip socket is missing.";
                return false;
            }

            if (!primaryFx)
            {
                error = "Weapon primary FX socket is missing.";
                return false;
            }

            if (trailBase == trailTip)
            {
                error = "Weapon trail base and tip must be different transforms.";
                return false;
            }

            if (!IsOwnedTransform(grip) ||
                !IsOwnedTransform(trailBase) ||
                !IsOwnedTransform(trailTip) ||
                !IsOwnedTransform(primaryFx))
            {
                error = "All weapon sockets must belong to the weapon prefab hierarchy.";
                return false;
            }

            error = string.Empty;
            return true;
        }

        private bool IsOwnedTransform(Transform candidate)
        {
            return candidate && (candidate == transform || candidate.IsChildOf(transform));
        }
    }
}
