using System;
using System.IO;
using TMPro;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace Moxiang.Editor
{
    public static class TerrainPaletteSetup
    {
        private const string Root = "Assets/Moxiang/Derived/Map10/";
        public static void Apply()
        {
            if (UnityEngine.SceneManagement.SceneManager.GetActiveScene().isDirty)
                throw new InvalidOperationException("Save the active scene before terrain replacement.");
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(Root + "Map10.mxhterrain");
            if (prefab == null || prefab.transform.childCount != 64) throw new InvalidDataException("Imported Map10 terrain missing.");
            var scene = EditorSceneManager.OpenScene(ConnectionSceneBuilder.ScenePath, OpenSceneMode.Single);
            var previous = GameObject.Find("Map10TexturedTerrain");
            if (previous != null) Undo.DestroyObjectImmediate(previous);
            var inspection = GameObject.Find("Map10GeometryInspection");
            if (inspection == null) throw new InvalidOperationException("Expected inspection terrain missing.");
            inspection.GetComponent<MeshRenderer>().enabled = false;
            PrefabUtility.InstantiatePrefab(prefab, scene);
            foreach (var label in UnityEngine.Object.FindObjectsByType<TMP_Text>(FindObjectsSortMode.None))
                if (label.text.StartsWith("MAP 10")) label.text = "MAP 10 · SOURCE MATERIAL INSPECTION\nCollision, full gameplay and remaster quality remain unaccepted.";
            if (!EditorSceneManager.SaveScene(scene)) throw new IOException("Could not save imported terrain scene.");
            AssetDatabase.SaveAssets(); Debug.Log("MXH_IMPORTED_TERRAIN_READY chunks=64");
        }

    }
}
