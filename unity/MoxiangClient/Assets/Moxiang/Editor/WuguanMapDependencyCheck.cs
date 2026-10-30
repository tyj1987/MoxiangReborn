using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;
namespace Moxiang.Editor
{
    public static class WuguanMapDependencyCheck
    {
        public static void Ensure(){try{Check();}catch(InvalidDataException){Reimport();}}
        public static void Reimport()
        {
            foreach(var number in new[]{10,2})
            {
                string root="Assets/Moxiang/Derived/Map"+number;
                foreach(var dependency in Directory.GetFiles(root,"*.mxhdds",SearchOption.AllDirectories))
                    AssetDatabase.ImportAsset(dependency.Replace('\\','/'),ImportAssetOptions.ForceUpdate|ImportAssetOptions.ForceSynchronousImport);
                AssetDatabase.ImportAsset(root+"/Map"+number+".mxhasset",ImportAssetOptions.ForceUpdate|ImportAssetOptions.ForceSynchronousImport);
                AssetDatabase.ImportAsset(root+"/Map"+number+".mxhterrain",ImportAssetOptions.ForceUpdate|ImportAssetOptions.ForceSynchronousImport);
            }
            Check();
        }
        public static void Check()
        {
            foreach(var number in new[]{10,2})
            {
                string root="Assets/Moxiang/Derived/Map"+number+"/Map"+number;
                var field=AssetDatabase.LoadAssetAtPath<ImportedHeightField>(root+".mxhasset");
                var terrain=AssetDatabase.LoadAssetAtPath<GameObject>(root+".mxhterrain");
                if(!field||!field.inspectionMesh||!terrain)
                    throw new InvalidDataException("Existing Map"+number+" importer output unavailable; reimport verified dependencies, do not save a broken main scene.");
                AssetDatabase.TryGetGUIDAndLocalFileIdentifier(terrain,out string guid,out long id);
                Debug.Log("MXH_EXISTING_MAP_READY map="+number+" guid="+guid+" id="+id+" chunks="+terrain.transform.childCount);
            }
        }
    }
}
