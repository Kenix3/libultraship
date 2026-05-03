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

// ---- Dependency failure tests ----

// A component that declares a dependency via GetDependencies().
class DependentComponent : public Component {
  public:
    std::vector<std::string> mDeps;
    explicit DependentComponent(const std::string& name, std::vector<std::string> deps)
        : Component(name), mDeps(std::move(deps)) {
        mDepsJson = nlohmann::json::array();
        for (const auto& d : mDeps) {
            mDepsJson.push_back(d);
        }
    }

  protected:
    const nlohmann::json& GetDependencies() const override { return mDepsJson; }
    void OnInit(const nlohmann::json& /*initArgs*/) override {}

  private:
    nlohmann::json mDepsJson;
};

TEST(ComponentDependencyTest, ThrowsWhenDependencyNotPresent) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<DependentComponent>("Child", std::vector<std::string>{"MissingDep"});
    parent->GetChildren().Add(child);

    EXPECT_THROW(child->Init(), std::runtime_error);
    EXPECT_FALSE(child->IsInitialized());
}

TEST(ComponentDependencyTest, ThrowsWhenDependencyNotInitialized) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto dep = std::make_shared<TestComponent>("TheDep");
    auto child = std::make_shared<DependentComponent>("Child", std::vector<std::string>{"TheDep"});
    parent->GetChildren().Add(dep);
    parent->GetChildren().Add(child);

    // dep is NOT initialized yet
    EXPECT_THROW(child->Init(), std::runtime_error);
    EXPECT_FALSE(child->IsInitialized());
}

TEST(ComponentDependencyTest, SucceedsWhenDependencyIsInitialized) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto dep = std::make_shared<TestComponent>("TheDep");
    auto child = std::make_shared<DependentComponent>("Child", std::vector<std::string>{"TheDep"});
    parent->GetChildren().Add(dep);
    parent->GetChildren().Add(child);

    dep->Init();
    EXPECT_NO_THROW(child->Init());
    EXPECT_TRUE(child->IsInitialized());
}

TEST(ComponentDependencyTest, ThrowsWithCorrectMessageForMissingDependency) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<DependentComponent>("Child", std::vector<std::string>{"NoDep"});
    parent->GetChildren().Add(child);

    try {
        child->Init();
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("Child"), std::string::npos);
        EXPECT_NE(msg.find("NoDep"), std::string::npos);
    }
}

TEST(ComponentDependencyTest, ThrowsWithCorrectMessageForUninitializedDependency) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto dep = std::make_shared<TestComponent>("NotReady");
    auto child = std::make_shared<DependentComponent>("Child", std::vector<std::string>{"NotReady"});
    parent->GetChildren().Add(dep);
    parent->GetChildren().Add(child);

    try {
        child->Init();
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("Child"), std::string::npos);
        EXPECT_NE(msg.find("NotReady"), std::string::npos);
        EXPECT_NE(msg.find("initialized"), std::string::npos);
    }
}

TEST(ComponentDependencyTest, MultipleDependenciesAllMustBeReady) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto dep1 = std::make_shared<TestComponent>("Dep1");
    auto dep2 = std::make_shared<TestComponent>("Dep2");
    auto child = std::make_shared<DependentComponent>("Child", std::vector<std::string>{"Dep1", "Dep2"});
    parent->GetChildren().Add(dep1);
    parent->GetChildren().Add(dep2);
    parent->GetChildren().Add(child);

    // Only dep1 initialized
    dep1->Init();
    EXPECT_THROW(child->Init(), std::runtime_error);
    EXPECT_FALSE(child->IsInitialized());

    // Now dep2 initialized too
    dep2->Init();
    EXPECT_NO_THROW(child->Init());
    EXPECT_TRUE(child->IsInitialized());
}

