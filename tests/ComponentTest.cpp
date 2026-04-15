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

// ---- Parent/child relationship tests ----

TEST(ComponentTest, AddChild) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    EXPECT_TRUE(parent->AddChild(child));
    EXPECT_TRUE(parent->GetChildren().Has(child));
    EXPECT_EQ(parent->GetChildren().GetCount(), 1u);
}

TEST(ComponentTest, AddChildSetsParent) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    parent->AddChild(child);
    EXPECT_TRUE(child->GetParents().Has(parent));
}

TEST(ComponentTest, AddParent) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    EXPECT_TRUE(child->AddParent(parent));
    EXPECT_TRUE(child->GetParents().Has(parent));
    EXPECT_TRUE(parent->GetChildren().Has(child));
}

TEST(ComponentTest, RemoveChild) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    parent->AddChild(child);
    EXPECT_TRUE(parent->RemoveChild(child));
    EXPECT_FALSE(parent->GetChildren().Has(child));
    EXPECT_FALSE(child->GetParents().Has(parent));
}

TEST(ComponentTest, RemoveParent) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");

    child->AddParent(parent);
    EXPECT_TRUE(child->RemoveParent(parent));
    EXPECT_FALSE(child->GetParents().Has(parent));
    EXPECT_FALSE(parent->GetChildren().Has(child));
}

TEST(ComponentTest, AddMultipleChildren) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto c1 = std::make_shared<TestComponent>("C1");
    auto c2 = std::make_shared<TestComponent>("C2");
    auto c3 = std::make_shared<TestComponent>("C3");

    EXPECT_TRUE(parent->AddChildren({ c1, c2, c3 }));
    EXPECT_EQ(parent->GetChildren().GetCount(), 3u);
}

TEST(ComponentTest, RemoveAllChildren) {
    auto parent = std::make_shared<TestComponent>("Parent");
    parent->AddChild(std::make_shared<TestComponent>("C1"));
    parent->AddChild(std::make_shared<TestComponent>("C2"));

    EXPECT_TRUE(parent->RemoveChildren());
    EXPECT_EQ(parent->GetChildren().GetCount(), 0u);
}

TEST(ComponentTest, RemoveChildById) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto child = std::make_shared<TestComponent>("Child");
    parent->AddChild(child);

    EXPECT_TRUE(parent->RemoveChild(child->GetId()));
    EXPECT_EQ(parent->GetChildren().GetCount(), 0u);
}

TEST(ComponentTest, RemoveChildrenByName) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto c1 = std::make_shared<TestComponent>("Alpha");
    auto c2 = std::make_shared<TestComponent>("Beta");
    auto c3 = std::make_shared<TestComponent>("Alpha");
    parent->AddChildren({ c1, c2, c3 });

    EXPECT_TRUE(parent->RemoveChildren(std::string("Alpha")));
    EXPECT_EQ(parent->GetChildren().GetCount(), 1u);
    EXPECT_TRUE(parent->GetChildren().Has(c2));
}

// ---- GetChild<T>() template tests ----

TEST(ComponentTest, GetChildByType) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto derived = std::make_shared<DerivedComponent>("Derived");
    auto another = std::make_shared<AnotherComponent>("Another");

    parent->AddChild(derived);
    parent->AddChild(another);

    auto found = parent->GetChild<DerivedComponent>();
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "Derived");
    EXPECT_EQ(found->value, 42);
}

TEST(ComponentTest, GetChildByTypeReturnsFirstMatch) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto d1 = std::make_shared<DerivedComponent>("First");
    auto d2 = std::make_shared<DerivedComponent>("Second");

    parent->AddChild(d1);
    parent->AddChild(d2);

    auto found = parent->GetChild<DerivedComponent>();
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "First");
}

TEST(ComponentTest, GetChildByTypeNotFound) {
    auto parent = std::make_shared<TestComponent>("Parent");
    parent->AddChild(std::make_shared<TestComponent>("Child"));

    auto found = parent->GetChild<DerivedComponent>();
    EXPECT_EQ(found, nullptr);
}

TEST(ComponentTest, GetChildByTypeFromEmpty) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto found = parent->GetChild<DerivedComponent>();
    EXPECT_EQ(found, nullptr);
}

// ---- RemoveChildren<T>() template tests ----

TEST(ComponentTest, RemoveChildrenByType) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto tc = std::make_shared<TestComponent>("Keep");
    auto dc = std::make_shared<DerivedComponent>("Remove");
    parent->AddChild(tc);
    parent->AddChild(dc);

    EXPECT_TRUE(parent->RemoveChildren<DerivedComponent>());
    EXPECT_EQ(parent->GetChildren().GetCount(), 1u);
    EXPECT_TRUE(parent->GetChildren().Has(tc));
}

TEST(ComponentTest, RemoveChildrenByTypeAndName) {
    auto parent = std::make_shared<TestComponent>("Parent");
    auto d1 = std::make_shared<DerivedComponent>("Alpha");
    auto d2 = std::make_shared<DerivedComponent>("Beta");
    parent->AddChild(d1);
    parent->AddChild(d2);

    EXPECT_TRUE(parent->RemoveChildren<DerivedComponent>(std::string("Alpha")));
    EXPECT_EQ(parent->GetChildren().GetCount(), 1u);
    EXPECT_TRUE(parent->GetChildren().Has(d2));
}

// ---- Deep hierarchy tests ----

TEST(ComponentTest, DeepHierarchy) {
    auto root = std::make_shared<TestComponent>("Root");
    auto mid = std::make_shared<TestComponent>("Mid");
    auto leaf = std::make_shared<DerivedComponent>("Leaf");

    root->AddChild(mid);
    mid->AddChild(leaf);

    // Can traverse the hierarchy
    auto midFound = root->GetChild<TestComponent>();
    ASSERT_NE(midFound, nullptr);

    auto leafFound = mid->GetChild<DerivedComponent>();
    ASSERT_NE(leafFound, nullptr);
    EXPECT_EQ(leafFound->GetName(), "Leaf");
}

TEST(ComponentTest, MultipleParents) {
    auto p1 = std::make_shared<TestComponent>("Parent1");
    auto p2 = std::make_shared<TestComponent>("Parent2");
    auto child = std::make_shared<TestComponent>("Child");

    child->AddParent(p1);
    child->AddParent(p2);

    EXPECT_EQ(child->GetParents().GetCount(), 2u);
    EXPECT_TRUE(p1->GetChildren().Has(child));
    EXPECT_TRUE(p2->GetChildren().Has(child));
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
