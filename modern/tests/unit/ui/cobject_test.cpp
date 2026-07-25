// mxh/tests/unit/ui/cobject_test.cpp
// Unit tests for mxh::ui::cObject (Phase 6.0 base class).
//
// Locks down the 1:1 surface:
//   * id / name accessors + setters
//   * default-constructed state
//   * explicit-id constructor
//   * parent (non-owning) pointer
//   * protected mutables for subclass Init
//   * move semantics: std::string ownership transfers without copy

#include "cObject.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>

using mxh::ui::cObject;

TEST(CObject, DefaultConstructedIsZero) {
    cObject o;
    EXPECT_EQ(o.id(), 0);
    EXPECT_EQ(o.name(), "");
    EXPECT_EQ(o.parent(), nullptr);
}

TEST(CObject, ExplicitIdConstructor) {
    cObject o(42);
    EXPECT_EQ(o.id(), 42);
    EXPECT_EQ(o.name(), "");
    EXPECT_EQ(o.parent(), nullptr);
}

TEST(CObject, SetIdRoundTrip) {
    cObject o;
    o.setId(100);
    EXPECT_EQ(o.id(), 100);
    o.setId(-7);
    EXPECT_EQ(o.id(), -7);
}

TEST(CObject, SetNameRoundTrip) {
    cObject o;
    o.setName("login_button");
    EXPECT_EQ(o.name(), "login_button");
    o.setName("logout_button");
    EXPECT_EQ(o.name(), "logout_button");
}

TEST(CObject, SetNameMoves) {
    // std::string setName should be a move; verify the source is moved-
    // from (an empty string) after the call.
    cObject o;
    std::string src = "test_name";
    o.setName(std::move(src));
    EXPECT_EQ(o.name(), "test_name");
    EXPECT_TRUE(src.empty());
}

TEST(CObject, ParentIsNonOwning) {
    cObject parent;
    cObject child;
    child.setParent(&parent);
    EXPECT_EQ(child.parent(), &parent);
    // Setting parent to nullptr breaks the link.
    child.setParent(nullptr);
    EXPECT_EQ(child.parent(), nullptr);
}

TEST(CObject, MutableIdForSubclassInit) {
    // Simulate a subclass that uses the protected mutableId() during Init.
    struct Sub : cObject {
        void init(int v) { mutableId() = v; }
    };
    Sub s;
    s.init(99);
    EXPECT_EQ(s.id(), 99);
}

TEST(CObject, MutableNameForSubclassInit) {
    struct Sub : cObject {
        void init(std::string n) { mutableName() = std::move(n); }
    };
    Sub s;
    s.init("from_init");
    EXPECT_EQ(s.name(), "from_init");
}

TEST(CObject, IsPolymorphic) {
    // The legacy cObject was the root of the UI tree via virtual dtor.
    // Make sure the modern version still lets derived objects be
    // destroyed through a cObject*.
    struct Sub : cObject {
        explicit Sub(int v) : cObject(v) {}
        ~Sub() override = default;
    };
    auto* base = new Sub(7);
    EXPECT_EQ(base->id(), 7);
    delete base;  // virtual dtor lets this work
}
