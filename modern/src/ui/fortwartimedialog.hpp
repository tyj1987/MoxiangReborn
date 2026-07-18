// fortwartimedialog.hpp — modern port of 墨香 FortWarDialog.h (subset)
//
// 1:1 port of TWO classes from legacy
//   `墨香【源码】\[Client]MH\FortWarDialog.h`:
//     - CFWEngraveDialog (engrave-in-progress bar)
//     - CFWTimeDialog    (siege-war timer + character name)
//
// The third class in the legacy header (CFWWareHouseDialog) extends
// cTabDialog and depends on cIconGridDialog + cItem + cMsgBox + cDivideBox —
// NOT ported here. R-12.x deferred.
//
// Both ported classes follow the cProgressBarDlg + cStallKindSelectDlg
// pattern: Linking + 1-2 method 1:1 with legacy body, ActionEvent /
// OnActionEvent stubs (gCurTime + CHATMGR + NETWORK + HERO singletons
// unported).

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

class cStatic;
class cObjectGuagen;

// ---------------------------------------------------------------------------
// CFWEngraveDialog — engrave-in-progress bar
// ---------------------------------------------------------------------------
class cFWEngraveDialog : public cDialog {
public:
    // Local id range 1:1 with legacy FW_* enum (rebased to 780..781).
    static constexpr int kIdEngraveGuage  = 780;
    static constexpr int kIdRemaintimeText = 781;

    cFWEngraveDialog();
    ~cFWEngraveDialog() override;

    void Linking();
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
    void SetActiveWithTime(bool val, std::uint32_t dwTime);

    // Test accessors.
    const cObjectGuagen* GetEngraveGuage() const noexcept { return m_pEngraveGuage.get(); }
    const cStatic* GetRemaintimeStatic() const noexcept    { return m_pRemaintimeStatic.get(); }
    std::uint32_t GetProcessTime() const noexcept          { return m_dwProcessTime; }
    float GetBasicTime() const noexcept                    { return m_fBasicTime; }

private:
    std::unique_ptr<cObjectGuagen> m_pEngraveGuage;
    std::unique_ptr<cStatic>       m_pRemaintimeStatic;

    std::uint32_t m_dwProcessTime = 0;
    float         m_fBasicTime    = 1.0f;
};

// ---------------------------------------------------------------------------
// CFWTimeDialog — siege-war timer + character name
// ---------------------------------------------------------------------------
class cFWTimeDialog : public cDialog {
public:
    // Local id range 1:1 with legacy FW_TIME* enum (rebased to 782..783).
    static constexpr int kIdTimeStatic      = 782;
    static constexpr int kIdCharacterName   = 783;

    cFWTimeDialog();
    ~cFWTimeDialog() override;

    void Linking();
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;
    void SetActiveWithTimeName(bool val, std::uint32_t dwTime, const char* pName);
    void SetCharacterName(const char* pName);

    // Test accessors.
    const cStatic* GetTimeStatic() const noexcept    { return m_pTimeStatic.get(); }
    const cStatic* GetCharacterName() const noexcept { return m_pCharacterName.get(); }
    std::uint32_t GetWarTime() const noexcept        { return m_dwWarTime; }

private:
    std::unique_ptr<cStatic> m_pTimeStatic;
    std::unique_ptr<cStatic> m_pCharacterName;

    std::uint32_t m_dwWarTime = 0;
};

} // namespace mxh::ui
