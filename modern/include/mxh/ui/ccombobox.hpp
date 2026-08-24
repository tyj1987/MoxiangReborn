// ccombobox.hpp — modern port of 墨香 cComboBox (combo box with
// dropdown list of selectable items).
//
// 1:1 port of legacy `cComboBox` from
//   `墨香【源码】\[Client]MH\Interface\cComboBox.{h,cpp}` (6.8 KB
//   legacy code). The modern port keeps the data model + state
//   machine + selection logic and stubs the engine-side
//   dispatch (cbWindowFunc, mouse dispatch, cImage sprite draw)
//   with no-ops until Phase 13+ real impl lands.
//
// The legacy `cComboBox : public cWindow, public cListItem` uses
// multiple inheritance (one for cWindow's geometry, one for
// cListItem's item-list management). The modern port uses
// COMPOSITION instead of multiple inheritance — cListItem is a
// base class with the same data model (item list + max-line cap)
// but the inheritance introduces a fragile diamond if any future
// sub-widget needs both. Composition is the modern idiom and
// keeps the test surface simple.
//
// Modern port scope (this commit):
//   - List model: m_items (std::vector<ITEM>), m_maxLine cap
//     (legacy drops head when over-cap; 1:1 with legacy AddItem
//     and AddItem(idx)).
//   - Selection: m_nCurSelectedIdx (-1 = none), m_nOverIdx
//     (hover-row index, only valid when dropdown is open).
//   - Combo button: m_pComboBtn (a cPushupButton* that drives
//     the dropdown). The modern port stores the pointer as
//     opaque (the cPushupButton port has SetPush / IsPushed).
//   - Combo text: m_comboText (modern std::string; legacy used
//     char[MAX_COMBOTEXT_SIZE]).
//   - Image slots: 4 cImage (top / middle / down / over) for the
//     dropdown sprite. Modern port stores void* placeholders
//     (real cImage binds with the 6.6 cImage seam).
//   - Layout: m_listWidth, m_topHeight, m_middleHeight,
//     m_downHeight, m_textClippingRect.
//   - Init / InitComboList / Add (link the cPushupButton) /
//     SetAbsXY / ActionEvent / ListMouseCheck /
//     PtIdxInComboList / SetMargin / GetComboText /
//     SelectComboText / GetCurSelectedIdx / SetCurSelectedIdx
//     / SetComboTextColor / SetOverImageScale. All 1:1 with
//     legacy surface; the engine-side dispatch (cbWindowFunc +
//     cWindowManager + cImage render) is stubbed.
//
// Modern-port simplifications (all documented in the .cpp file
// header):
// 1. **cListItem is composed, not inherited.** Legacy uses
//    multiple inheritance. Modern port has cListItem as a
//    base class for cListDialog-style widgets (where the
//    cListDialog wants the item list without cWindow geometry),
//    and cComboBox has it as a private base (matching the
//    legacy's "private" multiple-inheritance via cListItem).
// 2. **cPushupButton is opaque.** The modern cPushupButton is
//    ported (cPushupButton has SetPush / IsPushed); the
//    combo's `Add(cWindow* pushupBtn)` checks the type and
//    casts. Modern port stores a void* (cWindow*) to avoid
//    tight coupling — when the engine-binder layer lands, the
//    pointer is reinterpret_cast to cPushupButton* for the
//    SetPush / IsPushed calls.
// 3. **cImage is opaque.** The 4 image slots (top / middle /
//    down / over) are stored as void*. Real cImage binds with
//    6.6 cImage seam.
// 4. **Render is a no-op.** The legacy Render draws the
//    dropdown list (top + middle + down sprites + per-item
//    text + over-image on hover). All of that needs cImage
//    seam + font renderer. Modern port is no-op; the data
//    model + state is fully testable.
// 5. **ActionEvent is a no-op stub.** The legacy
//    cbWindowFunc dispatch has no modern equivalent (the
//    dispatcher integration is 6.6 follow-up).
// 6. **Engine singletons stubbed.** cWindowManager->IsMouseOverUsed
//    / IsMouseDownUsed / SetMouseOverUsed / SetMouseDownUsed
//    are all no-op. The data-side state (select-idx / over-idx /
//    text) is preserved 1:1.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this is a
// Tier 1.5 subcontrol port (0.13.48 prerequisite). It unblocks
// cStallFindDlg (Tier 2, 0.13.48 main port) + any cComboBox-
// dependent dialog (no others in the current P2-12 backlog).

#pragma once

#include "cListItem.hpp"  // composed base (ComboItem defined here)

#include <cstdint>
#include <string>

namespace mxh::ui {

class cPushupButton;

class cComboBox : public cListItem {
public:
    cComboBox();
    ~cComboBox() override;

    cComboBox(const cComboBox&) = delete;
    cComboBox& operator=(const cComboBox&) = delete;

    // -------------------------------------------------------------------------
    // Init: position, size, basic image, callback, id. 1:1 with
    // legacy cComboBox::Init. The `cbFUNC` callback is stubbed
    // (modern cDialog has no static-function-pointer seam; the
    // equivalent is a per-instance std::function set via
    // SetOnAction — 6.6 follow-up).
    // -------------------------------------------------------------------------
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
              std::uint16_t hei, void* basicImage = nullptr,
              std::int32_t id = 0);

