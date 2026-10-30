using System;
using System.Collections.Generic;
using System.Linq;
using TMPro;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.TextCore.LowLevel;

namespace Moxiang.Editor
{
    public static class CjkFontSetup
    {
        public const string FallbackPath = "Assets/Moxiang/Fonts/MoxiangCjkFallback.asset";

        public static void Apply()
        {
            if (EditorApplication.isPlaying || UnityEngine.SceneManagement.SceneManager.GetActiveScene().isDirty)
                throw new InvalidOperationException("Finish play mode and save scene edits before font setup.");
            var source = AssetDatabase.LoadAssetAtPath<Font>("Assets/Moxiang/Fonts/NotoSansCJKsc-Regular.otf");
            if (source == null) throw new InvalidOperationException("Import the audited CJK source font first.");
            var fallback = AssetDatabase.LoadAssetAtPath<TMP_FontAsset>(FallbackPath);
            if (fallback == null)
            {
                fallback = TMP_FontAsset.CreateFontAsset(source, 40, 4, GlyphRenderMode.SDFAA,
                    1024, 1024, AtlasPopulationMode.Dynamic, true);
                fallback.name = "MoxiangCjkFallback";
                AssetDatabase.CreateAsset(fallback, FallbackPath);
                AssetDatabase.AddObjectToAsset(fallback.material, fallback);
                foreach (var texture in fallback.atlasTextures) AssetDatabase.AddObjectToAsset(texture, fallback);
            }
            var serialized = new SerializedObject(fallback);
            var clear = serialized.FindProperty("m_ClearDynamicDataOnBuild");
            if (clear == null) throw new InvalidOperationException("Pinned TMP build cleanup setting missing.");
            clear.boolValue = true;
            serialized.ApplyModifiedPropertiesWithoutUndo();

            var scene = EditorSceneManager.OpenScene(ConnectionSceneBuilder.ScenePath, OpenSceneMode.Single);
            var labels = scene.GetRootGameObjects().SelectMany(root => root.GetComponentsInChildren<TMP_Text>(true)).ToArray();
            foreach (var font in labels.Select(label => label.font).Where(font => font != null && font != fallback).Distinct())
            {
                if (font.fallbackFontAssetTable == null) font.fallbackFontAssetTable = new List<TMP_FontAsset>();
                if (!font.fallbackFontAssetTable.Contains(fallback)) font.fallbackFontAssetTable.Add(fallback);
                EditorUtility.SetDirty(font);
            }
            int checkedLabels = 0;
            foreach (var label in labels)
            {
                string visible = new string(label.text.Where(character => !char.IsControl(character)).ToArray());
                if (label.font == null || !label.font.HasCharacters(visible, out uint[] missing, true, true))
                    throw new InvalidOperationException("Missing glyphs in UI label: " + label.name);
                label.ForceMeshUpdate(true, true);
                checkedLabels++;
            }
            EditorUtility.SetDirty(fallback);
            AssetDatabase.SaveAssets();
            EditorSceneManager.MarkSceneDirty(scene);
            if (!EditorSceneManager.SaveScene(scene)) throw new InvalidOperationException("Failed to save CJK scene.");
            Debug.Log("MXH_CJK_FONT_READY labels=" + checkedLabels + " atlas=1024 source=bundled clearOnBuild=true");
        }
    }
}
