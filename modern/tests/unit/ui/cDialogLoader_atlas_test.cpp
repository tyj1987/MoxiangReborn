#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cImage.hpp"
#include "mxh/ui/cResourceManager.hpp"
#include "mxh/ui/cSpriteAtlas.hpp"
#include "mxh/ui/cWindowManager.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include "../../support/synthetic_fixture.hpp"

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

bool write_encrypted(mxh::test::SyntheticFixtureDir& dir,
                     const std::filesystem::path& relative,
                     std::string_view text) {
    const auto path = dir.write_bytes(relative, "x");
    if (path.empty()) return false;
    return mxh::compat::write_mh_bin(
               path, {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()}) ==
           mxh::compat::MhError::Ok;
}

TEST(DialogLoaderAtlas, ScriptAtlasIndexAndHardPathIconAreDistinct) {
    // Hand-written dialog, atlas list, and seven path tables. Values are
    // invented for this test and are not taken from a shipped profile.
    mxh::test::SyntheticFixtureDir dir;
    ASSERT_TRUE(dir.created());
    const char* kAtlas =
        "2\n"
        "0\t./image/2D/panel.tif\t64\t32\t0\n"
        "1\t./image/2D/icon.tif\t40\t40\t0\n";
    ASSERT_TRUE(write_encrypted(dir, "Image/image_path.bin", kAtlas));
    ASSERT_FALSE(dir.write_bytes("Image/2D/panel.tif", "panel").empty());
    ASSERT_FALSE(dir.write_bytes("Image/2D/icon.tif", "icon").empty());
    const char* kTables[] = {
        "image_hard_path.bin", "image_item_path.bin", "image_mugong_path.bin",
        "image_ability_path.bin", "image_buff_path.bin", "image_minimap_path.bin",
        "image_jackpot_path.bin"};
    for (const char* name : kTables) {
        const char* row = (std::string(name) == "image_hard_path.bin")
                              ? "4 0 1 1 9 9\n"
                              : "1 0 0 0 4 4\n";
        ASSERT_TRUE(write_encrypted(dir, std::filesystem::path("Image") / name, row))
            << name;
    }
    const char* kDialog =
        "$MAINDLG\n"
        "{\n"
        " #ID SYNTH_MAIN\n"
        " #POINT 30 40 90 40\n"
        " #POINT_ 11 22 90 40\n"
        " #BASICIMAGE ( 1 2 4 16 20 )\n"
        " #ACTIVE 1\n"
        " $BTN\n"
        " {\n"
        "  #ID SYNTH_BTN\n"
        "  #POINT 4 4 12 12\n"
        "  #BASICIMAGE ( 1 0 0 8 8 )\n"
        " }\n"
        "}\n";
    ASSERT_TRUE(write_encrypted(
        dir, "Image/InterfaceScript/synth_main.bin", kDialog));

    const auto root = dir.path();
    ASSERT_TRUE(mxh::ui::cSpriteAtlas::getInstance().Init(root));
    ASSERT_TRUE(mxh::ui::cResourceManager::getInstance().InitScriptManager(root / "Image"));
    std::vector<std::string> paths;
    mxh::ui::cDialogLoader::SetSpriteLoader(&record_sprite_path, &paths);

    mxh::ui::cWindowManager windows;
    const auto loaded = mxh::ui::cDialogLoader::LoadOne(
        root / "Image" / "InterfaceScript" / "synth_main.bin", windows,
        mxh::ui::ResolutionMode::Low800x600);
    EXPECT_TRUE(loaded.ok) << loaded.error;
    EXPECT_GT(loaded.cimg_count, 0u);
    bool saw_icon = false;
    for (const auto& path : paths) {
        if (std::filesystem::path(path).filename() == "icon.tif") saw_icon = true;
    }
    EXPECT_TRUE(saw_icon);

    const auto* hard_path_icon = mxh::ui::cDialogLoader::LoadLegacyImage(4);
    ASSERT_NE(hard_path_icon, nullptr);
    ASSERT_EQ(windows.dialogCount(), 1u);
    EXPECT_NE(hard_path_icon, windows.dialogs()[0]->basicImage());
    bool saw_panel = false;
    for (const auto& path : paths) {
        if (std::filesystem::path(path).filename() == "panel.tif") saw_panel = true;
    }
    EXPECT_TRUE(saw_panel);

    std::vector<DrawRect> draws;
    mxh::ui::bindRenderer(&record_draw, &draws);
    windows.RenderAll();
    mxh::ui::bindRenderer(nullptr, nullptr);
    bool drew_natural = false;
    for (const auto& draw : draws) {
        if (draw.x == 11.0f && draw.y == 22.0f &&
            draw.width == 14.0f && draw.height == 16.0f) {
            drew_natural = true;
        }
    }
    EXPECT_TRUE(drew_natural);

    mxh::ui::cDialogLoader::SetSpriteLoader(nullptr, nullptr);
    mxh::ui::cSpriteAtlas::getInstance().Release();
    mxh::ui::cResourceManager::getInstance().ReleaseScriptManager();
}

}  // namespace
