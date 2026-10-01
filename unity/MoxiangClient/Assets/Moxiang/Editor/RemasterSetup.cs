using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEngine;
using UnityEngine.Rendering;

namespace Moxiang.Editor
{
    /// <summary>Explicit project configuration; never changes source gameplay data.</summary>
    public static class RemasterSetup
    {
        public const string EditorVersion = "6000.6.0f1";

        public static void ConfigureInputCompatibility()
        {
            var settings = new SerializedObject(AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/ProjectSettings.asset")[0]);
            var input = settings.FindProperty("activeInputHandler");
            if (input == null) throw new InvalidOperationException("Pinned Editor input setting is unavailable.");
            // uGUI uses Input System; existing world/skill controls use UnityEngine.Input.
            input.intValue = 2; // Both
            settings.ApplyModifiedPropertiesWithoutUndo();
            AssetDatabase.SaveAssets();
            Debug.Log("MXH_INPUT_COMPATIBILITY_READY both=true restart_before_build=true");
        }

        public static void Configure()
        {
            if (Application.unityVersion != EditorVersion)
                throw new InvalidOperationException("Remaster requires Unity " + EditorVersion);
            ConfigureInputCompatibility();
            PlayerSettings.companyName = "Moxiang";
            PlayerSettings.productName = "Moxiang Remaster";
            PlayerSettings.SetApplicationIdentifier(NamedBuildTarget.Standalone, "com.moxiang.remaster");
            PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.StandaloneWindows64, false);
            PlayerSettings.SetGraphicsAPIs(BuildTarget.StandaloneWindows64, new[] { GraphicsDeviceType.Direct3D11 });
            PlayerSettings.SetScriptingBackend(NamedBuildTarget.Standalone, ScriptingImplementation.Mono2x);
            PlayerSettings.defaultScreenWidth = 800;
            PlayerSettings.defaultScreenHeight = 600;
            PlayerSettings.fullScreenMode = FullScreenMode.Windowed;
            PlayerSettings.resizableWindow = true;
            PlayerSettings.runInBackground = true;
            PlayerSettings.colorSpace = ColorSpace.Linear;
            var pipeline = AssetDatabase.LoadAssetAtPath<RenderPipelineAsset>("Assets/Settings/PC_RPAsset.asset");
            if (pipeline == null) throw new InvalidOperationException("Pinned PC URP asset is missing.");
            GraphicsSettings.defaultRenderPipeline = pipeline;
            EditorSettings.serializationMode = SerializationMode.ForceText;
            EditorUserBuildSettings.SwitchActiveBuildTarget(BuildTargetGroup.Standalone, BuildTarget.StandaloneWindows64);
            AssetDatabase.SaveAssets();
            Debug.Log("MXH_SETUP_OK editor=" + Application.unityVersion + " target=Windows64 graphics=D3D11 backend=Mono");
        }

        public static void ValidateConfiguration()
        {
            if (Application.unityVersion != EditorVersion ||
                PlayerSettings.GetUseDefaultGraphicsAPIs(BuildTarget.StandaloneWindows64) ||
                !PlayerSettings.GetGraphicsAPIs(BuildTarget.StandaloneWindows64).SequenceEqual(new[] { GraphicsDeviceType.Direct3D11 }) ||
                GraphicsSettings.defaultRenderPipeline == null ||
                !GraphicsSettings.defaultRenderPipeline.GetType().FullName.Contains("UniversalRenderPipelineAsset"))
                throw new InvalidOperationException("Pinned Unity/URP/D3D11 configuration is missing. Run RemasterSetup.Configure in the target Editor.");
            Debug.Log("MXH_CONFIGURATION_VALID");
        }

        public static void BuildDevelopment()
        {
            ValidateConfiguration();
            const string pluginPath = "Assets/Plugins/x86_64/mxh_unity_core.dll";
            var plugin = AssetImporter.GetAtPath(pluginPath) as PluginImporter;
            if (plugin == null) throw new InvalidOperationException("Build and stage the x64 native core first.");
            plugin.SetCompatibleWithAnyPlatform(false);
            plugin.SetCompatibleWithEditor(true);
            plugin.SetEditorData("OS", "Windows");
            plugin.SetEditorData("CPU", "x86_64");
            plugin.SetCompatibleWithPlatform(BuildTarget.StandaloneWindows64, true);
            plugin.SetPlatformData(BuildTarget.StandaloneWindows64, "CPU", "x86_64");
            plugin.SaveAndReimport();
            var scenes = EditorBuildSettings.scenes.Where(s => s.enabled).Select(s => s.path).ToArray();
            if (scenes.Length == 0) throw new InvalidOperationException("No reviewed bootstrap scene in build settings.");
            var path = Path.GetFullPath(Path.Combine(Application.dataPath, "../../..", "modern/out/unity-remaster/player/MoxiangClient.exe"));
            Directory.CreateDirectory(Path.GetDirectoryName(path));
            var report = BuildPipeline.BuildPlayer(new BuildPlayerOptions {
                scenes = scenes, locationPathName = path, target = BuildTarget.StandaloneWindows64,
                options = BuildOptions.Development
            });
            if (report.summary.result != BuildResult.Succeeded)
                throw new InvalidOperationException("Player build failed: " + report.summary.result);
            Debug.Log("MXH_PLAYER_BUILD_OK " + path);
        }
    }
}
