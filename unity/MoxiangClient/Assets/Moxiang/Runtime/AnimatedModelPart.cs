using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace Moxiang
{
    // Each instance owns its deformable meshes. Imported/shared meshes stay immutable.
    [ExecuteAlways]
    public sealed class AnimatedModelPart : MonoBehaviour
    {
        public ImportedModel source;
        readonly List<Mesh> ownedMeshes = new List<Mesh>();
        readonly List<MeshFilter> filters = new List<MeshFilter>();
        readonly List<ModelMesh> sourceMeshes = new List<ModelMesh>();
        readonly List<Mesh> originalMeshes = new List<Mesh>();
        bool initialized;

        void Initialize()
        {
            if (initialized) return;
            if (source == null || source.descriptor?.meshes == null) throw new InvalidDataException("Missing model source.");
            var available = GetComponentsInChildren<MeshFilter>(true);
            if (available.Length != source.descriptor.meshes.Length) throw new InvalidDataException("Model instance mesh count mismatch.");
            var used = new HashSet<MeshFilter>();
            var mappedFilters = new List<MeshFilter>();
            var mappedSources = new List<ModelMesh>();
            // Validate the complete mapping before changing any renderer reference.
            foreach (var mesh in source.descriptor.meshes) {
                MeshFilter match = null;
                foreach (var filter in available)
                    if (filter.sharedMesh != null && filter.sharedMesh.name == mesh.name && !used.Contains(filter)) {
                        if (match != null) throw new InvalidDataException("Ambiguous model mesh name.");
                        match = filter;
                    }
                if (match == null) throw new InvalidDataException("Missing model instance mesh.");
                used.Add(match); mappedFilters.Add(match); mappedSources.Add(mesh);
            }
            filters.AddRange(mappedFilters); sourceMeshes.AddRange(mappedSources);
            for (int i=0;i<filters.Count;++i) {
                originalMeshes.Add(filters[i].sharedMesh);
                var copy = Instantiate(filters[i].sharedMesh);
                copy.MarkDynamic(); ownedMeshes.Add(copy); filters[i].sharedMesh=copy;
            }
            initialized = true;
        }

        public void SampleFrame(LegacyMotionSampler sampler, float frame, Matrix4x4? rigidAttachment = null)
        {
            if (sampler == null) throw new ArgumentNullException(nameof(sampler));
            Initialize();
            var bones = sampler.Sample(source.descriptor.bones,frame);
            for (int i=0;i<ownedMeshes.Count;++i) {
                var mesh = ownedMeshes[i]; var original = sourceMeshes[i];
                if (rigidAttachment.HasValue) {
                    mesh.vertices = LegacyModelGeometry.AttachedPositions(original,rigidAttachment.Value);
                    mesh.RecalculateNormals();
                } else {
                    mesh.vertices = LegacyModelGeometry.Positions(original,bones);
                    if (original.normals != null && original.normals.Length == original.positions.Length)
                        mesh.normals = LegacyModelGeometry.Normals(original,bones);
                    else if (original.normals == null || original.normals.Length == 0)
                        mesh.RecalculateNormals();
                    else throw new InvalidDataException("Model normal count mismatch.");
                }
                mesh.RecalculateBounds();
            }
        }

        void OnDisable() => ReleaseMeshes();
        void OnDestroy() => ReleaseMeshes();
        void ReleaseMeshes()
        {
            for(int i=0;i<originalMeshes.Count;++i)
                if (filters[i] != null) filters[i].sharedMesh=originalMeshes[i];
            foreach (var mesh in ownedMeshes) {
                if (mesh == null) continue;
                if (Application.isPlaying) Destroy(mesh); else DestroyImmediate(mesh);
            }
            ownedMeshes.Clear(); filters.Clear(); sourceMeshes.Clear(); originalMeshes.Clear();
            initialized = false;
        }
    }
}
