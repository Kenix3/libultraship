#include <gtest/gtest.h>
#include <stdexcept>
#include "ship/Component.h"

using namespace Ship;

// Concrete Component subclass for testing.
class TestComponent : public Component {
  public:
    explicit TestComponent(const std::string& name = "TestComponent") : Component(name) {
    }
};

// Derived Component subclass for type-based lookup tests.
class DerivedComponent : public Component {
  public:
    explicit DerivedComponent(const std::string& name = "DerivedComponent") : Component(name) {
    }
    int value = 42;
};

// Another derived type.
class AnotherComponent : public Component {
  public:
    explicit AnotherComponent(const std::string& name = "AnotherComponent") : Component(name) {
    }
};

// ---- Basic Component tests ----

TEST(ComponentTest, ConstructionAndName) {
    auto c = std::make_shared<TestComponent>("Foo");
    EXPECT_EQ(c->GetName(), "Foo");
}

TEST(ComponentTest, ToString) {
    auto c = std::make_shared<TestComponent>("Bar");
    auto str = c->ToString();
    EXPECT_NE(str.find("Bar"), std::string::npos);
}

TEST(ComponentTest, StringConversion) {
    auto c = std::make_shared<TestComponent>("Baz");
    std::string s = static_cast<std::string>(*c);
    EXPECT_EQ(s, c->ToString());
}

// ---- Parent/child relationship tests using PartList API ----

TEST(ComponentTest, AddChild) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    EXPECT_EQ(parent->GetChildren().Add(child), ListReturnCode::Success);
    EXPECT_TRUE(parent->GetChildren().Has(child));
    EXPECT_EQ(parent->GetChildren().GetCount(), 1u);
}

TEST(ComponentTest, AddChildDuplicate) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    parent->GetChildren().Add(child);
    EXPECT_EQ(parent->GetChildren().Add(child), ListReturnCode::Duplicate);
    EXPECT_EQ(parent->GetChildren().GetCount(), 1u);
}

TEST(ComponentTest, AddParent) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    EXPECT_EQ(child->GetParents().Add(parent), ListReturnCode::Success);
    EXPECT_TRUE(child->GetParents().Has(parent));
}

TEST(ComponentTest, RemoveChild) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    parent->GetChildren().Add(child);
    EXPECT_EQ(parent->GetChildren().Remove(child), ListReturnCode::Success);
    EXPECT_FALSE(parent->GetChildren().Has(child));
}

TEST(ComponentTest, RemoveParent) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    child->GetParents().Add(parent);
    EXPECT_EQ(child->GetParents().Remove(parent), ListReturnCode::Success);
    EXPECT_FALSE(child->GetParents().Has(parent));
}

TEST(ComponentTest, AddMultipleChildren) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto c1 = std::make_shared<TestComponent>("C1");
    auto c2 = std::make_shared<TestComponent>("C2");
    auto c3 = std::make_shared<TestComponent>("C3");

    parent->GetChildren().Add({ c1, c2, c3 });
    EXPECT_EQ(parent->GetChildren().GetCount(), 3u);
}

TEST(ComponentTest, RemoveAllChildren) {
    auto parent = std::make_shared<TestComponent>("Parent");
    parent->GetChildren().Add(std::make_shared<TestComponent>("C1"));
    parent->GetChildren().Add(std::make_shared<TestComponent>("C2"));

    parent->GetChildren().Remove();
    EXPECT_EQ(parent->GetChildren().GetCount(), 0u);
}

TEST(ComponentTest, RemoveChildById) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");
    parent->GetChildren().Add(child);

    EXPECT_EQ(parent->GetChildren().Remove(child->GetId()), ListReturnCode::Success);
    EXPECT_EQ(parent->GetChildren().GetCount(), 0u);
}

// ---- GetChildren().GetFirst<T>() template tests ----

TEST(ComponentTest, GetChildByType) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto derived = std::make_shared<DerivedComponent>("Derived");
    auto another = std::make_shared<AnotherComponent>("Another");

    parent->GetChildren().Add(derived);
    parent->GetChildren().Add(another);

    auto found = parent->GetChildren().GetFirst<DerivedComponent>();
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "Derived");
    EXPECT_EQ(found->value, 42);
}

TEST(ComponentTest, GetChildByTypeReturnsFirstMatch) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto d1 = std::make_shared<DerivedComponent>("First");
    auto d2 = std::make_shared<DerivedComponent>("Second");

    parent->GetChildren().Add(d1);
    parent->GetChildren().Add(d2);

    auto found = parent->GetChildren().GetFirst<DerivedComponent>();
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "First");
}

