// cstatusicondlg.hpp — modern port of 墨香 CStatusIconDlg (status icon stack).
//
// 1:1 port of legacy `CStatusIconDlg` from
//   `墨香【源码】\[Client]MH\StatusIconDlg.{h,cpp}`.
//
// IMPORTANT: the legacy `CStatusIconDlg` is **not** a `cDialog`
// subclass -- it's a free-floating stack of status icons drawn
// at a specific screen position next to the target object.
// The 1:1 port preserves that: cStatusIconDlg owns a render
// loop over the icon array and exposes a static USINGTON-style
// accessor (`StatusIconDlg()` returns a process-wide singleton
// pointer, mirroring the legacy EXTERNGLOBALTON macro).
//
// The class also keeps:
//   * m_MaxDesc / m_pDescriptionArray -- per-icon description
//     text loaded from ListStatusIcon.txt / .bin
//   * m_MaxIconPerLine              -- wrap-after-N icons
//   * m_CurIconNum                   -- how many icons are in
//                                       the array
//   * m_dwRemainTime[] / m_dwStartTime[] -- per-icon timer
//   * ICONRENDERINFO                 -- per-icon render state
//                                       (alpha, plus-flashing)
//
// The modern port defers the actual cImage / cImageSelf render
// (the legacy cImageSelf::RenderSprite call) and surfaces a
// single Draw callback the host can use to paint icons into its
// own render loop.

#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace mxh::ui {

// 1:1 with legacy eStatusIcon enum (read out of CommonStruct.h
// in the legacy code; values preserved verbatim).
enum StatusIconKind : std::int32_t {
    eStatusIcon_Other          = 0,
    eStatusIcon_Poison         = 1,
    eStatusIcon_Fire           = 2,
    eStatusIcon_Ice            = 3,
    eStatusIcon_Lightning      = 4,
    eStatusIcon_Stun           = 5,
    eStatusIcon_Slow           = 6,
    eStatusIcon_DamageUp       = 7,
    eStatusIcon_DefenceUp      = 8,
    eStatusIcon_AttackUp       = 9,
    eStatusIcon_ExpUp          = 10,
    eStatusIcon_DropUp         = 11,
    eStatusIcon_AllUp          = 12,
    eStatusIcon_Count          = 13,   // sentinel
    eStatusIcon_Max             = eStatusIcon_Count,
};

// 1:1 with legacy ICONRENDERINFO.
struct IconRenderInfo {
    std::uint32_t ItemIndex = 0;     // 1:1 with legacy DWORD ItemIndex
    bool          bPlus     = false;  // 1:1 with legacy BOOL bPlus
    bool          bAlpha    = false;  // 1:1 with legacy BOOL bAlpha
    std::int32_t  Alpha     = 0;      // 1:1 with legacy int Alpha
};

// 1:1 with legacy StaticString (from the legacy
// cNameValuePair / StaticString helpers).  Per-icon description
// key-value store.
struct StaticString {
    char key[16]   = {};
    char value[64] = {};
};

class cStatusIconDlg {
public:
    cStatusIconDlg();
    ~cStatusIconDlg();

    cStatusIconDlg(const cStatusIconDlg&) = delete;
    cStatusIconDlg& operator=(const cStatusIconDlg&) = delete;

    // 1:1 with legacy Init.  Modern port takes the CObject*
    // as void* (the legacy CObject class isn't compiled into
    // the modern port) and a position + wrap width.  The
    // host adapter casts the void* back to CObject* if needed.
    void Init(void* pObject, std::int32_t x, std::int32_t y,
              std::int32_t maxIconPerLine);

    // 1:1 with legacy Release.  Clears the icon arrays + the
    // description table.
    void Release();

    // 1:1 with legacy LoadDescription.  Reads the
    // ListStatusIcon.txt / .bin resource.  The actual file
    // load is deferred (CMHFile isn't fully wired up in the
    // modern port); tests inject descriptions via
    // AddDescriptionForTest.
    void LoadDescription();

    // 1:1 with legacy AddIcon(CObject*, WORD StatusIconNum,
    // WORD ItemIdx, DWORD dwRemainTime).
    void AddIcon(void* pObject, std::uint16_t statusIconNum,
                 std::uint16_t itemIdx,
                 std::uint32_t dwRemainTime = 0);

