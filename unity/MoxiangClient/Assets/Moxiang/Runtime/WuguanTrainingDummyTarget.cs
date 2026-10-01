using UnityEngine;

namespace Moxiang
{
    // Presentation only. No damage, health, skill timing or network state is changed.
    public sealed class WuguanTrainingDummyTarget : MonoBehaviour
    {
        public int index;
        public Transform visual;
        public float maxTiltDegrees = 8f;
        private Transform boundVisual;
        private Quaternion restRotation;

        public Quaternion EvaluateLocalRotation(Vector3 localHitDirection, float normalizedTime)
        {
            if (!Finite(normalizedTime) || !Finite(maxTiltDegrees) ||
                !Finite(localHitDirection.x) || !Finite(localHitDirection.y) || !Finite(localHitDirection.z))
                return Quaternion.identity;
            float t = Mathf.Clamp01(normalizedTime);
            if (localHitDirection.sqrMagnitude < .0001f || t <= 0f || t >= 1f)
                return Quaternion.identity;
            Vector3 planar = Vector3.ProjectOnPlane(localHitDirection, Vector3.up).normalized;
            if (planar.sqrMagnitude < .0001f) return Quaternion.identity;
            Vector3 axis = Vector3.Cross(Vector3.up, planar).normalized;
            float envelope = Mathf.Sin(Mathf.PI * t);
            return Quaternion.AngleAxis(Mathf.Clamp(maxTiltDegrees, 0f, 30f) * envelope, axis);
        }

        public void ApplyVisualReaction(Vector3 localHitDirection, float normalizedTime)
        {
            if (!visual) return;
            if (boundVisual != visual)
            {
                boundVisual = visual;
                restRotation = visual.localRotation;
            }
            visual.localRotation = restRotation * EvaluateLocalRotation(localHitDirection, normalizedTime);
        }

        private void OnDisable()
        {
            if (boundVisual) boundVisual.localRotation = restRotation;
            boundVisual = null;
        }

        private static bool Finite(float value)
        {
            return !float.IsNaN(value) && !float.IsInfinity(value);
        }
    }
}