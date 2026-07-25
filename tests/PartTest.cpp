#include <gtest/gtest.h>
#include "ship/Part.h"
#include "ship/PartList.h"

using namespace Ship;

namespace {
class HookCountingPart : public Part {
  public:
    int onAddedCount = 0;
    int onRemovedCount = 0;

    void OnAdded(bool /*forced*/) override {
        onAddedCount++;
    }

    void OnRemoved(bool /*forced*/) override {
        onRemovedCount++;
    }
};

class HookCountingPartList : public PartList<HookCountingPart> {
  public:
    int addedHookCount = 0;
    int removedHookCount = 0;

  protected:
    void Added(std::shared_ptr<HookCountingPart> /*part*/, const bool /*forced*/) override {
        addedHookCount++;
    }

    void Removed(std::shared_ptr<HookCountingPart> /*part*/, const bool /*forced*/) override {
        removedHookCount++;
    }
};
} // namespace

// Part has only GetId() and operator== — no GetName/ToString.
// Name-based operations are on Component/ComponentList.

// ---- Part tests ----

TEST(PartTest, UniqueIds) {
    Part a;
    Part b;
    EXPECT_NE(a.GetId(), b.GetId());
}

TEST(PartTest, Equality) {
    Part a;
    Part b;
    EXPECT_TRUE(a == a);
    EXPECT_FALSE(a == b);
}

// ---- PartList tests ----

TEST(PartListTest, AddAndHas) {
    PartList<Part> list;
    auto p = std::make_shared<Part>();
    EXPECT_FALSE(list.Has(p));
    EXPECT_EQ(list.Add(p), ListReturnCode::Success);
    EXPECT_TRUE(list.Has(p));
    EXPECT_EQ(list.GetCount(), 1u);
}

TEST(PartListTest, AddDuplicate) {
    PartList<Part> list;
    auto p = std::make_shared<Part>();
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
    auto p = std::make_shared<Part>();
    list.Add(p);
    EXPECT_EQ(list.Remove(p->GetId()), ListReturnCode::Success);
    EXPECT_FALSE(list.Has(p));
}

