using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.AssetImporters;
using UnityEngine;

namespace Moxiang.Editor
{
    // Rest-pose inspection with complete source skin data retained as a subasset.
    [ScriptedImporter(3, "mxhmodel")]
    public sealed class MxhModelImporter : ScriptedImporter
    {
        public override void OnImportAsset(AssetImportContext context)
        {
            var descriptor = JsonUtility.FromJson<ModelDescriptor>(File.ReadAllText(context.assetPath));
            if (descriptor == null || descriptor.schemaVersion != 1 || descriptor.kind != "model" ||
                descriptor.releaseReady || descriptor.coordinateSpace != "legacy-unconverted" ||
                descriptor.sourceSha256 == null || descriptor.sourceSha256.Length != 64)
                throw new InvalidDataException("Unsupported model descriptor.");
            var data = ScriptableObject.CreateInstance<ImportedModel>(); data.descriptor = descriptor;
            context.AddObjectToAsset("source-model", data);
            var worldBones = descriptor.bones.ToDictionary(bone => bone.index, bone => LegacyModelGeometry.Matrix(bone.transform));
            var root = new GameObject(Path.GetFileNameWithoutExtension(context.assetPath));
            root.AddComponent<AnimatedModelPart>().source = data;
            var materials = new Dictionary<uint, Material>();
            foreach (var source in descriptor.materials) {
                // Explicit companion DDS derivatives are required; no runtime loose-file fallback.
                string textureRoot = Path.GetDirectoryName(context.assetPath).Replace('\\', '/') + "/Textures/" +
                    Path.GetFileNameWithoutExtension(source.textureName).ToLowerInvariant();
                string path = textureRoot + ".mxhdds";
                if (!File.Exists(path)) path = textureRoot + ".png";
                context.DependsOnSourceAsset(path);
                var texture = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
                if (texture == null) throw new InvalidDataException("Missing audited model texture: " + path);
                var shader = Shader.Find("Universal Render Pipeline/Lit");
                if (shader == null) throw new InvalidDataException("Missing pinned URP shader.");
                var material = new Material(shader) { name = "SourceMaterial" + source.index };
                material.SetTexture("_BaseMap", texture);
                materials.Add(source.index, material);
                context.AddObjectToAsset("material-" + source.index, material);
            }
            foreach (var source in descriptor.meshes) {
                var mesh = new Mesh { name = source.name };
                mesh.vertices = LegacyModelGeometry.Positions(source, worldBones);
                if (source.texcoords.Length != mesh.vertexCount) throw new InvalidDataException("Model UV count mismatch.");
                mesh.uv = source.texcoords;
                mesh.subMeshCount = source.faceGroups.Length;
                for (int i = 0; i < source.faceGroups.Length; ++i) mesh.SetTriangles(source.faceGroups[i].indices, i);
                if (source.normals != null && source.normals.Length == source.positions.Length)
                    mesh.normals = LegacyModelGeometry.Normals(source, worldBones);
                else if (source.normals == null || source.normals.Length == 0)
                    mesh.RecalculateNormals();
                else throw new InvalidDataException("Model normal count mismatch.");
                mesh.RecalculateBounds();
                var child = new GameObject(source.name);
                child.transform.SetParent(root.transform, false);
                child.AddComponent<MeshFilter>().sharedMesh = mesh;
                child.AddComponent<MeshRenderer>().sharedMaterials = source.faceGroups.Select(group => materials[group.materialIndex]).ToArray();
                context.AddObjectToAsset("mesh-" + source.index, mesh);
            }
            context.AddObjectToAsset("root", root); context.SetMainObject(root);
        }
    }
}
