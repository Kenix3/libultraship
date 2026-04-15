#include <gtest/gtest.h>
#include "ship/Part.h"
#include "ship/PartList.h"

using namespace Ship;

// A concrete Part subclass for testing.
class TestPart : public Part {
  public:
    TestPart() : Part() {
    }
    std::string GetName() const override {
        return "TestPart";
    }
};

class NamedPart : public Part {
  public:
    explicit NamedPart(std::string name) : Part(), mName(std::move(name)) {
    }
    std::string GetName() const override {
        return mName;
    }

  private:
    std::string mName;
};

// Derived type for template filtering tests.
class DerivedPart : public NamedPart {
  public:
    explicit DerivedPart(const std::string& name = "DerivedPart") : NamedPart(name) {
    }
};

// ---- Part tests ----

TEST(PartTest, UniqueIds) {
    TestPart a;
    TestPart b;
    EXPECT_NE(a.GetId(), b.GetId());
}

TEST(PartTest, ToString) {
    TestPart a;
    EXPECT_FALSE(a.ToString().empty());
}

// ---- PartList tests ----

TEST(PartListTest, AddAndHas) {
    PartList<Part> list;
    auto p = std::make_shared<TestPart>();
    EXPECT_FALSE(list.Has(p));
    EXPECT_EQ(list.Add(p), ListReturnCode::Success);
    EXPECT_TRUE(list.Has(p));
    EXPECT_EQ(list.GetCount(), 1u);
}

TEST(PartListTest, AddDuplicate) {
    PartList<Part> list;
    auto p = std::make_shared<TestPart>();
    list.Add(p);
    EXPECT_EQ(list.Add(p), ListReturnCode::Duplicate);
    EXPECT_EQ(list.GetCount(), 1u);
}

TEST(PartListTest, AddNull) {
    PartList<Part> list;
    EXPECT_EQ(list.Add(nullptr), ListReturnCode::Failed);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListTest, RemoveById) {
    PartList<Part> list;
    auto p = std::make_shared<TestPart>();
    list.Add(p);
    EXPECT_EQ(list.Remove(p->GetId()), ListReturnCode::Success);
    EXPECT_FALSE(list.Has(p));
}

TEST(PartListTest, RemoveByPointer) {
    PartList<Part> list;
    auto p = std::make_shared<TestPart>();
    list.Add(p);
    EXPECT_EQ(list.Remove(p), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListTest, RemoveAll) {
    PartList<Part> list;
    list.Add(std::make_shared<TestPart>());
    list.Add(std::make_shared<TestPart>());
    EXPECT_EQ(list.Remove(), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListTest, RemoveNotFound) {
    PartList<Part> list;
    auto p = std::make_shared<TestPart>();
    EXPECT_EQ(list.Remove(p), ListReturnCode::NotFound);
}

TEST(PartListTest, HasById) {
    PartList<Part> list;
    auto p = std::make_shared<TestPart>();
    list.Add(p);
    EXPECT_TRUE(list.Has(p->GetId()));
    EXPECT_FALSE(list.Has(p->GetId() + 999));
}

TEST(PartListTest, HasByName) {
    PartList<Part> list;
    auto p = std::make_shared<NamedPart>("Foo");
    list.Add(p);
    EXPECT_TRUE(list.Has(std::string("Foo")));
    EXPECT_FALSE(list.Has(std::string("Bar")));
}

TEST(PartListTest, GetById) {
    PartList<Part> list;
    auto p = std::make_shared<TestPart>();
    list.Add(p);
    auto found = list.Get(p->GetId());
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetId(), p->GetId());
}

TEST(PartListTest, GetAll) {
    PartList<Part> list;
    list.Add(std::make_shared<TestPart>());
    list.Add(std::make_shared<TestPart>());
    auto all = list.Get();
    EXPECT_EQ(all->size(), 2u);
}

TEST(PartListTest, GetByType) {
    PartList<Part> list;
    list.Add(std::make_shared<NamedPart>("a"));
    list.Add(std::make_shared<DerivedPart>("b"));
    list.Add(std::make_shared<NamedPart>("c"));

    auto derived = list.Get<DerivedPart>();
    ASSERT_EQ(derived->size(), 1u);
    EXPECT_EQ((*derived)[0]->GetName(), "b");
}

TEST(PartListTest, HasByType) {
    PartList<Part> list;
    EXPECT_FALSE(list.Has<DerivedPart>());
    list.Add(std::make_shared<NamedPart>("a"));
    EXPECT_FALSE(list.Has<DerivedPart>());
    list.Add(std::make_shared<DerivedPart>("d"));
    EXPECT_TRUE(list.Has<DerivedPart>());
}

TEST(PartListTest, HasByTypeAndName) {
    PartList<Part> list;
    list.Add(std::make_shared<DerivedPart>("Alpha"));
    EXPECT_TRUE(list.Has<DerivedPart>(std::string("Alpha")));
    EXPECT_FALSE(list.Has<DerivedPart>(std::string("Beta")));
}

TEST(PartListTest, RemoveByType) {
    PartList<Part> list;
    list.Add(std::make_shared<NamedPart>("keep"));
    list.Add(std::make_shared<DerivedPart>("remove"));
    list.Add(std::make_shared<NamedPart>("keep2"));

    EXPECT_EQ(list.Remove<DerivedPart>(), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 2u);
    EXPECT_FALSE(list.Has<DerivedPart>());
}

TEST(PartListTest, GetByMultipleIds) {
    PartList<Part> list;
    auto a = std::make_shared<TestPart>();
    auto b = std::make_shared<TestPart>();
    auto c = std::make_shared<TestPart>();
    list.Add(a);
    list.Add(b);
    list.Add(c);

    auto result = list.Get(std::vector<uint64_t>{ a->GetId(), c->GetId() });
    EXPECT_EQ(result->size(), 2u);
}

TEST(PartListTest, AddMultiple) {
    PartList<Part> list;
    std::vector<std::shared_ptr<Part>> parts = { std::make_shared<TestPart>(), std::make_shared<TestPart>() };
    EXPECT_EQ(list.Add(parts), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 2u);
}

TEST(PartListTest, RemoveMultipleByPointer) {
    PartList<Part> list;
    auto a = std::make_shared<TestPart>();
    auto b = std::make_shared<TestPart>();
    list.Add(a);
    list.Add(b);
    EXPECT_EQ(list.Remove(std::vector<std::shared_ptr<Part>>{ a, b }), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListTest, RemoveMultipleByIds) {
    PartList<Part> list;
    auto a = std::make_shared<TestPart>();
    auto b = std::make_shared<TestPart>();
    list.Add(a);
    list.Add(b);
    EXPECT_EQ(list.Remove(std::vector<uint64_t>{ a->GetId(), b->GetId() }), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListTest, EmptyList) {
    PartList<Part> list;
    EXPECT_FALSE(list.Has());
    EXPECT_EQ(list.GetCount(), 0u);
    list.Add(std::make_shared<TestPart>());
    EXPECT_TRUE(list.Has());
}

TEST(PartListTest, DirectListAccess) {
    PartList<Part> list;
    list.Add(std::make_shared<TestPart>());
    auto& vec = list.GetList();
    EXPECT_EQ(vec.size(), 1u);

    const PartList<Part>& constList = list;
    const auto& constVec = constList.GetList();
    EXPECT_EQ(constVec.size(), 1u);
}