TEST(PartListTest, RemoveByPointer) {
    PartList<Part> list;
    auto p = std::make_shared<Part>();
    list.Add(p);
    EXPECT_EQ(list.Remove(p), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListTest, RemoveAll) {
    PartList<Part> list;
    list.Add(std::make_shared<Part>());
    list.Add(std::make_shared<Part>());
    EXPECT_EQ(list.Remove(), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListTest, RemoveNotFound) {
    PartList<Part> list;
    auto p = std::make_shared<Part>();
    EXPECT_EQ(list.Remove(p), ListReturnCode::NotFound);
}

TEST(PartListTest, HasById) {
    PartList<Part> list;
    auto p = std::make_shared<Part>();
    list.Add(p);
    EXPECT_TRUE(list.Has(p->GetId()));
    EXPECT_FALSE(list.Has(p->GetId() + 999));
}

TEST(PartListTest, GetById) {
    PartList<Part> list;
    auto p = std::make_shared<Part>();
    list.Add(p);
    auto found = list.Get(p->GetId());
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetId(), p->GetId());
}

TEST(PartListTest, GetAll) {
    PartList<Part> list;
    list.Add(std::make_shared<Part>());
    list.Add(std::make_shared<Part>());
    auto all = list.Get();
    EXPECT_EQ(all->size(), 2u);
}

TEST(PartListTest, GetByMultipleIds) {
    PartList<Part> list;
    auto a = std::make_shared<Part>();
    auto b = std::make_shared<Part>();
    auto c = std::make_shared<Part>();
    list.Add(a);
    list.Add(b);
    list.Add(c);

    auto result = list.Get(std::vector<uint64_t>{ a->GetId(), c->GetId() });
    EXPECT_EQ(result->size(), 2u);
}

TEST(PartListTest, AddMultiple) {
    PartList<Part> list;
    std::vector<std::shared_ptr<Part>> parts = { std::make_shared<Part>(), std::make_shared<Part>() };
    EXPECT_EQ(list.Add(parts), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 2u);
}

TEST(PartListTest, RemoveMultipleByPointer) {
    PartList<Part> list;
    auto a = std::make_shared<Part>();
    auto b = std::make_shared<Part>();
    list.Add(a);
    list.Add(b);
    EXPECT_EQ(list.Remove(std::vector<std::shared_ptr<Part>>{ a, b }), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListTest, RemoveMultipleByIds) {
    PartList<Part> list;
    auto a = std::make_shared<Part>();
    auto b = std::make_shared<Part>();
    list.Add(a);
    list.Add(b);
    EXPECT_EQ(list.Remove(std::vector<uint64_t>{ a->GetId(), b->GetId() }), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListTest, EmptyList) {
    PartList<Part> list;
    EXPECT_FALSE(list.Has());
    EXPECT_EQ(list.GetCount(), 0u);
    list.Add(std::make_shared<Part>());
    EXPECT_TRUE(list.Has());
}

TEST(PartListTest, DirectListAccess) {
    PartList<Part> list;
    list.Add(std::make_shared<Part>());
    auto vec = list.Get();
    EXPECT_EQ(vec->size(), 1u);

    const PartList<Part>& constList = list;
    auto constVec = constList.Get();
    EXPECT_EQ(constVec->size(), 1u);
}

// ---- GetFirst<T>() tests (with Component subclasses) ----
#include "ship/Component.h"

class TypeA : public Component {
  public:
    TypeA() : Component("TypeA") {
    }
};

class TypeB : public Component {
  public:
    TypeB() : Component("TypeB") {
    }
};

TEST(PartListTest, GetFirstByType) {
    PartList<Component> list;
    auto a = std::make_shared<TypeA>();
    auto b = std::make_shared<TypeB>();
    list.Add(a);
    list.Add(b);

    auto found = list.GetFirst<TypeA>();
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "TypeA");
}

TEST(PartListTest, GetFirstByTypeNotFound) {
    PartList<Component> list;
    list.Add(std::make_shared<TypeA>());
    auto found = list.GetFirst<TypeB>();
    EXPECT_EQ(found, nullptr);
}

TEST(PartListTest, GetFirstByTypeFromEmpty) {
    PartList<Component> list;
    auto found = list.GetFirst<TypeA>();
    EXPECT_EQ(found, nullptr);
}

TEST(PartListTest, GetFirstReturnsFirstMatch) {
    PartList<Component> list;
    auto a1 = std::make_shared<TypeA>();
    auto a2 = std::make_shared<TypeA>();
    list.Add(a1);
    list.Add(a2);

    auto found = list.GetFirst<TypeA>();
    EXPECT_EQ(found, a1);
}

// ---- PartList storage policy tests ----

TEST(PartListStoragePolicyTest, StrongStorageOwnsPartUntilRemoved) {
    PartList<Part, std::shared_ptr<Part>> list;

    auto part = std::make_shared<Part>();
    std::weak_ptr<Part> weakPart = part;
    const uint64_t id = part->GetId();

    ASSERT_EQ(list.Add(part), ListReturnCode::Success);
    part.reset();

    EXPECT_FALSE(weakPart.expired());
    EXPECT_TRUE(list.Has(id));
    EXPECT_EQ(list.GetCount(), 1u);

    EXPECT_EQ(list.Remove(id), ListReturnCode::Success);
    EXPECT_TRUE(weakPart.expired());
}

TEST(PartListStoragePolicyTest, WeakStorageDoesNotOwnPart) {
    PartList<Part, std::weak_ptr<Part>> list;

    auto part = std::make_shared<Part>();
    std::weak_ptr<Part> weakPart = part;
    const uint64_t id = part->GetId();

    ASSERT_EQ(list.Add(part), ListReturnCode::Success);
    part.reset();

    EXPECT_TRUE(weakPart.expired());
    EXPECT_EQ(list.GetCount(), 0u);
    EXPECT_FALSE(list.Has(id));
    EXPECT_FALSE(list.Has());
}

TEST(PartListStoragePolicyTest, WeakStoragePrunesExpiredBeforeDuplicateCheck) {
    PartList<Part, std::weak_ptr<Part>> list;

    auto part = std::make_shared<Part>();
    ASSERT_EQ(list.Add(part), ListReturnCode::Success);
    ASSERT_EQ(list.Add(part), ListReturnCode::Duplicate);

    part.reset();

    auto replacement = std::make_shared<Part>();
    EXPECT_EQ(list.Add(replacement), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 1u);
}

TEST(PartListStoragePolicyTest, WeakStorageRemoveExpiredByIdReturnsNotFound) {
    PartList<Part, std::weak_ptr<Part>> list;

    auto part = std::make_shared<Part>();
    const uint64_t id = part->GetId();

    ASSERT_EQ(list.Add(part), ListReturnCode::Success);
    part.reset();

    EXPECT_EQ(list.Remove(id), ListReturnCode::NotFound);
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(PartListStoragePolicyTest, WeakStoragePrunesExpiredOnGetSnapshot) {
    PartList<Part, std::weak_ptr<Part>> list;

    auto alive = std::make_shared<Part>();
    auto expired = std::make_shared<Part>();
    const uint64_t aliveId = alive->GetId();

    ASSERT_EQ(list.Add(alive), ListReturnCode::Success);
    ASSERT_EQ(list.Add(expired), ListReturnCode::Success);

    expired.reset();

    auto snapshot = list.Get();
    ASSERT_EQ(snapshot->size(), 1u);
    EXPECT_EQ((*snapshot)[0]->GetId(), aliveId);
    EXPECT_EQ(list.GetCount(), 1u);
}

TEST(PartListStoragePolicyTest, WeakStoragePrunesExpiredOnGetByIds) {
    PartList<Part, std::weak_ptr<Part>> list;

    auto alive = std::make_shared<Part>();
    auto expired = std::make_shared<Part>();
    const uint64_t aliveId = alive->GetId();
    const uint64_t expiredId = expired->GetId();

    ASSERT_EQ(list.Add(alive), ListReturnCode::Success);
    ASSERT_EQ(list.Add(expired), ListReturnCode::Success);

    expired.reset();

    auto result = list.Get(std::vector<uint64_t>{ aliveId, expiredId });
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ((*result)[0]->GetId(), aliveId);
    EXPECT_FALSE(list.Has(expiredId));
    EXPECT_EQ(list.GetCount(), 1u);
}

TEST(PartListStoragePolicyTest, WeakStoragePrunesExpiredOnRemoveAll) {
    PartList<Part, std::weak_ptr<Part>> list;

    auto alive = std::make_shared<Part>();
    auto expired = std::make_shared<Part>();

    ASSERT_EQ(list.Add(alive), ListReturnCode::Success);
    ASSERT_EQ(list.Add(expired), ListReturnCode::Success);

    expired.reset();

    EXPECT_EQ(list.Remove(), ListReturnCode::Success);
    EXPECT_EQ(list.GetCount(), 0u);
    EXPECT_FALSE(list.Has());
}

TEST(PartListStoragePolicyTest, WeakStoragePrunesExpiredOnTypedQueries) {
    PartList<Component, std::weak_ptr<Component>> list;

    auto alive = std::make_shared<TypeA>();
    auto expired = std::make_shared<TypeA>();

    ASSERT_EQ(list.Add(alive), ListReturnCode::Success);
    ASSERT_EQ(list.Add(expired), ListReturnCode::Success);

    expired.reset();

    EXPECT_TRUE(list.Has<TypeA>());
    auto first = list.GetFirst<TypeA>();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->GetId(), alive->GetId());

    auto typed = list.Get<TypeA>();
    EXPECT_EQ(typed->size(), 1u);
    EXPECT_EQ(list.GetCount(), 1u);
}

TEST(PartListHookLifecycleTest, AddedAndOnAddedCalledExactlyOnceOnEffectiveAdd) {
    HookCountingPartList list;
    auto part = std::make_shared<HookCountingPart>();

    EXPECT_EQ(list.Add(part), ListReturnCode::Success);
    EXPECT_EQ(list.addedHookCount, 1);
    EXPECT_EQ(part->onAddedCount, 1);

    EXPECT_EQ(list.Add(part), ListReturnCode::Duplicate);
    EXPECT_EQ(list.addedHookCount, 1);
    EXPECT_EQ(part->onAddedCount, 1);
}

TEST(PartListHookLifecycleTest, RemovedAndOnRemovedCalledExactlyOnceOnEffectiveRemove) {
    HookCountingPartList list;
    auto part = std::make_shared<HookCountingPart>();

    ASSERT_EQ(list.Add(part), ListReturnCode::Success);
    EXPECT_EQ(list.Remove(part), ListReturnCode::Success);
    EXPECT_EQ(list.removedHookCount, 1);
    EXPECT_EQ(part->onRemovedCount, 1);

    EXPECT_EQ(list.Remove(part), ListReturnCode::NotFound);
    EXPECT_EQ(list.removedHookCount, 1);
    EXPECT_EQ(part->onRemovedCount, 1);
}
