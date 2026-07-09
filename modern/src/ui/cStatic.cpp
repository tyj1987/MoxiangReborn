// cStatic.cpp — modern implementation of 墨香 cStatic (label).

#include "cStatic.hpp"

#include <cstdio>
#include <cstdlib>

namespace mxh::ui {

cStatic::cStatic() = default;
cStatic::~cStatic() = default;

void cStatic::SetStaticText(std::string text) {
    m_text = std::move(text);
}

void cStatic::SetStaticValue(std::int32_t v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", v);
    m_text = buf;
}

std::int32_t cStatic::GetStaticValue() const noexcept {
    if (m_text.empty()) return 0;
    return std::atoi(m_text.c_str());
}

} // namespace mxh::ui
