// mxh/ui/cWindow.hpp
// Phase 6.0 — modern C++ rewrite of legacy [Client]MH/interface/cWindow.h.
//
// Surface intent: keep the legacy 1:1 API (Init / Render / ActionEvent /
// PtInWindow / Add / SetFocus / SetMovable / SetDepend / ...) but in a
// header-only modern C++ form that compiles without MFC and is testable
// CPU-side. The actual GPU render path + cImage integration lands in
// Phase 6.1.2; this skeleton exercises the framework contracts only.
//
// Public API mirrors the legacy cWindow closely. Differences are limited to:
//   * No virtual ToolTipRender (overlays land in Phase 6.1.x)
//   * No CMouse* / CKeyboard* (we pass primitive coords + a flags bitmask;
//     a real input layer adapter wraps these in 6.1.x)
//   * cImage is passed as opaque `void*` (cImage lives in the render module
//     and gets wired in 6.1.2; the framework doesn't depend on it)
//   * `Add` is non-virtual and owns the child via unique_ptr
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "cObject.hpp"

namespace mxh::ui {

class cWindow : public cObject {
public:
    // -------------------------------------------------------------------------
    // Window event codes (mirrors the legacy WE_* enum used by the engine).
    // We define only the subset the framework itself produces / routes; the
    // full legacy enum is added in 6.1.x as control widgets are ported.
    // -------------------------------------------------------------------------
    enum class WindowEvent : std::uint32_t {
        Null          = 0,    // WE_NULL — no event consumed
        MouseMove     = 1,    // WE_MOUSEOVER
        LButtonDown   = 2,    // WE_LBTNCLICK is the click event (down+up at same pos)
        LButtonUp     = 3,
        LButtonClick  = 4,
        RButtonDown   = 5,
        RButtonUp     = 6,
        RButtonClick  = 7,
        KeyDown       = 10,
        Char_         = 11,
    };

    // Mouse flags (mirrors legacy `DWORD we` bit field; we only need the
    // common L/R button state bits for hit dispatch in this skeleton).
    static constexpr std::uint32_t MouseFlagLButton = 0x0001u;
    static constexpr std::uint32_t MouseFlagRButton = 0x0002u;
    static constexpr std::uint32_t MouseFlagShift   = 0x0010u;
    static constexpr std::uint32_t MouseFlagControl = 0x0020u;

    cWindow() = default;
    ~cWindow() override = default;

    cWindow(const cWindow&) = delete;
    cWindow& operator=(const cWindow&) = delete;

    // -------------------------------------------------------------------------
    // Legacy Init(x, y, wid, hei, basicImage, ID) — stores position / size /
    // image pointer and the widget id. The image is kept as opaque `void*` in
    // the skeleton; 6.1.2 swaps in cImage* once the rendering path lands.
    // -------------------------------------------------------------------------
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid, std::uint16_t hei,
              void* basicImage = nullptr, std::int32_t id = 0);

    // Render — Phase A.1.5 wiring.  Real GPU draw goes through cImage
    // (m_basicImage, cast from void*) and recurses into children.  The
    // legacy cWindow::Render() walked the child tree depth-first, drawing
    // each child's basicImage (and the dialog's chrome) before the
    // children themselves.  We preserve that order here.
    //
    // 1:1 quirk: the legacy engine sets m_absX / m_absY as the screen-
    // space origin, so children's absolute coords include the parent's
    // offset by the time the renderer sees them.  We honour that
    // contract by adding the parent's absX/absY when recursing.
    //
    // Virtual so subclasses (cDialog, cButton, cMsgBox, ...) can
    // override with their own draw tree.
    virtual void Render();

    // Hit test: returns true if (x,y) in absolute coordinates is inside the
    // window's bounding rectangle. Inclusive on the boundary, matching the
    // legacy engine's "the click on the edge still counts" contract.
    bool PtInWindow(std::int32_t x, std::int32_t y) const noexcept;

    // Position / size / focus / state setters.
    virtual void SetAbsXY(std::int32_t x, std::int32_t y) noexcept;
    void SetRelXY(std::int32_t x, std::int32_t y) noexcept;
    void SetValidXY(std::int32_t x, std::int32_t y) noexcept;
    void SetWH(std::int32_t w, std::int32_t h) noexcept;
    void SetFocus(bool v) noexcept        { m_bFocus    = v; }
    void SetMovable(bool v) noexcept      { m_bMovable  = v; }
    void SetDepend(bool v) noexcept       { m_bDepend   = v; }
    void SetVisible(bool v) noexcept      { m_bVisible  = v; }
    virtual void SetEnabled(bool v) noexcept { m_bEnabled  = v; }
    // Legacy alias: the original cWindow exposes SetDisable(BOOL); we
    // route it through SetEnabled so the dispatcher / engine-facing code
    // can use either name.
    virtual void SetDisable(bool v) noexcept { SetEnabled(!v); }
    void SetBasicImage(void* img) noexcept { m_basicImage = img; }
    void SetImageRGB(std::uint32_t color) noexcept { m_imageRGB = color; }
    std::uint32_t imageRGB() const noexcept { return m_imageRGB; }

