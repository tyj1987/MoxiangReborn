using System;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;

namespace Moxiang.Editor
{
    /// <summary>Development assets cannot silently enter a release build.</summary>
    public sealed class RemasterBuildGuard : IPreprocessBuildWithReport
    {
        public int callbackOrder => -1000;
        public void OnPreprocessBuild(BuildReport report)
        {
            RemasterSetup.ValidateConfiguration();
            if (report.summary.platform != BuildTarget.StandaloneWindows64)
                throw new BuildFailedException("Remaster baseline only validates Windows x64.");
            if ((report.summary.options & BuildOptions.Development) == 0)
                throw new BuildFailedException("Release gate is closed: native integration, asset provenance, gameplay and visual acceptance remain incomplete.");
            foreach (var guid in AssetDatabase.FindAssets("t:ImportedHeightField"))
            {
                var asset = AssetDatabase.LoadAssetAtPath<ImportedHeightField>(AssetDatabase.GUIDToAssetPath(guid));
                if (asset != null && asset.descriptor.releaseReady)
                    throw new BuildFailedException("Inspection importer cannot certify release-ready terrain.");
            }
        }
    }
}