    // InitComboList: 1:1 with legacy. Stores the 4 image slots
    // (top / middle / down / over) and the per-image heights.
    // The legacy cImage* is opaque in modern (void*).
    void InitComboList(std::uint16_t listWid,
                       void* topImage,   std::uint16_t topHei,
                       void* middleImage, std::uint16_t middleHei,
                       void* downImage,  std::uint16_t downHei,
                       void* overImage);

    // Render no-op (cImage seam 6.6).
    void Render() override {}

    // ActionEvent: 1:1 with legacy. Modern port is a no-op
    // (engine-side dispatch is stubbed; data-side effects are
    // preserved).
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // Add: 1:1 with legacy. The legacy checks the pushupBtn's
    // type == WT_PUSHUPBUTTON and casts to cPushupButton*. Modern
    // port stores the pointer as void*; the type check is
    // documented as a no-op (the engine-binder layer will re-add
    // it).
    void Add(cWindow* pushupBtn);

    // SetAbsXY: 1:1 with legacy. Cascades to the pushup button
    // (relative position + new abs).
    void SetAbsXY(std::int32_t x, std::int32_t y) noexcept override;

    // ListMouseCheck: 1:1 with legacy. Modern port is a no-op
    // (engine-side mouse dispatch is stubbed; data-side state
    // — m_nOverIdx + m_nCurSelectedIdx + m_comboText — is
    // preserved).
    void ListMouseCheck(std::int32_t mouseX, std::int32_t mouseY,
                        bool leftDown);

    // PtIdxInComboList: 1:1 with legacy. Returns the row index
    // for the given screen (x, y), or "no hit" (GetItemCount()+1)
    // if the click falls outside the list rect.
    std::uint16_t PtIdxInComboList(std::int32_t x, std::int32_t y) const;

    // -------------------------------------------------------------------------
    // Setters / getters.
    // -------------------------------------------------------------------------
    void SetComboTextColor(std::uint32_t color) noexcept { m_comboTextColor = color; }
    void SetOverImageScale(float x, float y) noexcept  { m_overImageScaleX = x; m_overImageScaleY = y; }
    void SetMargin(std::int32_t left, std::int32_t top,
                   std::int32_t right, std::int32_t bottom) noexcept;

    const std::string& GetComboText() const noexcept { return m_comboText; }
    void               SelectComboText(std::uint16_t idx);

    int GetCurSelectedIdx() const noexcept     { return m_nCurSelectedIdx; }
    void SetCurSelectedIdx(int idx) noexcept   { m_nCurSelectedIdx = idx; }

    int  GetOverIdx() const noexcept            { return m_nOverIdx; }
    void SetOverIdx(int idx) noexcept           { m_nOverIdx = idx; }

    // Push-up button access (1:1 with legacy m_pComboBtn).
    cWindow* GetComboBtn() const noexcept       { return m_pComboBtn; }

    // -------------------------------------------------------------------------
    // Image slots (1:1 with legacy m_TopImage / m_MiddleImage /
    // m_DownImage / m_OverImage). void* placeholders for the
    // 6.6 cImage seam.
    // -------------------------------------------------------------------------
    void* GetTopImage()   const noexcept { return m_topImage; }
    void* GetMiddleImage() const noexcept { return m_middleImage; }
    void* GetDownImage()   const noexcept { return m_downImage; }
    void* GetOverImage()  const noexcept { return m_overImage; }

    // Height accessors (1:1 with legacy m_topHeight / m_middleHeight
    // / m_downHeight).
    std::uint16_t GetTopHeight()   const noexcept { return m_topHeight; }
    std::uint16_t GetMiddleHeight() const noexcept { return m_middleHeight; }
    std::uint16_t GetDownHeight()   const noexcept { return m_downHeight; }

    // Text-clipping rect (1:1 with legacy m_textClippingRect).
    struct TextClippingRect {
        std::int32_t left, top, right, bottom;
    };
    TextClippingRect GetTextClippingRect() const noexcept { return m_textClippingRect; }

    // List width (1:1 with legacy m_listWidth).
    std::uint16_t GetListWidth() const noexcept { return m_listWidth; }

    // Test-only: get the items vector.
    const std::vector<ComboItem>& ItemsForTesting() const noexcept { return m_items; }

    // Constants.
    static constexpr std::size_t MAX_COMBOTEXT_SIZE = 256;

private:
    cWindow* m_pComboBtn = nullptr;  // 1:1 with legacy m_pComboBtn (opaque)

    std::string m_comboText;
    std::uint32_t m_comboTextColor = 0xFFFFFFFFu;

    // 4 image slots.
    void* m_topImage    = nullptr;
    void* m_middleImage = nullptr;
    void* m_downImage   = nullptr;
    void* m_overImage   = nullptr;
    std::uint16_t m_topHeight    = 0;
    std::uint16_t m_middleHeight = 0;
    std::uint16_t m_downHeight   = 0;

    float m_overImageScaleX = 1.0f;
    float m_overImageScaleY = 1.0f;

    int m_nOverIdx = -1;

    std::uint16_t m_listWidth = 0;
    TextClippingRect m_textClippingRect{3, 4, 0, 0};

    int m_nCurSelectedIdx = -1;

    // Items are inherited from cListItem (the base class).
    // (1:1 with legacy cListItem::m_ListItem, stored as
    // std::vector<ComboItem>.)
};

} // namespace mxh::ui
