using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
namespace Moxiang.Editor
{
    public static class WuguanTrainingDummyPrefab
    {
        public const string Path="Assets/Moxiang/Art/WuguanTrainingHall/Prefabs/ServerTrainingDummy.prefab";
        public static GameObject Build()
        {
            var hall=AssetDatabase.LoadAssetAtPath<GameObject>(WuguanTrainingHallSetup.PrefabPath);
            if(!hall)throw new InvalidDataException("Build the verified hall before its server target presentation.");
            var donor=hall.GetComponent<WuguanTrainingHallModule>().dummyAnchors[0].GetComponent<WuguanTrainingDummyTarget>();
            if(!donor||!donor.visual)throw new InvalidDataException("Audited body/arm visual is missing.");
            var root=new GameObject("ServerTrainingDummy");
            try
            {
                var visual=Object.Instantiate(donor.visual.gameObject,root.transform,false);visual.name="Visual";
                visual.transform.localPosition=Vector3.zero;visual.transform.localRotation=Quaternion.identity;visual.transform.localScale=Vector3.one;
                var target=root.AddComponent<WuguanTrainingDummyTarget>();target.visual=visual.transform;
                var receiver=root.AddComponent<WuguanServerHitReceiver>();receiver.target=target;
                var selectable=root.AddComponent<TargetSelectable>();selectable.objectId=0;selectable.isNpc=false;
                root.AddComponent<ServerEntityHealth>();
                var collider=root.AddComponent<CapsuleCollider>();collider.radius=.55f;collider.height=2.2f;collider.center=new Vector3(0,1.1f,0);
                foreach(var renderer in visual.GetComponentsInChildren<MeshRenderer>(true))
                {GameObjectUtility.SetStaticEditorFlags(renderer.gameObject,0);renderer.receiveGI=ReceiveGI.LightProbes;renderer.lightProbeUsage=LightProbeUsage.BlendProbes;}
                var prefab=PrefabUtility.SaveAsPrefabAsset(root,Path);if(!prefab)throw new IOException("Cannot save authoritative target presentation.");
                AssetDatabase.SaveAssets();return prefab;
            }
            finally{Object.DestroyImmediate(root);}
        }
    }
}
