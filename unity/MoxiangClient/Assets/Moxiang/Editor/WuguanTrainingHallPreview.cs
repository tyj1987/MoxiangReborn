using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
namespace Moxiang.Editor
{
    public static class WuguanTrainingHallPreview
    {
        [System.Serializable]
        private sealed class Evidence
        {
            public Vector3 cameraPosition, targetPosition, combatViewport;
            public bool cameraAboveFloor, combatInFrame;
            public string scope = "Prefab preview only; not gameplay or art acceptance";
        }
        public static void ConfigureCamera(Camera camera, WuguanTrainingHallModule module)
        {
            if (!camera || !module || !module.combatZone || !module.entranceAnchor)
                throw new InvalidDataException("Camera and hall anchors are required.");
            Vector3 inward = Vector3.ProjectOnPlane(module.combatZone.position - module.entranceAnchor.position, Vector3.up);
            if (inward.sqrMagnitude < .01f) throw new InvalidDataException("Entrance overlaps combat anchor.");
            camera.transform.position = module.entranceAnchor.position + inward.normalized * 1.3f + Vector3.up * 3.3f;
            camera.transform.LookAt(module.combatZone.position + Vector3.up * 1.2f, Vector3.up);
            camera.fieldOfView = 76f;
            camera.aspect = 1280f / 720f;
            camera.nearClipPlane = .05f;
            camera.farClipPlane = 100f;
            camera.clearFlags = CameraClearFlags.SolidColor;
            camera.backgroundColor = new Color(.025f, .03f, .035f);
        }
        public static void Capture()
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(WuguanTrainingHallSetup.PrefabPath);
            if (!prefab) throw new InvalidDataException("Wuguan prefab missing.");
            var scene = EditorSceneManager.NewPreviewScene();
            var previousTarget = RenderTexture.active;
            Camera camera = null;
            RenderTexture target = null;
            Texture2D image = null;
            try
            {
                var root = (GameObject)PrefabUtility.InstantiatePrefab(prefab, scene);
                root.transform.position = Vector3.zero;
                var module = root.GetComponent<WuguanTrainingHallModule>();
                var sun = InScene("PreviewSun", scene).AddComponent<Light>();
                sun.type = LightType.Directional;
                sun.color = new Color(1f, .91f, .78f);
                sun.intensity = 1.8f;
                sun.transform.rotation = Quaternion.Euler(48f, -28f, 0f);
                sun.shadows = LightShadows.Soft;
                var fill = InScene("PreviewFill", scene).AddComponent<Light>();
                fill.type = LightType.Point;
                fill.color = new Color(.55f, .66f, .85f);
                fill.intensity = 1.5f;
                fill.range = 18f;
                fill.transform.position = module.combatZone.position + Vector3.up * 4f;
                camera = InScene("PreviewCamera", scene).AddComponent<Camera>();
                camera.scene = scene;
                ConfigureCamera(camera, module);
                var targetPosition = module.combatZone.position + Vector3.up * 1.2f;
                var vp = camera.WorldToViewportPoint(targetPosition);
                bool above = camera.transform.position.y > module.combatZone.position.y + .5f;
                bool framed = vp.z > camera.nearClipPlane && vp.x > .05f && vp.x < .95f && vp.y > .05f && vp.y < .95f;
                if (!above || !framed) throw new InvalidDataException("Preview camera is below the hall or not framing combat zone.");
                target = new RenderTexture(1280, 720, 24);
                image = new Texture2D(1280, 720, TextureFormat.RGB24, false);
                camera.targetTexture = target;
                camera.Render();
                RenderTexture.active = target;
                image.ReadPixels(new Rect(0, 0, 1280, 720), 0, 0);
                image.Apply();
                var output = Path.GetFullPath(Path.Combine(Application.dataPath, "../../../modern/out/unity-remaster/wuguan-training-hall"));
                Directory.CreateDirectory(output);
                File.WriteAllBytes(Path.Combine(output, "unity-preview.png"), image.EncodeToPNG());
                File.WriteAllText(Path.Combine(output, "preview-camera.json"), JsonUtility.ToJson(new Evidence {
                    cameraPosition = camera.transform.position, targetPosition = targetPosition, combatViewport = vp,
                    cameraAboveFloor = above, combatInFrame = framed
                }, true));
                Debug.Log("MXH_WUGUAN_PREVIEW_SAVED " + output + " camera_above_floor=true");
            }
            finally
            {
                RenderTexture.active = previousTarget;
                if (camera) camera.targetTexture = null;
                if (target) { target.Release(); Object.DestroyImmediate(target); }
                if (image) Object.DestroyImmediate(image);
                EditorSceneManager.ClosePreviewScene(scene);
            }
        }
        private static GameObject InScene(string name, Scene scene)
        {
            var result = new GameObject(name);
            SceneManager.MoveGameObjectToScene(result, scene);
            return result;
        }
    }
}
