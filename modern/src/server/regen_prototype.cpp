// regen_prototype.cpp - Phase D6 RegenPrototype 1:1 port.

#include "mxh/server/regen_prototype.hpp"

namespace mxh::server {

void regen_object_init_prototype(RegenObject& obj, RegenPrototype* prototype) {
    obj.m_pPrototype = prototype;
}

void regen_object_init_help_type(RegenObject& obj) {
    if (obj.m_pPrototype == nullptr) return;
    obj.m_CurHelpType = obj.m_pPrototype->InitHelpType;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int regen_prototype_translation_unit_anchor = 0;
}
