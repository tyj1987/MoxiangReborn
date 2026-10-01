using System;
using System.IO;
using System.Linq;
using Effekseer;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering.Universal;

namespace Moxiang.Vfx.Editor
{
    public static class EffekseerIntegrationSetup
    {
        public const string SettingsPath = "Assets/Moxiang/Vfx/Resources/EffekseerSettings.asset";

        private static readonly string[] RendererPaths =
        {
            "Assets/Settings/PC_Renderer.asset",
            "Assets/Settings/Mobile_Renderer.asset",
        };

        [MenuItem("Moxiang/VFX/Configure Effekseer")]
        public static void Configure()
        {
            var settings = EnsureSettings();
            int added = 0;
            foreach (string rendererPath in RendererPaths)
                if (EnsureRenderFeature(rendererPath))
                    ++added;

            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
            Debug.Log($"MXH_EFFEKSEER_CONFIGURED settings={AssetDatabase.GetAssetPath(settings)} render_features_added={added}");
        }

        public static EffekseerSettings EnsureSettings()
        {
            var settings = AssetDatabase.LoadAssetAtPath<EffekseerSettings>(SettingsPath);
            if (settings != null)
                return settings;

            string directory = Path.GetDirectoryName(SettingsPath)?.Replace('\\', '/');
            if (string.IsNullOrEmpty(directory))
                throw new InvalidOperationException("Invalid Effekseer settings path.");

            EnsureAssetFolder(directory);
            settings = ScriptableObject.CreateInstance<EffekseerSettings>();
            settings.RendererType = EffekseerRendererType.Native;
            settings.effectInstances = 4096;
            settings.maxSquares = 8192;
            settings.threadCount = Mathf.Clamp(SystemInfo.processorCount / 2, 1, 4);
            settings.enableDistortion = true;
            settings.enableDepth = true;
            settings.enableDistortionMobile = false;
            settings.enableDepthMobile = false;
            settings.DoStartNetworkAutomatically = false;
            AssetDatabase.CreateAsset(settings, SettingsPath);
            return settings;
        }

        public static bool EnsureRenderFeature(string rendererPath)
        {
            var renderer = AssetDatabase.LoadAssetAtPath<UniversalRendererData>(rendererPath);
            if (renderer == null)
                throw new InvalidDataException($"Missing URP renderer data: {rendererPath}");

            if (renderer.rendererFeatures.Any(x => x is EffekseerURPRenderPassFeature))
                return false;

            var feature = ScriptableObject.CreateInstance<EffekseerURPRenderPassFeature>();
            feature.name = "Effekseer";
            feature.LayerMask = ~0;
            AssetDatabase.AddObjectToAsset(feature, renderer);
            renderer.rendererFeatures.Add(feature);
            feature.Create();
            EditorUtility.SetDirty(feature);
            EditorUtility.SetDirty(renderer);
            return true;
        }

        private static void EnsureAssetFolder(string assetPath)
        {
            string[] parts = assetPath.Split('/');
            string current = parts[0];
            for (int i = 1; i < parts.Length; ++i)
            {
                string next = current + "/" + parts[i];
                if (!AssetDatabase.IsValidFolder(next))
                    AssetDatabase.CreateFolder(current, parts[i]);
                current = next;
            }
        }
    }
}