TEST(ComponentTest, GetChildByTypeNotFound) {
    auto parent = std::make_shared<TestComponent>("Parent");
    parent->GetChildren().Add(std::make_shared<TestComponent>("Child"));

    auto found = parent->GetChildren().GetFirst<DerivedComponent>();
    EXPECT_EQ(found, nullptr);
}

TEST(ComponentTest, GetChildByTypeFromEmpty) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto found = parent->GetChildren().GetFirst<DerivedComponent>();
    EXPECT_EQ(found, nullptr);
}

// ---- Remove<T>() template tests ----

TEST(ComponentTest, RemoveChildrenByType) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto tc = std::make_shared<TestComponent>("Keep");
    auto dc = std::make_shared<DerivedComponent>("Remove");
    parent->GetChildren().Add(tc);
    parent->GetChildren().Add(dc);

    parent->GetChildren().Remove<DerivedComponent>();
    EXPECT_EQ(parent->GetChildren().GetCount(), 1u);
    EXPECT_TRUE(parent->GetChildren().Has(tc));
}

// ---- Deep hierarchy tests ----

TEST(ComponentTest, DeepHierarchy) {
    auto root = std::make_shared<TestComponent>("Root");
    auto mid = std::make_shared<TestComponent>("Mid");
    auto leaf = std::make_shared<DerivedComponent>("Leaf");

    root->GetChildren().Add(mid);
    mid->GetChildren().Add(leaf);

    auto midFound = root->GetChildren().GetFirst<TestComponent>();
    ASSERT_NE(midFound, nullptr);

    auto leafFound = mid->GetChildren().GetFirst<DerivedComponent>();
    ASSERT_NE(leafFound, nullptr);
    EXPECT_EQ(leafFound->GetName(), "Leaf");
}

TEST(ComponentTest, MultipleParents) {
    auto p1 = std::make_shared<TestComponent>("Parent1");
    auto p2 = std::make_shared<TestComponent>("Parent2");
    auto child = std::make_shared<TestComponent>("Child");

    child->GetParents().Add(p1);
    child->GetParents().Add(p2);

    EXPECT_EQ(child->GetParents().GetCount(), 2u);
}

// ---- PartList hook tests ----

TEST(ComponentTest, AddWithForce) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    EXPECT_EQ(parent->GetChildren().Add(child, true), ListReturnCode::Success);
    EXPECT_TRUE(parent->GetChildren().Has(child));
}

TEST(ComponentTest, RemoveWithForce) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    parent->GetChildren().Add(child);
    EXPECT_EQ(parent->GetChildren().Remove(child, true), ListReturnCode::Success);
    EXPECT_FALSE(parent->GetChildren().Has(child));
}

// ---- Bidirectional relationship tests ----

TEST(ComponentTest, AddChildCreatesParent) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    parent->GetChildren().Add(child);
    EXPECT_TRUE(child->GetParents().Has(parent));
    EXPECT_EQ(child->GetParents().GetCount(), 1u);
}

TEST(ComponentTest, AddParentCreatesChild) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    child->GetParents().Add(parent);
    EXPECT_TRUE(parent->GetChildren().Has(child));
    EXPECT_EQ(parent->GetChildren().GetCount(), 1u);
}

TEST(ComponentTest, RemoveChildRemovesParent) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    parent->GetChildren().Add(child);
    EXPECT_TRUE(child->GetParents().Has(parent));

    parent->GetChildren().Remove(child);
    EXPECT_FALSE(child->GetParents().Has(parent));
    EXPECT_EQ(child->GetParents().GetCount(), 0u);
}

TEST(ComponentTest, RemoveParentRemovesChild) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    child->GetParents().Add(parent);
    EXPECT_TRUE(parent->GetChildren().Has(child));

    child->GetParents().Remove(parent);
    EXPECT_FALSE(parent->GetChildren().Has(child));
    EXPECT_EQ(parent->GetChildren().GetCount(), 0u);
}

TEST(ComponentTest, BidirectionalMultipleParents) {
    auto p1 = std::make_shared<TestComponent>("Parent1");
    auto p2 = std::make_shared<TestComponent>("Parent2");
    auto child = std::make_shared<TestComponent>("Child");

    child->GetParents().Add(p1);
    child->GetParents().Add(p2);

    EXPECT_TRUE(p1->GetChildren().Has(child));
    EXPECT_TRUE(p2->GetChildren().Has(child));
    EXPECT_EQ(p1->GetChildren().GetCount(), 1u);
    EXPECT_EQ(p2->GetChildren().GetCount(), 1u);
}

TEST(ComponentTest, BidirectionalMultipleChildren) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto c1 = std::make_shared<TestComponent>("C1");
    auto c2 = std::make_shared<TestComponent>("C2");

    parent->GetChildren().Add(c1);
    parent->GetChildren().Add(c2);

    EXPECT_TRUE(c1->GetParents().Has(parent));
    EXPECT_TRUE(c2->GetParents().Has(parent));
}

