using System.IO;
using UnityEditor.AssetImporters;
using UnityEngine;

namespace Moxiang.Editor
{
    [ScriptedImporter(1,"mxhappearance")]
    public sealed class MxhAppearanceImporter : ScriptedImporter
    {
        public override void OnImportAsset(AssetImportContext context)
        {
            var descriptor=JsonUtility.FromJson<AppearanceDescriptor>(File.ReadAllText(context.assetPath));
            _=new AppearanceIndex(descriptor);
            if(descriptor.sources==null || descriptor.sources.Length!=7)throw new InvalidDataException("Missing appearance provenance.");
            foreach(var source in descriptor.sources)
                if(source==null || string.IsNullOrEmpty(source.path) || source.sha256==null || source.sha256.Length!=64 || source.bytes==0)
                    throw new InvalidDataException("Invalid appearance provenance.");
            var asset=ScriptableObject.CreateInstance<ImportedAppearance>();asset.descriptor=descriptor;
            context.AddObjectToAsset("appearance",asset);context.SetMainObject(asset);
        }
    }
}
