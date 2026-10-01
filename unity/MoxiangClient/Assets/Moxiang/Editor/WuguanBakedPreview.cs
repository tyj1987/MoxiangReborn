using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
namespace Moxiang.Editor
{
    public static class WuguanBakedPreview
    {
        [Serializable] public sealed class View
        {
            public string name,imagePath,sha256;
            public Vector3 cameraPosition,targetPosition;
            public int width=1920,height=1080,lightmaps,staticLightmappedRenderers;
            public string sourceScene=WuguanLightingBake.ScenePath;
        }
        public static void Capture()
        {
            if(!Application.isBatchMode)throw new InvalidOperationException("Use the project batch capture workflow.");
            for(int i=0;i<SceneManager.sceneCount;i++)if(SceneManager.GetSceneAt(i).isDirty)throw new InvalidOperationException("Unsaved scene blocks capture.");
            var scene=EditorSceneManager.OpenScene(WuguanLightingBake.ScenePath,OpenSceneMode.Single);
            var content=scene.GetRootGameObjects().SelectMany(x=>x.GetComponentsInChildren<WuguanMap44Content>()).Single();
            content.Validate();var module=content.hall;
            if(LightmapSettings.lightmaps.Length==0)throw new InvalidDataException("Real scene lightmaps are missing.");
            var cameraObject=new GameObject("BakedSceneEvidenceCamera");var camera=cameraObject.AddComponent<Camera>();camera.scene=scene;
            var previous=RenderTexture.active;RenderTexture rt=null;Texture2D image=null;
            var output=Path.Combine(WuguanTrainingHallPbr.Repository,"modern/out/unity-remaster/wuguan-training-hall");
            try
            {
                rt=new RenderTexture(1920,1080,24);image=new Texture2D(1920,1080,TextureFormat.RGB24,false);
                camera.targetTexture=rt;var views=new System.Collections.Generic.List<View>();
                foreach(var name in new[]{"entry","side","dummy-close"})
                {
                    WuguanTrainingHallPreview.ConfigureCamera(camera,module);camera.aspect=16f/9f;
                    Vector3 target=module.combatZone.position+Vector3.up*1.2f;
                    if(name=="side"){camera.transform.position=module.combatZone.position+new Vector3(6.6f,2.6f,2.5f);camera.fieldOfView=70;}
                    if(name=="dummy-close"){target=module.dummyAnchors[0].position+Vector3.up*1.1f;camera.transform.position=target+new Vector3(1.8f,.6f,2f);camera.fieldOfView=43;}
                    camera.transform.LookAt(target,Vector3.up);camera.Render();RenderTexture.active=rt;
                    image.ReadPixels(new Rect(0,0,1920,1080),0,0);image.Apply();
                    var file=Path.Combine(output,"map44-baked-"+name+".png");File.WriteAllBytes(file,image.EncodeToPNG());
                    views.Add(new View{name=name,imagePath=file,sha256=WuguanTrainingHallPbr.Digest(file),cameraPosition=camera.transform.position,targetPosition=target,
                        lightmaps=LightmapSettings.lightmaps.Length,staticLightmappedRenderers=module.GetComponentsInChildren<MeshRenderer>().Count(x=>!x.GetComponentInParent<WuguanTrainingDummyTarget>()&&x.lightmapIndex>=0&&x.lightmapIndex<LightmapSettings.lightmaps.Length)});
                    File.WriteAllText(Path.Combine(output,"map44-baked-"+name+".json"),JsonUtility.ToJson(views.Last(),true));
                }
                Debug.Log("MXH_MAP44_BAKED_PREVIEW_OK views=3 actual_lightmaps="+LightmapSettings.lightmaps.Length);
            }
            finally{RenderTexture.active=previous;camera.targetTexture=null;if(rt){rt.Release();UnityEngine.Object.DestroyImmediate(rt);}if(image)UnityEngine.Object.DestroyImmediate(image);UnityEngine.Object.DestroyImmediate(cameraObject);}
        }
    }
}
