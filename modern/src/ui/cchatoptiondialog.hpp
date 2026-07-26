#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
namespace mxh::ui {class cChatOptionDialog final:public cDialog{public:void SetChatEnabled(bool v)noexcept{m_chat=v;}void SetWhisperEnabled(bool v)noexcept{m_whisper=v;}void SetFilterMask(std::uint32_t v)noexcept{m_filter=v;}bool ChatEnabled()const noexcept{return m_chat;}bool WhisperEnabled()const noexcept{return m_whisper;}std::uint32_t FilterMask()const noexcept{return m_filter;}void Reset()noexcept{m_chat=true;m_whisper=true;m_filter=0; }private:bool m_chat{true},m_whisper{true};std::uint32_t m_filter{};};}
