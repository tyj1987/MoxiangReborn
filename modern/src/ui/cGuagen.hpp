// cGuagen.hpp — modern port of 墨香 cGuagen (progress bar / gauge).
//
// 1:1 port of legacy `cGuagen` from
//   `墨香【源码】\[Client]MH\interface\cGuagen.h`.
//
// A cGuagen is a horizontal progress bar widget. It owns:
//   - a piece image (cImage) drawn as the bar fill
//   - a percentage rate (0.0 .. 1.0) controlling how much of the bar
//     is filled
//   - per-axis width / height scale to size the piece image
//   - an offset (m_imgRelPos) for placing the piece relative to the
//     widget's own position
//
// Render is a no-op (the actual draw goes through the 6.4 cImage seam
// + Phase 5 sprite renderer; this skeleton just stores the data).
//
// Reference usages in legacy (for P2-12 dialog porting context):
//   - CharacterDialog (HP/MP/Exp bar widgets)
//   - InventoryExDialog (durability bar)
//   - GuildDialog (guild level bar)
//   - ProgressDialog (loading progress)
//   - QuestDialog (quest progress)

#pragma once

#include "cWindow.hpp"
#include "cImage.hpp"

#include <cstdint>

namespace mxh::ui {

class cGuagen : public cWindow {
public:
    cGuagen();
    ~cGuagen() override;

    void Render() override;

    // 0.0 .. 1.0; values > 1.0 are clamped to 1.0.
    void SetValue(float val);
    float GetValue() const noexcept { return m_fPercentRate; }

    void SetGuageImagePos(std::int32_t imgX, std::int32_t imgY);
    void SetGuageImagePos(float imgX, float imgY);

    void SetPieceImage(const cImage& piece) noexcept { m_GuagePieceImage = piece; }
    const cImage& GetPieceImage() const noexcept { return m_GuagePieceImage; }

    void SetGuageWidth(float width) noexcept { m_fGuageWidth = width; }
    float GetGuageWidth() const noexcept { return m_fGuageWidth; }

    void SetGuagePieceWidth(float width) noexcept { m_fGuagePieceWidth = width; }
    float GetGuagePieceWidth() const noexcept { return m_fGuagePieceWidth; }

    void SetGuagePieceHeightScale(float hei) noexcept { m_fGuagePieceHeightScaleY = hei; }
    float GetGuagePieceHeightScale() const noexcept { return m_fGuagePieceHeightScaleY; }

    // Piece image draw offset (legacy stored this as VECTOR2 — modern
    // port keeps it as two floats for direct SetAbsXY integration).
    float GetImageRelX() const noexcept { return m_imgRelPos.x; }
    float GetImageRelY() const noexcept { return m_imgRelPos.y; }

private:
    cImage m_GuagePieceImage;
    struct { float x; float y; } m_imgRelPos{0.0f, 0.0f};

    float m_fGuageWidth{0.0f};
    float m_fGuagePieceWidth{0.0f};
    float m_fPercentRate{0.0f};
    float m_fGuagePieceHeightScaleY{1.0f};
};

}  // namespace mxh::ui
