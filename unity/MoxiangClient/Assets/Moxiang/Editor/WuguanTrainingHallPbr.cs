using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using UnityEditor;
using UnityEngine;
namespace Moxiang.Editor
{
    public static class WuguanTrainingHallPbr
    {
        public const string ArtPath="Assets/Moxiang/Art/WuguanTrainingHall";
        public const string ModelPath=ArtPath+"/Source/WuguanTrainingHall_PBR.fbx";
        public const string MaterialPath=ArtPath+"/Materials/Wuguan_PBR_Atlas.mat";
        public const string ManifestPath=ArtPath+"/pbr-manifest.json";
        public const string SourcePath="unity/source-overlays/wuguan-training-hall/WuguanTrainingHall.blend";
        public const string ScriptPath="unity/source-overlays/wuguan-training-hall/bake_pbr.py";
        public static string Repository => Path.GetFullPath(Path.Combine(Application.dataPath,"../../.."));
        [Serializable] public sealed class TextureRecord
        {
            public string role,path,sha256,colorSpace;
            public int width,height;
            public float coverage;
        }
        [Serializable] public sealed class Manifest
        {
            public int schema,atlasSize,objects,triangles,sourceEvaluatedTriangles;
            public string engine,sourceBlend,sourceBlendSha256,bakeScript,bakeScriptSha256;
            public string modelPath,modelSha256,bakedBlendPath,bakedBlendSha256,normalConvention,packing;
            public TextureRecord[] textures;
        }
        public static Manifest LoadManifest()
        {
            var text=File.ReadAllText(Path.Combine(Application.dataPath,"Moxiang/Art/WuguanTrainingHall/pbr-manifest.json"));
            var m=JsonUtility.FromJson<Manifest>(text);
            ValidateManifest(m);
            return m;
        }
        public static void ValidateManifest(Manifest m)
        {
            if(m==null||m.schema!=1||m.engine!="CYCLES"||m.atlasSize!=4096||m.objects!=66||m.triangles<=0||m.triangles>100000||m.triangles!=m.sourceEvaluatedTriangles)
                throw new InvalidDataException("PBR manifest schema or geometry mismatch.");
            if(m.sourceBlend!=SourcePath||m.bakeScript!=ScriptPath||m.modelPath!="unity/MoxiangClient/"+ModelPath||
               m.bakedBlendPath!="unity/source-overlays/wuguan-training-hall/WuguanTrainingHall_PBR.blend")
                throw new InvalidDataException("PBR source paths differ from reviewed paths.");
            if(m.normalConvention!="Tangent +X +Y +Z"||m.packing!="Metallic RGB; Smoothness A = 1 - linear Roughness")
                throw new InvalidDataException("PBR channel convention mismatch.");
            if(m.textures==null||m.textures.Length!=3||m.textures.Any(x=>x==null))
                throw new InvalidDataException("PBR texture set incomplete.");
            foreach(var role in new[]{"BaseColor","Normal","MetallicSmoothness"})
            {
                var matches=m.textures.Where(x=>x.role==role).ToArray();
                if(matches.Length!=1)throw new InvalidDataException("Duplicate or missing PBR role: "+role);
                var t=matches[0];
                if(t.path!="unity/MoxiangClient/"+ArtPath+"/Textures/Hall_"+role+".png"||
                   t.width!=4096||t.height!=4096||t.coverage<.35f||float.IsNaN(t.coverage)||
                   t.colorSpace!=(role=="BaseColor"?"sRGB":"Linear"))
                    throw new InvalidDataException("PBR texture metadata mismatch: "+role);
                VerifyHash(t.path,t.sha256);
            }
            VerifyHash(m.sourceBlend,m.sourceBlendSha256);
            VerifyHash(m.bakeScript,m.bakeScriptSha256);
            VerifyHash(m.modelPath,m.modelSha256);
            VerifyHash(m.bakedBlendPath,m.bakedBlendSha256);
        }
        public static void VerifyHash(string path,string expected)
        {
            if(string.IsNullOrEmpty(expected)||expected.Length!=64||expected.Any(c=>!Uri.IsHexDigit(c)))
                throw new InvalidDataException("Invalid PBR digest.");
            var absolute=Path.GetFullPath(Path.Combine(Repository,path));
            if(!absolute.StartsWith(Repository+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("PBR path escaped repository.");
            using(var stream=File.OpenRead(absolute))
            using(var algorithm=SHA256.Create())
            {
                var actual=BitConverter.ToString(algorithm.ComputeHash(stream)).Replace("-","");
                if(!string.Equals(actual,expected,StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException("PBR content digest mismatch: "+path);
            }
        }
        private static Texture2D ImportTexture(TextureRecord t)
        {
            var path=t.path.Substring("unity/MoxiangClient/".Length);
            AssetDatabase.ImportAsset(path,ImportAssetOptions.ForceSynchronousImport);
            var importer=AssetImporter.GetAtPath(path) as TextureImporter;
            if(importer==null)throw new InvalidDataException("Texture importer missing: "+path);
            importer.textureType=t.role=="Normal"?TextureImporterType.NormalMap:TextureImporterType.Default;
            importer.sRGBTexture=t.role=="BaseColor";
            importer.convertToNormalmap=false;
            importer.mipmapEnabled=true;
            importer.maxTextureSize=4096;
            importer.npotScale=TextureImporterNPOTScale.None;
            importer.wrapMode=TextureWrapMode.Clamp;
            importer.anisoLevel=4;
            importer.alphaSource=TextureImporterAlphaSource.FromInput;
            importer.alphaIsTransparency=false;
            importer.isReadable=false;
            importer.textureCompression=TextureImporterCompression.CompressedHQ;
            var platform=importer.GetPlatformTextureSettings("Standalone");
            platform.overridden=true;platform.maxTextureSize=4096;
            platform.format=t.role=="Normal"?TextureImporterFormat.BC5:TextureImporterFormat.BC7;
            platform.compressionQuality=100;
            importer.SetPlatformTextureSettings(platform);
            importer.SaveAndReimport();
            var texture=AssetDatabase.LoadAssetAtPath<Texture2D>(path);
            if(!texture||texture.width!=4096||texture.height!=4096)
                throw new InvalidDataException("PBR texture resolution changed: "+path);
            return texture;
        }
        public static string ReceiptPath => Path.Combine(Repository,"modern/out/unity-remaster/wuguan-training-hall/pbr-build-receipt.json");
        [Serializable] public sealed class BuildReceipt
        {
            public string completedUtc,prefabPath,prefabSha256,manifestSha256,materialSha256;
            public int textureCount;
        }
        public static string Digest(string absolute)
        {
            using(var stream=File.OpenRead(absolute))using(var hash=SHA256.Create())
                return BitConverter.ToString(hash.ComputeHash(stream)).Replace("-","");
        }
        public static void WriteBuildReceipt()
        {
            var manifest=LoadManifest();
            var prefab=AssetDatabase.LoadAssetAtPath<GameObject>(WuguanTrainingHallSetup.PrefabPath);
            var material=AssetDatabase.LoadAssetAtPath<Material>(MaterialPath);
            if(!prefab||!prefab.GetComponent<WuguanTrainingHallModule>().IsConfigured||!material)
                throw new InvalidDataException("Saved PBR prefab is not configured.");
            foreach(var r in prefab.GetComponentsInChildren<Renderer>(true))
                if(r.sharedMaterials.Any(m=>m!=material))throw new InvalidDataException("PBR material binding incomplete.");
            var receipt=new BuildReceipt {
                completedUtc=DateTime.UtcNow.ToString("O"),
                prefabPath="unity/MoxiangClient/"+WuguanTrainingHallSetup.PrefabPath,
                prefabSha256=Digest(Path.GetFullPath(WuguanTrainingHallSetup.PrefabPath)),
                manifestSha256=Digest(Path.GetFullPath(ManifestPath)),
                materialSha256=Digest(Path.GetFullPath(MaterialPath)),textureCount=manifest.textures.Length
            };
            Directory.CreateDirectory(Path.GetDirectoryName(ReceiptPath));
            File.WriteAllText(ReceiptPath,JsonUtility.ToJson(receipt,true));
            Debug.Log("MXH_WUGUAN_BUILD_RECEIPT prefab_hash="+receipt.prefabSha256);
        }
        public static void Apply(GameObject root)
        {
            var manifest=LoadManifest();
            var textures=manifest.textures.ToDictionary(t=>t.role,ImportTexture);
            var shader=Shader.Find("Universal Render Pipeline/Lit");
            if(!shader||!shader.isSupported)throw new InvalidDataException("URP Lit shader unavailable.");
            Directory.CreateDirectory(ArtPath+"/Materials");
            AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);
            var material=AssetDatabase.LoadAssetAtPath<Material>(MaterialPath);
            if(!material){material=new Material(shader);AssetDatabase.CreateAsset(material,MaterialPath);}
            material.shader=shader;
            material.name="Wuguan_PBR_Atlas";
            material.SetColor("_BaseColor",Color.white);
            material.SetTexture("_BaseMap",textures["BaseColor"]);
            material.SetTexture("_BumpMap",textures["Normal"]);
            material.SetTexture("_MetallicGlossMap",textures["MetallicSmoothness"]);
            material.SetFloat("_WorkflowMode",1);material.SetFloat("_Metallic",1);
            material.SetFloat("_Smoothness",1);material.SetFloat("_SmoothnessTextureChannel",0);
            material.SetFloat("_BumpScale",1);material.SetFloat("_Surface",0);material.SetFloat("_AlphaClip",0);
            material.EnableKeyword("_NORMALMAP");material.EnableKeyword("_METALLICSPECGLOSSMAP");
            material.DisableKeyword("_SPECULAR_SETUP");material.DisableKeyword("_SMOOTHNESS_TEXTURE_ALBEDO_CHANNEL_A");
            material.enableInstancing=true;material.renderQueue=-1;
            foreach(var renderer in root.GetComponentsInChildren<Renderer>(true))
                renderer.sharedMaterials=Enumerable.Repeat(material,renderer.sharedMaterials.Length).ToArray();
            WuguanHighLowSetup.Apply(material);
            EditorUtility.SetDirty(material);AssetDatabase.SaveAssets();
            Debug.Log("MXH_WUGUAN_PBR_READY atlas=4096 textures=3 source_hashes=verified");
        }
    }
}
