#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cImage.hpp"
#include "mxh/ui/cResourceManager.hpp"
#include "mxh/ui/cSpriteAtlas.hpp"
#include "mxh/ui/cWindowManager.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace {

struct DrawRect { float x, y, width, height; };

bool record_draw(void* ctx, void*, float x, float y, float width, float height,
                 float, float, float, float, std::uint32_t, int) {
    static_cast<std::vector<DrawRect>*>(ctx)->push_back(
        {x, y, width, height});
    return true;
}

void* record_sprite_path(void* ctx, const std::string& path) {
    auto* paths = static_cast<std::vector<std::string>*>(ctx);
    paths->push_back(path);
    static char sprite;
    return &sprite;
}

TEST(DialogLoaderAtlas, ScriptAtlasIndexAndHardPathIconAreDistinct) {
    const std::filesystem::path root = MXH_SOURCE_DIR "/data/PlayDH";
    ASSERT_TRUE(mxh::ui::cSpriteAtlas::getInstance().Init(root));
    ASSERT_TRUE(mxh::ui::cResourceManager::getInstance().InitScriptManager(root / "Image"));
    std::vector<std::string> paths;
    mxh::ui::cDialogLoader::SetSpriteLoader(&record_sprite_path, &paths);

    mxh::ui::cWindowManager windows;
    const auto loaded = mxh::ui::cDialogLoader::LoadOne(
        root / "Image" / "InterfaceScript" / "15.bin", windows,
        mxh::ui::ResolutionMode::Low800x600);
    EXPECT_TRUE(loaded.ok);
    EXPECT_GT(loaded.cimg_count, 0u);
    bool saw_atlas_21 = false;
    for (const auto& path : paths) {
        if (std::filesystem::path(path).filename() == "2.tif") {
            saw_atlas_21 = true;
        }
    }
    EXPECT_TRUE(saw_atlas_21)
        << "15.bin #BASICIMAGE index 21 must resolve through image_path.bin";

    const auto* hard_path_icon = mxh::ui::cDialogLoader::LoadLegacyImage(21);
    ASSERT_NE(hard_path_icon, nullptr);
    ASSERT_EQ(windows.dialogCount(), 1u);
    EXPECT_NE(hard_path_icon, windows.dialogs()[0]->basicImage())
        << "direct atlas and hard-path IDs must not share a cached image";
    bool saw_icon_atlas = false;
    for (const auto& path : paths) {
        if (std::filesystem::path(path).filename() == "1.tif") {
            saw_icon_atlas = true;
        }
    }
    EXPECT_TRUE(saw_icon_atlas)
        << "hard-path icon index 21 still resolves to 1.tif";

    std::vector<DrawRect> draws;
    mxh::ui::bindRenderer(&record_draw, &draws);
    windows.RenderAll();
    mxh::ui::bindRenderer(nullptr, nullptr);
    bool drew_natural_main_bar = false;
    for (const auto& draw : draws) {
        if (draw.x == 198.0f && draw.y == 558.0f &&
            draw.width == 14.0f && draw.height == 42.0f) {
            drew_natural_main_bar = true;
        }
    }
    EXPECT_TRUE(drew_natural_main_bar)
        << "15.bin main bar must draw its 14x42 source rect at natural size, "
           "not stretch it to the 602x42 input bounds";

    mxh::ui::cDialogLoader::SetSpriteLoader(nullptr, nullptr);
}

}  // namespace