    // Read accessors.
    std::int32_t  absX() const noexcept  { return m_absX; }
    std::int32_t  absY() const noexcept  { return m_absY; }
    std::int32_t  relX() const noexcept  { return m_relX; }
    std::int32_t  relY() const noexcept  { return m_relY; }
    std::int32_t  validX() const noexcept { return m_validX; }
    std::int32_t  validY() const noexcept { return m_validY; }
    std::uint16_t width() const noexcept { return m_w; }
    std::uint16_t height() const noexcept { return m_h; }
    bool   hasFocus() const noexcept  { return m_bFocus; }
    bool   isMovable() const noexcept { return m_bMovable; }
    bool   isDepend() const noexcept  { return m_bDepend; }
    bool   isVisible() const noexcept { return m_bVisible; }
    bool   isEnabled() const noexcept { return m_bEnabled; }
    void*  basicImage() const noexcept { return m_basicImage; }

    // Active state. 1:1 with legacy cWindow::SetActive / IsActive --
    // toggles per-child visibility / dispatch for non-dialog
    // children (cStatic / cTextArea / cButton / cEditBox / etc.).
    // cDialog overrides SetActive to also flip m_bActive; the
    // base no-op is enough for control windows in Phase 6.4+
    // (render path is deferred).
    virtual void SetActive(bool v) noexcept        { m_bActive = v; }
    bool         isActive() const noexcept        { return m_bActive; }

    // -------------------------------------------------------------------------
    // Child management. `Add` takes ownership (unique_ptr) and parent-links
    // the child. Mirrors the legacy `virtual void Add(cWindow* wnd)` contract
    // but the modern API enforces ownership statically.
    // -------------------------------------------------------------------------
    void Add(std::unique_ptr<cWindow> child);

    std::size_t childCount() const noexcept { return m_children.size(); }
    cWindow*    childAt(std::size_t i) const noexcept;
    // Steal child back (used by tests; production code rarely needs this).
    std::unique_ptr<cWindow> removeChildAt(std::size_t i);

    // -------------------------------------------------------------------------
    // Event dispatch.
    //
    // The skeleton implements top-down dispatch: if the (x, y) hits a child,
    // ActionEvent recurses into the topmost child first; otherwise the parent
    // itself consumes the event. Returns a legacy WE_* code (0 = WE_NULL = no
    // event). Full event semantics (focus chain, capture, tooltip) land in 6.1.
    //
    // mouseFlags carries the same bits as legacy `DWORD we`; only the L/R
    // button state is interpreted here.
    //
    // ActionKeyboardEvent takes a virtual-key code (`key`) and a character
    // payload (`ch`, 0 for non-character keys). The base class is a no-op;
    // control widgets (cEditBox, etc.) override to consume keys.
    // -------------------------------------------------------------------------
    virtual std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                      std::uint32_t mouseFlags);
    virtual std::uint32_t ActionKeyboardEvent(std::int32_t /*key*/,
                                               std::int32_t /*ch*/) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }

private:
    std::int32_t  m_absX     = 0;
    std::int32_t  m_absY     = 0;
    std::int32_t  m_relX     = 0;
    std::int32_t  m_relY     = 0;
    std::int32_t  m_validX   = 0;
    std::int32_t  m_validY   = 0;
    std::uint16_t m_w        = 0;
    std::uint16_t m_h        = 0;

    void* m_basicImage = nullptr;  // opaque in skeleton (cImage* in 6.1.2)
    std::uint32_t m_imageRGB = 0xFFFFFFFFu;

    bool m_bFocus   = false;
    bool m_bMovable = false;
    bool m_bDepend  = false;
    bool m_bVisible = true;
    bool m_bEnabled = true;
    bool m_bActive  = false;  // 1:1 with legacy cWindow active state

    // Owning children. Insertion is O(1) amortized; topmost is m_children.back().
    std::vector<std::unique_ptr<cWindow>> m_children;
};

} // namespace mxh::ui
