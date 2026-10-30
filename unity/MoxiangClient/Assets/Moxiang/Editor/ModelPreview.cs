using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace Moxiang.Editor
{
    public static class ModelPreview
    {
        public static void Capture()
        {
            CaptureFrame(-1);
        }
        public static void CaptureFrame(int frame)
        {
            var previous = UnityEngine.SceneManagement.SceneManager.GetActiveScene();
            var previousRenderTarget = RenderTexture.active;
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Additive);
            UnityEngine.SceneManagement.SceneManager.SetActiveScene(scene);
            RenderTexture target = null;
            Texture2D pixels = null;
            try {
                LegacyMotionSampler sampler = null;
                Matrix4x4 head = Matrix4x4.identity;
                if (frame >= 0) {
                    var motion = AssetDatabase.LoadAssetAtPath<ImportedMotion>("Assets/Moxiang/Derived/Man/Motions/m001.mxhmotion");
                    sampler = new LegacyMotionSampler(motion.descriptor);
                    var body = AssetDatabase.LoadAllAssetsAtPath("Assets/Moxiang/Derived/Man/m_body01.mxhmodel").OfType<ImportedModel>().Single().descriptor;
                    head = sampler.Sample(body.bones,frame)[body.bones.Single(b=>b.name=="Bip01 Head").index];
                }
                var owner = new GameObject("ManSourcePoseInspection");
                Bounds bounds = default;
                bool first = true;
                foreach (string part in new[] {"m_body01", "m_face01", "m_hair01", "m_hand01", "m_shoes01"}) {
                    var prefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Moxiang/Derived/Man/" + part + ".mxhmodel");
                    if (prefab == null) throw new InvalidDataException("Missing imported part " + part);
                    var instance = UnityEngine.Object.Instantiate(prefab, owner.transform);
                    if (sampler != null) {
                        instance.GetComponent<AnimatedModelPart>().SampleFrame(sampler,frame,part=="m_face01" ? head : (Matrix4x4?)null);
                    }
                    foreach (var renderer in instance.GetComponentsInChildren<Renderer>()) {
                        renderer.gameObject.layer = 31;
                        if (first) { bounds = renderer.bounds; first = false; } else bounds.Encapsulate(renderer.bounds);
                    }
                }
                var sun = new GameObject("PreviewLight").AddComponent<Light>();
                sun.type = LightType.Directional; sun.intensity = 1.5f; sun.cullingMask = 1 << 31;
                sun.transform.rotation = Quaternion.Euler(35, -25, 0);
                var camera = new GameObject("PreviewCamera").AddComponent<Camera>();
                camera.cullingMask = 1 << 31; camera.clearFlags = CameraClearFlags.SolidColor;
                camera.backgroundColor = new Color(0.08f, 0.1f, 0.13f);
                camera.nearClipPlane = 0.001f; camera.farClipPlane = 10;
                camera.orthographic = true; camera.orthographicSize = Mathf.Max(bounds.extents.y, bounds.extents.x) * 1.25f;
                camera.transform.position = bounds.center + new Vector3(0.08f, 0.03f, -0.5f);
                camera.transform.LookAt(bounds.center);
                target = new RenderTexture(800, 800, 24); camera.targetTexture = target;
                pixels = new Texture2D(800, 800, TextureFormat.RGB24, false);
                camera.Render(); RenderTexture.active = target;
                pixels.ReadPixels(new Rect(0,0,800,800),0,0); pixels.Apply();
                string filename = frame < 0 ? "man-source-pose.png" : "man-motion-frame-" + frame + ".png";
                string path = Path.GetFullPath(Path.Combine(Application.dataPath, "../../../modern/out/unity-remaster/" + filename));
                File.WriteAllBytes(path, pixels.EncodeToPNG());
                Debug.Log("MXH_MAN_POSE_PREVIEW frame=" + frame + " bounds=" + bounds.size);
            }
            finally {
                RenderTexture.active = previousRenderTarget;
                if (target != null) { target.Release(); UnityEngine.Object.DestroyImmediate(target); }
                if (pixels != null) UnityEngine.Object.DestroyImmediate(pixels);
                UnityEngine.SceneManagement.SceneManager.SetActiveScene(previous);
                EditorSceneManager.CloseScene(scene, true);
            }
        }
    }
}
