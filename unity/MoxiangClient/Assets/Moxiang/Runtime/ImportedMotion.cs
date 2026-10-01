using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace Moxiang
{
    [Serializable] public sealed class MotionVectorKey { public uint ticks, frame; public Vector3 value, axis; public float axisAngle; }
    [Serializable] public sealed class MotionRotationKey { public uint ticks, frame; public Quaternion value; }
    [Serializable] public sealed class MotionTrack {
        public uint index, sourceObjectType, motionFlags, meshAnimationKeyCount;
        public string name; public MotionVectorKey[] positions, scales;
        public MotionRotationKey[] rotations; public byte[] meshAnimationBytes;
    }
    [Serializable] public sealed class MotionDescriptor {
        public int schemaVersion; public bool releaseReady;
        public string kind, profileId, converterVersion, sourceId, sourceSha256;
        public uint ticksPerFrame, firstFrame, lastFrame, frameSpeed, keyFrameStep;
        public byte[] sourceHeaderBytes; public MotionTrack[] objects;
    }
    public sealed class ImportedMotion : ScriptableObject { public MotionDescriptor descriptor; }

    // Explicit source frames only. Playback duration/action mapping is a separate contract.
    public sealed class LegacyMotionSampler
    {
        readonly Dictionary<string, MotionTrack> tracks = new Dictionary<string, MotionTrack>(StringComparer.Ordinal);
        public LegacyMotionSampler(MotionDescriptor motion)
        {
            if (motion == null || motion.schemaVersion != 1 || motion.kind != "motion" || motion.objects == null
                || motion.firstFrame > motion.lastFrame) throw new InvalidDataException("Invalid motion descriptor.");
            foreach (var track in motion.objects) {
                if (track == null || string.IsNullOrEmpty(track.name) || !tracks.TryAdd(track.name, track))
                    throw new InvalidDataException("Missing or duplicate motion track name.");
                // Preserve these tracks in the asset, but never silently claim to evaluate them.
                if (track.meshAnimationKeyCount != 0 || (track.scales != null && track.scales.Length != 0))
                    throw new NotSupportedException("Animated mesh and oriented scale tracks require a dedicated evaluator.");
                uint previous = 0;
                if (track.positions != null) for (int i = 0; i < track.positions.Length; ++i) {
                    var key = track.positions[i];
                    if (key == null || (i > 0 && key.frame <= previous) || !Finite(key.value))
                        throw new InvalidDataException("Invalid position keys.");
                    previous = key.frame;
                }
                previous = 0;
                if (track.rotations != null) for (int i = 0; i < track.rotations.Length; ++i) {
                    var key = track.rotations[i];
                    if (key == null || (i > 0 && key.frame <= previous) || !Finite(key.value)
                        || Quaternion.Dot(key.value, key.value) < 0.000001f)
                        throw new InvalidDataException("Invalid rotation keys.");
                    previous = key.frame;
                }
            }
        }
        static bool Finite(Vector3 v) => float.IsFinite(v.x) && float.IsFinite(v.y) && float.IsFinite(v.z);
        static bool Finite(Quaternion q) => float.IsFinite(q.x) && float.IsFinite(q.y) && float.IsFinite(q.z) && float.IsFinite(q.w);

        static Vector3 Position(MotionVectorKey[] keys, float frame, Vector3 fallback)
        {
            if (keys == null || keys.Length == 0) return fallback;
            if (frame <= keys[0].frame) return keys[0].value;
            for (int i = 1; i < keys.Length; ++i)
                if (frame <= keys[i].frame) return Vector3.LerpUnclamped(keys[i-1].value, keys[i].value,
                    (frame - keys[i-1].frame) / (keys[i].frame - keys[i-1].frame));
            return keys[keys.Length-1].value;
        }
        static Quaternion Rotation(MotionRotationKey[] keys, float frame, Quaternion fallback)
        {
            if (keys == null || keys.Length == 0) return fallback;
            if (frame <= keys[0].frame) return keys[0].value;
            for (int i = 1; i < keys.Length; ++i)
                if (frame <= keys[i].frame) return SourceSlerp(keys[i-1].value, keys[i].value,
                    (frame - keys[i-1].frame) / (keys[i].frame - keys[i-1].frame));
            return keys[keys.Length-1].value;
        }
        // SS3DGFunc.dll (F87933...796DA) uses polynomial Sin/ACos, not CRT
        // trigonometry or normalized Unity Slerp. Keep source keys immutable:
        // the legacy function negates its first input in-place for a negative dot.
        static Quaternion SourceSlerp(Quaternion a, Quaternion b, float t)
        {
            float dot = (float)((double)a.x*b.x + (double)a.z*b.z + (double)a.y*b.y + (double)a.w*b.w);
            if (dot < 0) { a = new Quaternion(-a.x,-a.y,-a.z,-a.w); dot = -dot; }
            float wa = 1-t, wb = t;
            if (1-dot > 0.05f) {
                float square = dot*dot;
                float cube = dot*dot*dot, fifth = dot*square*square;
                float theta = BitConverter.Int32BitsToSingle(0x3fc90fdb) -
                    ((dot + fifth*BitConverter.Int32BitsToSingle(0x3d99999a)) + cube*BitConverter.Int32BitsToSingle(0x3e2aaa7e));
                float Sin(float x) {
                    float xx = x*x;
                    return (x + (x*xx*xx)*BitConverter.Int32BitsToSingle(0x3c088723)) -
                        (x*x*x)*BitConverter.Int32BitsToSingle(0x3e2aaa7e);
                }
                float inverse = 1/Sin(theta);
                wa = Sin((1-t)*theta)*inverse; wb = Sin(t*theta)*inverse;
            }
            return new Quaternion(a.x*wa+b.x*wb,a.y*wa+b.y*wb,a.z*wa+b.z*wb,a.w*wa+b.w*wb);
        }
        Matrix4x4 Local(ModelBone bone, float frame)
        {
            if (!Finite(bone.position) || !Finite(bone.scale) || !Finite(bone.rotationAxis) || !float.IsFinite(bone.rotationAngle))
                throw new InvalidDataException("Invalid bone transform.");
            tracks.TryGetValue(bone.name, out var track);
            float half = -bone.rotationAngle * 0.5f;
            var axis = bone.rotationAxis * Mathf.Sin(half);
            var q = Rotation(track?.rotations, frame, new Quaternion(axis.x, axis.y, axis.z, Mathf.Cos(half)));
            // Transpose the native row-vector quaternion matrix, including its handedness.
            // Do not feed the source quaternion directly into Unity's TRS.
            var rotation = LegacyModelGeometry.Matrix(new float[] {
                1-2*q.y*q.y-2*q.z*q.z, 2*q.x*q.y-2*q.z*q.w, 2*q.x*q.z+2*q.y*q.w, 0,
                2*q.x*q.y+2*q.z*q.w, 1-2*q.x*q.x-2*q.z*q.z, 2*q.y*q.z-2*q.x*q.w, 0,
                2*q.x*q.z-2*q.y*q.w, 2*q.y*q.z+2*q.x*q.w, 1-2*q.x*q.x-2*q.y*q.y, 0, 0,0,0,1 });
            return Matrix4x4.Translate(Position(track?.positions, frame, bone.position)) * rotation * Matrix4x4.Scale(bone.scale);
        }
        public Dictionary<uint, Matrix4x4> Sample(ModelBone[] bones, float frame)
        {
            if (bones == null || !float.IsFinite(frame)) throw new InvalidDataException("Invalid skeleton or frame.");
            var source = new Dictionary<uint, ModelBone>();
            foreach (var bone in bones)
                if (bone == null || string.IsNullOrEmpty(bone.name) || !source.TryAdd(bone.index, bone))
                    throw new InvalidDataException("Invalid or duplicate bone.");
            var output = new Dictionary<uint, Matrix4x4>();
            var visiting = new HashSet<uint>();
            Matrix4x4 Visit(uint id) {
                if (output.TryGetValue(id, out var result)) return result;
                if (!source.TryGetValue(id, out var bone) || !visiting.Add(id))
                    throw new InvalidDataException("Missing parent or cyclic skeleton.");
                result = Local(bone, frame);
                if (bone.parentIndex != uint.MaxValue) result = Visit(bone.parentIndex) * result;
                visiting.Remove(id); output.Add(id, result); return result;
            }
            foreach (var bone in bones) Visit(bone.index);
            return output;
        }
    }
}
