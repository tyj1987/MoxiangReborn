using UnityEditor;
using UnityEngine;

namespace Moxiang.Editor
{
    /// <summary>Deterministic import defaults for Asset Reset V2 runtime exports only.</summary>
    public sealed class MoxiangAssetPostprocessor : AssetPostprocessor
    {
        private bool Managed => MoxiangAssetPolicy.IsManagedAsset(assetPath);
        private MoxiangAssetCategory Category => MoxiangAssetPolicy.Classify(assetPath);

        private void OnPreprocessModel()
        {
            if (!Managed)
                return;

            var importer = (ModelImporter)assetImporter;
            var category = Category;
            importer.importCameras = false;
            importer.importLights = false;
            importer.importVisibility = false;
            importer.materialImportMode = ModelImporterMaterialImportMode.None;
            // Blender source is Z-up. Unity 6 keeps the FBX conversion on the model root
            // unless axis conversion is baked; that breaks modular placement and bounds.
            importer.bakeAxisConversion = true;
            importer.isReadable = false;
            importer.optimizeMeshPolygons = true;
            importer.optimizeMeshVertices = true;
            importer.generateSecondaryUV = category == MoxiangAssetCategory.Environment ||
                                           category == MoxiangAssetCategory.Terrain;
        }

        private void OnPreprocessTexture()
        {
            if (!Managed)
                return;

            var category = Category;
            var budget = MoxiangAssetPolicy.GetBudget(category);
            var importer = (TextureImporter)assetImporter;

            if (budget.MaxTextureSize > 0)
                importer.maxTextureSize = budget.MaxTextureSize;

            importer.isReadable = false;
            importer.textureCompression = TextureImporterCompression.CompressedHQ;
            importer.mipmapEnabled = category != MoxiangAssetCategory.Ui;

            if (category == MoxiangAssetCategory.Ui)
            {
                importer.textureType = TextureImporterType.Sprite;
                importer.spriteImportMode = SpriteImportMode.Single;
                importer.alphaIsTransparency = true;
                importer.sRGBTexture = true;
            }
            else if (MoxiangAssetPolicy.IsNormalTexture(assetPath))
            {
                importer.textureType = TextureImporterType.NormalMap;
                importer.sRGBTexture = false;
            }
            else if (MoxiangAssetPolicy.IsLinearDataTexture(assetPath))
            {
                importer.textureType = TextureImporterType.Default;
                importer.sRGBTexture = false;
            }

            if (budget.MaxTextureSize > 0)
            {
                var platform = importer.GetPlatformTextureSettings("Standalone");
                platform.overridden = true;
                platform.maxTextureSize = budget.MaxTextureSize;
                platform.format = TextureImporterFormat.Automatic;
                importer.SetPlatformTextureSettings(platform);
            }
        }

        private void OnPreprocessAudio()
        {
            if (!Managed || Category != MoxiangAssetCategory.Audio)
                return;

            var importer = (AudioImporter)assetImporter;
            importer.loadInBackground = true;
            var settings = importer.defaultSampleSettings;
            settings.preloadAudioData = true;
            settings.loadType = AudioClipLoadType.CompressedInMemory;
            settings.compressionFormat = AudioCompressionFormat.Vorbis;
            settings.quality = 0.7f;
            settings.sampleRateSetting = AudioSampleRateSetting.PreserveSampleRate;
            importer.defaultSampleSettings = settings;
        }
    }
}
