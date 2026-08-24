#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cMNCreateDialog final : public cDialog {
public:
    cMNCreateDialog();
    ~cMNCreateDialog() override;

    cMNCreateDialog(const cMNCreateDialog&) = delete;
    cMNCreateDialog& operator=(const cMNCreateDialog&) = delete;

    void Linking();
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
};

} // namespace mxh::ui