TEST(ComponentTest, BidirectionalRemoveAllChildren) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto c1 = std::make_shared<TestComponent>("C1");
    auto c2 = std::make_shared<TestComponent>("C2");

    parent->GetChildren().Add(c1);
    parent->GetChildren().Add(c2);

    parent->GetChildren().Remove();
    EXPECT_EQ(c1->GetParents().GetCount(), 0u);
    EXPECT_EQ(c2->GetParents().GetCount(), 0u);
}

// ---- ComponentList tests ----

#include "ship/ComponentList.h"

TEST(ComponentListTest, HasByName) {
    ComponentList list;
    auto c = std::make_shared<TestComponent>("Foo");
    list.Add(c);
    EXPECT_TRUE(list.Has(std::string("Foo")));
    EXPECT_FALSE(list.Has(std::string("Bar")));
}

TEST(ComponentListTest, HasByTypeAndName) {
    ComponentList list;
    list.Add(std::make_shared<TestComponent>("TC"));
    list.Add(std::make_shared<DerivedComponent>("DC"));

    EXPECT_TRUE(list.Has<DerivedComponent>(std::string("DC")));
    EXPECT_FALSE(list.Has<DerivedComponent>(std::string("TC")));
}

TEST(ComponentListTest, GetByName) {
    ComponentList list;
    list.Add(std::make_shared<TestComponent>("Alpha"));
    list.Add(std::make_shared<TestComponent>("Beta"));
    list.Add(std::make_shared<TestComponent>("Alpha"));

    auto result = list.Get(std::string("Alpha"));
    EXPECT_EQ(result->size(), 2u);
}

TEST(ComponentListTest, GetByTypeAndName) {
    ComponentList list;
    list.Add(std::make_shared<TestComponent>("MyName"));
    list.Add(std::make_shared<DerivedComponent>("MyName"));

    auto result = list.Get<DerivedComponent>(std::string("MyName"));
    EXPECT_EQ(result->size(), 1u);
    EXPECT_NE(std::dynamic_pointer_cast<DerivedComponent>((*result)[0]), nullptr);
}

TEST(ComponentListTest, GetByMultipleNames) {
    ComponentList list;
    list.Add(std::make_shared<TestComponent>("A"));
    list.Add(std::make_shared<TestComponent>("B"));
    list.Add(std::make_shared<TestComponent>("C"));

    auto result = list.Get(std::vector<std::string>{ "A", "C" });
    EXPECT_EQ(result->size(), 2u);
}

// ---- Additional edge-case tests ----

TEST(ComponentTest, BidirectionalMultiLevelHierarchy) {
    auto root = std::make_shared<TestComponent>("Root");
    auto mid = std::make_shared<TestComponent>("Mid");
    auto leaf = std::make_shared<TestComponent>("Leaf");

    root->GetChildren().Add(mid);
    mid->GetChildren().Add(leaf);

    // Verify parent links go up
    EXPECT_TRUE(mid->GetParents().Has(root));
    EXPECT_TRUE(leaf->GetParents().Has(mid));

    // Root should not be a direct parent of leaf
    EXPECT_FALSE(leaf->GetParents().Has(root));
}

TEST(ComponentTest, RemoveMidNodeBreaksChain) {
    auto root = std::make_shared<TestComponent>("Root");
    auto mid = std::make_shared<TestComponent>("Mid");
    auto leaf = std::make_shared<TestComponent>("Leaf");

    root->GetChildren().Add(mid);
    mid->GetChildren().Add(leaf);

    root->GetChildren().Remove(mid);

    EXPECT_EQ(root->GetChildren().GetCount(), 0u);
    EXPECT_FALSE(mid->GetParents().Has(root));
    // Leaf is still a child of mid
    EXPECT_TRUE(mid->GetChildren().Has(leaf));
}

TEST(ComponentTest, GetCountReflectsAddAndRemove) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto c1 = std::make_shared<TestComponent>("C1");
    auto c2 = std::make_shared<TestComponent>("C2");
    auto c3 = std::make_shared<TestComponent>("C3");

    EXPECT_EQ(parent->GetChildren().GetCount(), 0u);
    parent->GetChildren().Add(c1);
    EXPECT_EQ(parent->GetChildren().GetCount(), 1u);
    parent->GetChildren().Add(c2);
    parent->GetChildren().Add(c3);
    EXPECT_EQ(parent->GetChildren().GetCount(), 3u);

    parent->GetChildren().Remove(c2);
    EXPECT_EQ(parent->GetChildren().GetCount(), 2u);

    parent->GetChildren().Remove();
    EXPECT_EQ(parent->GetChildren().GetCount(), 0u);
}

