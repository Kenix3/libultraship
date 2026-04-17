#include <gtest/gtest.h>
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