    // 1:1 with legacy AddQuestTimeIcon.
    void AddQuestTimeIcon(void* pObject, std::uint16_t statusIconNum);

    // 1:1 with legacy RemoveIcon.  Removes the first matching
    // icon for (pObject, StatusIconNum, ItemIdx).  If multiple
    // icons exist for the same (pObject, StatusIconNum), all of
    // them are removed when ItemIdx == 0; otherwise only the
    // first matching ItemIdx is removed.
    void RemoveIcon(void* pObject, std::uint16_t statusIconNum,
                    std::uint16_t itemIdx = 0);

    // 1:1 with legacy RemoveQuestTimeIcon.
    void RemoveQuestTimeIcon(void* pObject, std::uint16_t statusIconNum);

    // 1:1 with legacy RemoveAllQuestTimeIcon.
    void RemoveAllQuestTimeIcon();

    // 1:1 with legacy SetOneMinuteToShopItem -- the modern
    // port keeps the API surface; the actual dwRemainTime
    // override happens on the next AddIcon call (legacy:
    // the shop-item icon's remaining time is clamped to 60s
    // when re-added).
    void SetOneMinuteToShopItem(std::uint32_t itemIdx);

    // 1:1 with legacy Render.  The modern port fires a
    // per-icon callback the host can use to paint into its
    // own render loop.  The legacy's cImageSelf::RenderSprite
    // is replaced with a void* + pos + scale signature; the
    // host's renderer recognises the void* as a legacy
    // cImageSelf*.
    struct RenderCtx {
        std::int32_t drawX      = 0;
        std::int32_t drawY      = 0;
        std::int32_t maxPerLine = 0;
        std::int32_t curIdx     = 0;     // 0..m_CurIconNum-1
        std::uint16_t iconKind  = 0;     // StatusIconKind
        std::uint16_t itemIdx   = 0;
        bool          bPlus     = false;
        bool          bAlpha    = false;
        std::int32_t  alpha     = 0;
    };
    using DrawIconCallback = std::function<void(const RenderCtx&)>;
    void SetOnDrawIcon(DrawIconCallback cb) noexcept { m_onDrawIcon = std::move(cb); }
    void Render();

    // 1:1 with legacy AddQuestIconCount.
    void AddQuestIconCount() noexcept { ++m_nQuestIconCount; }
    std::int32_t GetQuestIconCount() const noexcept { return m_nQuestIconCount; }

    // Test / introspection accessors.
    std::int32_t CurIconNum() const noexcept { return m_CurIconNum; }
    std::int32_t MaxIconPerLine() const noexcept { return m_MaxIconPerLine; }
    void*         BoundObject() const noexcept   { return m_pObject; }
    std::int32_t  IconCount() const noexcept     { return static_cast<std::int32_t>(m_IconCount.size()); }
    std::int32_t  DescriptionCount() const noexcept { return m_MaxDesc; }

    // Test hook -- register a description entry (replaces the
    // legacy LoadDescription file load).
    void AddDescriptionForTest(const char* key, const char* value);

private:
    struct IconEntry {
        std::uint16_t statusIconNum = 0;
        std::uint16_t itemIdx       = 0;
        std::uint32_t dwRemainTime  = 0;
        std::uint32_t dwStartTime   = 0;
    };

    void*  m_pObject       = nullptr;
    std::int32_t m_DrawPositionX = 0;
    std::int32_t m_DrawPositionY = 0;
    std::int32_t m_MaxIconPerLine = 0;
    std::int32_t m_CurIconNum   = 0;

    std::vector<std::uint16_t> m_IconCount;            // [eStatusIcon_Max]
    std::vector<IconRenderInfo> m_IconInfo;            // [eStatusIcon_Max]
    std::vector<std::uint32_t> m_dwRemainTime;         // [eStatusIcon_Max]
    std::vector<std::uint32_t> m_dwStartTime;          // [eStatusIcon_Max]

    std::int32_t m_MaxDesc = 0;
    StaticString* m_pDescriptionArray = nullptr;

    std::int32_t  m_nQuestIconCount = 0;

    DrawIconCallback m_onDrawIcon;
};

} // namespace mxh::ui
