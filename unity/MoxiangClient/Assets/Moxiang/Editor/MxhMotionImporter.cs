using System.IO;
using UnityEditor.AssetImporters;
using UnityEngine;

namespace Moxiang.Editor
{
    [ScriptedImporter(1, "mxhmotion")]
    public sealed class MxhMotionImporter : ScriptedImporter
    {
        public override void OnImportAsset(AssetImportContext context)
        {
            var descriptor = JsonUtility.FromJson<MotionDescriptor>(File.ReadAllText(context.assetPath));
            if (descriptor == null || descriptor.schemaVersion != 1 || descriptor.kind != "motion"
                || descriptor.profileId != "unity-remaster-v1" || descriptor.releaseReady
                || descriptor.sourceSha256 == null || descriptor.sourceSha256.Length != 64)
                throw new InvalidDataException("Unsupported motion asset or provenance.");
            var asset = ScriptableObject.CreateInstance<ImportedMotion>();
            asset.descriptor = descriptor;
            context.AddObjectToAsset("motion", asset);
            context.SetMainObject(asset);
        }
    }
}
