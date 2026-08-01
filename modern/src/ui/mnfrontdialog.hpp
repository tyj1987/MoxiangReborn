#pragma once

#include "mxh/ui/cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cMNFrontDialog final : public cDialog {
public:
    cMNFrontDialog();
    ~cMNFrontDialog() override;

    cMNFrontDialog(const cMNFrontDialog&) = delete;
    cMNFrontDialog& operator=(const cMNFrontDialog&) = delete;

    void Linking();
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
};

} // namespace mxh::ui
