// mxh/ui/cObject.hpp
// Phase 6.0 — base class for all UI nodes. Modern C++ rewrite of
// legacy [Client]MH/interface/cObject.h. Same conceptual surface
// (id / name / parent) but with std::string, owning children via
// unique_ptr, and no MFC dependencies.
#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace mxh::ui {

class cObject {
public:
    cObject() = default;
    explicit cObject(std::int32_t id) : m_id(id) {}
    virtual ~cObject() = default;

    // Copy / move are enabled by default (cObject is a value-type ID/name
    // holder). Subclasses that own resources should delete them.

    // Identity.
    std::int32_t  id() const noexcept { return m_id; }
    void          setId(std::int32_t v) noexcept { m_id = v; }

    // Debug name (used by tooling + logs; never affects behavior).
    const std::string& name() const noexcept { return m_name; }
    void               setName(std::string n) { m_name = std::move(n); }

    // Original symbolic #ID from InterfaceScript. The legacy client first
    // retained this token and then resolved it through WindowIDs.h; keeping
    // both forms makes diagnostics and exact resource-driven lookup possible.
    const std::string& legacyId() const noexcept { return m_legacyId; }
    void setLegacyId(std::string id) { m_legacyId = std::move(id); }

    // Original #FUNC callback token from InterfaceScript. Some legacy
    // controls (notably the character creation button) have no #ID and can
    // only be bound correctly through this symbolic function name.
    const std::string& legacyFunc() const noexcept { return m_legacyFunc; }
    void setLegacyFunc(std::string func) { m_legacyFunc = std::move(func); }

    // Parent (non-owning; ownership flows through the owning cWindow tree).
    cObject* parent() const noexcept { return m_parent; }
    void     setParent(cObject* p) noexcept { m_parent = p; }

protected:
    // Subclasses (e.g. cWindow) need to mutate id/name during Init. Keep
    // these protected rather than friending every subclass.
    std::int32_t& mutableId() noexcept { return m_id; }
    std::string&  mutableName() noexcept { return m_name; }

private:
    std::int32_t m_id     = 0;
    std::string  m_name;
    std::string  m_legacyId;
    std::string  m_legacyFunc;
    cObject*     m_parent = nullptr;
};

} // namespace mxh::ui
