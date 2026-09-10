using UnityEditor;
using UnityEditor.PackageManager;
using UnityEditor.PackageManager.Requests;
using UnityEngine;

namespace Moxiang.Editor
{
    public static class GraphicsModuleSetup
    {
        private static AddAndRemoveRequest request;
        private static double deadline;
        public static void Install()
        {
            request = Client.AddAndRemove(new[] { "com.unity.modules.amd@1.0.0", "com.unity.modules.nvidia@1.0.0" });
            deadline = EditorApplication.timeSinceStartup + 180;
            EditorApplication.update += Poll;
        }
        private static void Poll()
        {
            if (!request.IsCompleted)
            {
                if (EditorApplication.timeSinceStartup < deadline) return;
                EditorApplication.update -= Poll;
                Debug.LogError("MXH_GRAPHICS_MODULES_TIMEOUT");
                EditorApplication.Exit(2);
                return;
            }
            EditorApplication.update -= Poll;
            if (request.Status != StatusCode.Success)
            {
                Debug.LogError("MXH_GRAPHICS_MODULES_FAILED " + request.Error?.message);
                EditorApplication.Exit(1);
                return;
            }
            Debug.Log("MXH_GRAPHICS_MODULES_READY");
            EditorApplication.Exit(0);
        }
    }
}