TEST(ComponentDependencyTest, DependencyFoundInAncestorHierarchy) {
    // Root -> Mid -> Leaf, where Leaf depends on "Root"
    auto root = std::make_shared<TestComponent>("Root");
    auto mid = std::make_shared<TestComponent>("Mid");
    auto leaf = std::make_shared<DependentComponent>("Leaf", std::vector<std::string>{"Root"});
    root->GetChildren().Add(mid);
    mid->GetChildren().Add(leaf);
    root->Init(); // Mark root as initialized

    EXPECT_NO_THROW(leaf->Init());
    EXPECT_TRUE(leaf->IsInitialized());
}

TEST(ComponentDependencyTest, DependencyFoundAsSibling) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto sibling = std::make_shared<TestComponent>("Sibling");
    auto child = std::make_shared<DependentComponent>("Child", std::vector<std::string>{"Sibling"});
    parent->GetChildren().Add(sibling);
    parent->GetChildren().Add(child);
    sibling->Init();

    EXPECT_NO_THROW(child->Init());
    EXPECT_TRUE(child->IsInitialized());
}

// ---- InitArgs passthrough tests ----

class InitArgsCapture : public Component {
  public:
    nlohmann::json capturedArgs;
    explicit InitArgsCapture(const std::string& name = "InitArgsCapture") : Component(name) {}
  protected:
    void OnInit(const nlohmann::json& initArgs) override {
        capturedArgs = initArgs;
    }
};

TEST(ComponentInitArgsTest, InitPassesArgsToOnInit) {
    auto c = std::make_shared<InitArgsCapture>("Capture");
    nlohmann::json args = {{"key", "value"}, {"count", 42}};
    c->Init(args);
    EXPECT_TRUE(c->IsInitialized());
    EXPECT_EQ(c->capturedArgs["key"], "value");
    EXPECT_EQ(c->capturedArgs["count"], 42);
}

TEST(ComponentInitArgsTest, DefaultInitArgsIsEmptyObject) {
    auto c = std::make_shared<InitArgsCapture>("Capture");
    c->Init();
    EXPECT_TRUE(c->IsInitialized());
    EXPECT_TRUE(c->capturedArgs.is_object());
    EXPECT_TRUE(c->capturedArgs.empty());
}

// ---- BFS search (HasInChildren, GetFirstInChildren, GetInChildren) ----

TEST(ComponentSearchTest, HasInChildrenFindsDeepDescendant) {
    auto root = std::make_shared<TestComponent>("Root");
    auto mid = std::make_shared<TestComponent>("Mid");
    auto leaf = std::make_shared<DerivedComponent>("Deep");
    root->GetChildren().Add(mid);
    mid->GetChildren().Add(leaf);

    EXPECT_TRUE(root->HasInChildren<DerivedComponent>());
}

TEST(ComponentSearchTest, HasInChildrenReturnsFalseWhenAbsent) {
    auto root = std::make_shared<TestComponent>("Root");
    root->GetChildren().Add(std::make_shared<TestComponent>("Child"));

    EXPECT_FALSE(root->HasInChildren<DerivedComponent>());
}

TEST(ComponentSearchTest, GetFirstInChildrenFindsDeepDescendant) {
    auto root = std::make_shared<TestComponent>("Root");
    auto mid = std::make_shared<TestComponent>("Mid");
    auto leaf = std::make_shared<DerivedComponent>("DeepLeaf");
    root->GetChildren().Add(mid);
    mid->GetChildren().Add(leaf);

    auto found = root->GetFirstInChildren<DerivedComponent>();
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "DeepLeaf");
}

TEST(ComponentSearchTest, GetInChildrenFindsAll) {
    auto root = std::make_shared<TestComponent>("Root");
    auto d1 = std::make_shared<DerivedComponent>("D1");
    auto mid = std::make_shared<TestComponent>("Mid");
    auto d2 = std::make_shared<DerivedComponent>("D2");
    root->GetChildren().Add(d1);
    root->GetChildren().Add(mid);
    mid->GetChildren().Add(d2);

    auto all = root->GetInChildren<DerivedComponent>();
    EXPECT_EQ(all->size(), 2u);
}