// ---- Component::Init / OnInit / IsInitialized / MarkInitialized tests ----

// A component that tracks how many times OnInit() was called.
class TrackingComponent : public Component {
  public:
    explicit TrackingComponent(const std::string& name = "TrackingComponent") : Component(name) {}
    int onInitCallCount = 0;

  protected:
    void OnInit(const nlohmann::json& /*initArgs*/ = nlohmann::json::object()) override {
        onInitCallCount++;
    }
};

// A component that auto-marks itself initialized on construction (like Config).
class AutoInitComponent : public Component {
  public:
    explicit AutoInitComponent(const std::string& name = "AutoInitComponent") : Component(name) {
        MarkInitialized();
    }
};

// A component whose OnInit() throws.
class ThrowingComponent : public Component {
  public:
    explicit ThrowingComponent() : Component("ThrowingComponent") {}

  protected:
    void OnInit(const nlohmann::json& /*initArgs*/ = nlohmann::json::object()) override {
        throw std::runtime_error("OnInit failed");
    }
};

TEST(ComponentInitTest, NotInitializedByDefault) {
    auto c = std::make_shared<TestComponent>("Foo");
    EXPECT_FALSE(c->IsInitialized());
}

TEST(ComponentInitTest, InitSetsIsInitialized) {
    auto c = std::make_shared<TestComponent>("Foo");
    c->Init();
    EXPECT_TRUE(c->IsInitialized());
}

TEST(ComponentInitTest, InitCallsOnInit) {
    auto c = std::make_shared<TrackingComponent>();
    EXPECT_EQ(c->onInitCallCount, 0);
    c->Init();
    EXPECT_EQ(c->onInitCallCount, 1);
}

TEST(ComponentInitTest, InitIsIdempotent) {
    auto c = std::make_shared<TrackingComponent>();
    c->Init();
    c->Init();
    c->Init();
    // OnInit() must be called exactly once even when Init() is called multiple times.
    EXPECT_EQ(c->onInitCallCount, 1);
    EXPECT_TRUE(c->IsInitialized());
}

TEST(ComponentInitTest, MarkInitializedSetsFlag) {
    auto c = std::make_shared<AutoInitComponent>();
    EXPECT_TRUE(c->IsInitialized());
}

TEST(ComponentInitTest, MarkInitializedPreventsOnInitFromBeingCalledByInit) {
    // A component that has already called MarkInitialized() in its constructor
    // should not call OnInit() again if Init() is subsequently called.
    class CountingAutoInit : public Component {
      public:
        int onInitCallCount = 0;
        CountingAutoInit() : Component("CountingAutoInit") { MarkInitialized(); }
      protected:
        void OnInit(const nlohmann::json& /*initArgs*/ = nlohmann::json::object()) override { onInitCallCount++; }
    };
    auto c = std::make_shared<CountingAutoInit>();
    c->Init(); // should be a no-op since already marked
    EXPECT_EQ(c->onInitCallCount, 0);
    EXPECT_TRUE(c->IsInitialized());
}

TEST(ComponentInitTest, InitDoesNotMarkInitializedIfOnInitThrows) {
    auto c = std::make_shared<ThrowingComponent>();
    EXPECT_FALSE(c->IsInitialized());
    EXPECT_THROW(c->Init(), std::runtime_error);
    // After a failed OnInit(), the component must remain uninitialized.
    EXPECT_FALSE(c->IsInitialized());
}

TEST(ComponentInitTest, InitCanRetryAfterOnInitThrows) {
    class ConditionalThrow : public Component {
      public:
        bool shouldThrow = true;
        int callCount = 0;
        ConditionalThrow() : Component("ConditionalThrow") {}
      protected:
        void OnInit(const nlohmann::json& /*initArgs*/ = nlohmann::json::object()) override {
            callCount++;
            if (shouldThrow) throw std::runtime_error("not ready");
        }
    };
    auto c = std::make_shared<ConditionalThrow>();
    EXPECT_THROW(c->Init(), std::runtime_error);
    EXPECT_FALSE(c->IsInitialized());
    EXPECT_EQ(c->callCount, 1);

    // Fix the condition and retry.
    c->shouldThrow = false;
    c->Init();
    EXPECT_TRUE(c->IsInitialized());
    EXPECT_EQ(c->callCount, 2);
}

TEST(ComponentInitTest, DefaultOnInitIsNoOp) {
    // The base Component::Init() calls the default (no-op) OnInit() without error.
    auto c = std::make_shared<TestComponent>("NoOpInit");
    EXPECT_NO_THROW(c->Init());
    EXPECT_TRUE(c->IsInitialized());
}
