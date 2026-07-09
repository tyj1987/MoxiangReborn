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
    cObject*     m_parent = nullptr;
};

} // namespace mxh::ui
