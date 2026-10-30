using System;
using System.IO;
using UnityEditor;
using UnityEngine;
namespace Moxiang.Editor
{
    public static class WuguanHighLowSetup
    {
        public const string Root="Assets/Moxiang/Art/WuguanTrainingHall";
        public const string ManifestPath=Root+"/highlow-manifest.json";
        [Serializable] public sealed class Pair {public string low,high,cage;public bool refined;public int lowTriangles,highTriangles;public float cageDistance;}
        [Serializable] public sealed class Statistics {public float mean,min,max,occludedRatio,normalChangedRatio;}
        [Serializable] public sealed class Evidence
        {
            public int schema,size,refinedPairs,lowTriangles,highTriangles;
            public string sourceBlend,sourceSha256,pbrManifestSha256,script,scriptSha256;
            public string normalPath,normalSha256,aoPath,aoSha256,blendPath,blendSha256;
            public Pair[] pairs;public Statistics stats;
        }
        public static Evidence Validate()
        {
            var e=JsonUtility.FromJson<Evidence>(File.ReadAllText(ManifestPath));
            if(e==null||e.schema!=1||e.size!=4096||e.pairs==null||e.pairs.Length!=66||e.refinedPairs<21||
               e.highTriangles<=e.lowTriangles||e.stats==null||e.stats.normalChangedRatio<.005f||e.stats.occludedRatio<.005f)
                throw new InvalidDataException("High/low bake evidence incomplete.");
            string prefix="unity/MoxiangClient/"+Root;
            if(e.normalPath!=prefix+"/Textures/Hall_HighLow_Normal.png"||e.aoPath!=prefix+"/Textures/Hall_AO.png"||
               e.blendPath!="unity/source-overlays/wuguan-training-hall/WuguanTrainingHall_HighLow.blend")
                throw new InvalidDataException("High/low output paths are not the reviewed paths.");
            WuguanTrainingHallPbr.VerifyHash(e.sourceBlend,e.sourceSha256);
            WuguanTrainingHallPbr.VerifyHash(e.script,e.scriptSha256);
            WuguanTrainingHallPbr.VerifyHash("unity/MoxiangClient/"+Root+"/pbr-manifest.json",e.pbrManifestSha256);
            WuguanTrainingHallPbr.VerifyHash(e.normalPath,e.normalSha256);
            WuguanTrainingHallPbr.VerifyHash(e.aoPath,e.aoSha256);
            WuguanTrainingHallPbr.VerifyHash(e.blendPath,e.blendSha256);
            int refined=0;var names=new System.Collections.Generic.HashSet<string>();
            foreach(var p in e.pairs){if(p==null||!names.Add(p.low)||p.high!="HIGH_"+p.low||p.cage!="CAGE_"+p.low||p.cageDistance<=0)throw new InvalidDataException("Invalid or duplicate high/low pair.");if(p.refined){if(p.highTriangles<=p.lowTriangles)throw new InvalidDataException("Refined pair lacks high geometry.");refined++;}}
            if(refined!=e.refinedPairs)throw new InvalidDataException("High/low refinement count mismatch.");
            return e;
        }
        public static void Apply(Material material)
        {
            Validate();
            var normal=Import(Root+"/Textures/Hall_HighLow_Normal.png",true);
            var ao=Import(Root+"/Textures/Hall_AO.png",false);
            material.SetTexture("_BumpMap",normal);material.SetTexture("_OcclusionMap",ao);
            material.SetFloat("_OcclusionStrength",.8f);material.EnableKeyword("_NORMALMAP");
            EditorUtility.SetDirty(material);
            Debug.Log("MXH_WUGUAN_HIGHLOW_APPLIED refined_pairs=21 ao=ray_baked");
        }
        static Texture2D Import(string path,bool normal)
        {
            AssetDatabase.ImportAsset(path,ImportAssetOptions.ForceSynchronousImport);
            var importer=AssetImporter.GetAtPath(path) as TextureImporter;
            if(!importer)throw new InvalidDataException("Missing high/low texture importer.");
            importer.textureType=normal?TextureImporterType.NormalMap:TextureImporterType.Default;
            importer.sRGBTexture=false;importer.mipmapEnabled=true;importer.maxTextureSize=4096;
            importer.wrapMode=TextureWrapMode.Clamp;importer.convertToNormalmap=false;
            importer.alphaIsTransparency=false;importer.isReadable=false;
            var platform=importer.GetPlatformTextureSettings("Standalone");platform.overridden=true;
            platform.maxTextureSize=4096;platform.format=normal?TextureImporterFormat.BC5:TextureImporterFormat.BC7;
            platform.compressionQuality=100;importer.SetPlatformTextureSettings(platform);importer.SaveAndReimport();
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }
    }
}
