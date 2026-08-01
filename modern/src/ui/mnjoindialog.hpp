#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cMNJoinDialog final : public cDialog {
public:
    cMNJoinDialog();
    ~cMNJoinDialog() override;

    cMNJoinDialog(const cMNJoinDialog&) = delete;
    cMNJoinDialog& operator=(const cMNJoinDialog&) = delete;

    void Linking();
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
};

} // namespace mxh::ui
