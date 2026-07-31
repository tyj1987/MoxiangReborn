// cdebugdlg.cpp -- modern implementation of
//   Moxiang CDebugDlg.

#include "cdebugdlg.hpp"

#include <cstdio>
#include <cstdarg>

namespace mxh::ui {

cDebugDlg::cDebugDlg() = default;

cDebugDlg::~cDebugDlg() = default;

void cDebugDlg::DebugMsgParser(std::uint8_t type, const char* msg, ...) {
    if (!msg) return;

    char body[256] = {0};
    {
        va_list args;
        va_start(args, msg);
        std::vsnprintf(body, sizeof(body), msg, args);
        va_end(args);
    }

    char line[300] = {0};
    std::uint32_t color = 0xFF000000u;
    switch (static_cast<DebugType>(type)) {
        case DebugType::Attack:
            if (!m_bAttackFlag) return;
            std::snprintf(line, sizeof(line), "ATTACK: %s", body);
            break;
        case DebugType::Item:
            if (!m_bItemFlag) return;
            std::snprintf(line, sizeof(line), "ITEM: %s", body);
            break;
        case DebugType::Move:
            if (!m_bMoveFlag) return;
            std::snprintf(line, sizeof(line), "MOVE: %s", body);
            break;
        case DebugType::Mugong:
            if (!m_bMugongFlag) return;
            std::snprintf(line, sizeof(line), "MUGONG: %s", body);
            break;
        case DebugType::Chat:
            if (!m_bChatFlag) return;
            std::snprintf(line, sizeof(line), "CHAT: %s", body);
            break;
        default:
            std::snprintf(line, sizeof(line), "NORMAL: %s", body);
            break;
    }
    AddItemForTest(line, color);
}

void cDebugDlg::AddItemForTest(const std::string& text,
                               std::uint32_t color) {
    m_lastAddedText  = text;
    m_lastAddedColor = color;
    ++m_addItemCount;
}

} // namespace mxh::ui
