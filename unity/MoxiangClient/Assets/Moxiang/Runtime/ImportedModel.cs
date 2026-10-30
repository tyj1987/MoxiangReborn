using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace Moxiang
{
    [Serializable] public sealed class ModelInfluence { public uint boneIndex; public float weight; public Vector3 offset, normalOffset; }
    [Serializable] public sealed class ModelPhysique { public ModelInfluence[] influences; }
    [Serializable] public sealed class ModelFaceGroup { public uint materialIndex; public int[] indices; }
    [Serializable] public sealed class ModelMaterial { public uint index, diffuse, flags; public float transparency; public string textureName; }
    [Serializable] public sealed class ModelBone { public uint index, parentIndex; public string name; public Vector3 position, rotationAxis, scale; public float rotationAngle; public float[] transform; }
    [Serializable] public sealed class ModelMesh {
        public uint index, flags; public string name; public float[] transform;
        public Vector3[] positions, normals; public Vector2[] texcoords;
        public ModelFaceGroup[] faceGroups; public ModelPhysique[] physique;
    }
    [Serializable] public sealed class ModelDescriptor {
        public int schemaVersion; public bool releaseReady;
        public string kind, profileId, converterVersion, sourceId, sourceSha256, coordinateSpace;
        public ModelMaterial[] materials; public ModelBone[] bones; public ModelMesh[] meshes;
    }
    public sealed class ImportedModel : ScriptableObject { public ModelDescriptor descriptor; }

    public static class LegacyModelGeometry
    {
        // Original AttachDress replaces the rigid object's root matrix with the
        // attachment node. MOD positions first become local through source inverse.
        public static Vector3[] AttachedPositions(ModelMesh mesh, Matrix4x4 attachment)
        {
            if (mesh.positions == null) throw new InvalidDataException("Missing rigid vertices.");
            if (mesh.physique != null && mesh.physique.Length != 0)
                throw new InvalidDataException("Rigid attachment cannot replace a skinned mesh.");
            var source = Matrix(mesh.transform);
            if (Mathf.Abs(source.determinant) < 1e-12f) throw new InvalidDataException("Singular attachment source matrix.");
            var result = new Vector3[mesh.positions.Length];
            var world = attachment * source.inverse;
            for (int i=0;i<result.Length;++i) {
                result[i] = world.MultiplyPoint3x4(mesh.positions[i]) * 0.001f;
                if (!float.IsFinite(result[i].x) || !float.IsFinite(result[i].y) || !float.IsFinite(result[i].z))
                    throw new InvalidDataException("Non-finite attached position.");
            }
            return result;
        }

        public static Matrix4x4 Matrix(float[] source)
        {
            if (source == null || source.Length != 16) throw new InvalidDataException("Invalid model matrix.");
            var matrix = new Matrix4x4();
            // Legacy row-vector layout -> Unity column-vector layout.
            for (int i = 0; i < 16; ++i) {
                if (!float.IsFinite(source[i])) throw new InvalidDataException("Non-finite model matrix.");
                matrix[i % 4, i / 4] = source[i];
            }
            return matrix;
        }

        public static Vector3[] Positions(ModelMesh mesh, IReadOnlyDictionary<uint, Matrix4x4> worldBones)
        {
            if (mesh.positions == null) throw new InvalidDataException("Missing model vertices.");
            var output = new Vector3[mesh.positions.Length];
            _ = Matrix(mesh.transform);
            if (mesh.physique != null && mesh.physique.Length != 0 && mesh.physique.Length != output.Length)
                throw new InvalidDataException("Incomplete model skin data.");
            for (int i = 0; i < output.Length; ++i) {
                var influences = mesh.physique != null && mesh.physique.Length != 0 ? mesh.physique[i].influences : null;
                // MOD stores m_pv3World. Legacy CommitDevice applies the inverse
                // mesh transform to obtain local vertices before rendering.
                // In the exported source pose these positions are already world-space.
                if (influences == null || influences.Length == 0) output[i] = mesh.positions[i];
                else {
                    float total = 0;
                    foreach (var influence in influences) {
                        if (!worldBones.TryGetValue(influence.boneIndex, out var bone) || !float.IsFinite(influence.weight) || influence.weight < 0)
                            throw new InvalidDataException("Unresolved or invalid bone influence.");
                        output[i] += bone.MultiplyPoint3x4(influence.offset) * influence.weight;
                        total += influence.weight;
                    }
                    if (Mathf.Abs(total - 1) > 0.001f) throw new InvalidDataException("Unnormalized source skin weights.");
                }
                if (!float.IsFinite(output[i].x) || !float.IsFinite(output[i].y) || !float.IsFinite(output[i].z))
                    throw new InvalidDataException("Non-finite model position.");
                output[i] *= 0.001f;
            }
            return output;
        }

        public static Vector3[] Normals(ModelMesh mesh, IReadOnlyDictionary<uint, Matrix4x4> worldBones)
        {
            if (mesh.normals == null || mesh.normals.Length != mesh.positions.Length)
                throw new InvalidDataException("Model normal count mismatch.");
            var output = new Vector3[mesh.normals.Length];
            for (int i = 0; i < output.Length; ++i) {
                var influences = mesh.physique != null && mesh.physique.Length != 0 ? mesh.physique[i].influences : null;
                if (influences == null || influences.Length == 0) output[i] = mesh.normals[i];
                else foreach (var influence in influences)
                    output[i] += worldBones[influence.boneIndex].MultiplyVector(influence.normalOffset) * influence.weight;
                if (!float.IsFinite(output[i].x) || !float.IsFinite(output[i].y) || !float.IsFinite(output[i].z))
                    throw new InvalidDataException("Non-finite model normal.");
                output[i] = output[i].normalized;
            }
            return output;
        }
    }
}
