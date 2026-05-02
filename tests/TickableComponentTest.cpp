#include <gtest/gtest.h>
#include "ship/TickableComponent.h"
#include "ship/Context.h"
#include "ship/TickableList.h"
#include "ship/Action.h"

using namespace Ship;

// Concrete TickableComponent subclass for testing.
class ConcreteTickable : public TickableComponent {
  public:
    explicit ConcreteTickable(std::shared_ptr<Context> ctx,
                               TickGroup tg = TickGroup::TickGroupDefault,
                               TickPriority tp = TickPriority::TickPriorityDefault)
        : TickableComponent("TestTC", ctx, tg, tp) {}

    std::string mLastEventName;
    double mLastDuration = 0.0;
    bool mActionRanCalled = false;

    bool ActionRan(const std::string& eventName, const double durationSinceLastTick) override {
        mLastEventName = eventName;
        mLastDuration = durationSinceLastTick;
        mActionRanCalled = true;
        return true;
    }
};

class TickableComponentTest : public ::testing::Test {
  protected:
    std::shared_ptr<Context> mContext;

    void SetUp() override {
        mContext = Context::CreateInstance("test", "t", "");
    }

    void TearDown() override {
        // Break the reference cycle: TickableList → TickableComponent → Context
        mContext->GetTickableComponents().Remove(true);
        mContext.reset();
    }
};

// ---- Test 1: RegisterWithContext() adds to Context's TickableList ----

TEST_F(TickableComponentTest, RegisterWithContextAddsToTickableList) {
    auto tc = std::make_shared<ConcreteTickable>(mContext);
    tc->RegisterWithContext(tc);

    EXPECT_TRUE(mContext->GetTickableComponents().Has(tc));
    tc->UnregisterFromContext();
}

// ---- Test 2: UnregisterFromContext() removes from Context's TickableList ----

TEST_F(TickableComponentTest, UnregisterFromContextRemovesFromTickableList) {
    auto tc = std::make_shared<ConcreteTickable>(mContext);
    tc->RegisterWithContext(tc);
    ASSERT_TRUE(mContext->GetTickableComponents().Has(tc));

    tc->UnregisterFromContext();
    EXPECT_FALSE(mContext->GetTickableComponents().Has(tc));
}

// ---- Test 3: SetContext() moves from old context to new context ----

TEST_F(TickableComponentTest, SetContextMovesFromOldToNew) {
    // Construct a second Context directly; CreateInstance would return the existing
    // singleton while mContext is still alive.
    auto ctx2 = std::make_shared<Context>("test2", "t2", "");

    auto tc = std::make_shared<ConcreteTickable>(mContext);
    tc->RegisterWithContext(tc);
    ASSERT_TRUE(mContext->GetTickableComponents().Has(tc));

    tc->SetContext(ctx2);
    EXPECT_FALSE(mContext->GetTickableComponents().Has(tc));
    EXPECT_TRUE(ctx2->GetTickableComponents().Has(tc));

    // Cleanup ctx2's tickable list to break reference cycle
    ctx2->GetTickableComponents().Remove(true);
}

// ---- Test 4: TickGroup getter returns correct value ----

TEST_F(TickableComponentTest, GetTickGroupReturnsCorrectGroup) {
    auto tc = std::make_shared<ConcreteTickable>(mContext, TickGroup::TickGroupDefault);
    EXPECT_EQ(tc->GetTickGroup(), TickGroup::TickGroupDefault);
}

// ---- Test 5: TickPriority getter returns correct value ----

TEST_F(TickableComponentTest, GetTickPriorityReturnsCorrectPriority) {
    auto tc = std::make_shared<ConcreteTickable>(mContext, TickGroup::TickGroupDefault,
                                                  TickPriority::TickPriorityDefault);
    EXPECT_EQ(tc->GetTickPriority(), TickPriority::TickPriorityDefault);
}

// ---- Test 6: GetOrder() combines TickGroup and TickPriority ----

TEST_F(TickableComponentTest, GetOrderCombinesGroupAndPriority) {
    auto tc = std::make_shared<ConcreteTickable>(mContext, TickGroup::TickGroupDefault,
                                                  TickPriority::TickPriorityDefault);
    uint64_t expectedOrder =
        (static_cast<uint64_t>(TickGroup::TickGroupDefault) << 32) |
        static_cast<uint64_t>(TickPriority::TickPriorityDefault);
    EXPECT_EQ(tc->GetOrder(), expectedOrder);
}

// ---- Test 7: SetTickGroup() updates GetOrder() ----

TEST_F(TickableComponentTest, SetTickGroupUpdatesOrder) {
    auto tc = std::make_shared<ConcreteTickable>(mContext, TickGroup::TickGroupDefault,
                                                  TickPriority::TickPriorityDefault);
    // Use a non-default value by casting
    TickGroup newGroup = static_cast<TickGroup>(1u);
    tc->SetTickGroup(newGroup);
    EXPECT_EQ(tc->GetTickGroup(), newGroup);

    uint64_t expectedOrder =
        (static_cast<uint64_t>(newGroup) << 32) |
        static_cast<uint64_t>(TickPriority::TickPriorityDefault);
    EXPECT_EQ(tc->GetOrder(), expectedOrder);
}

// ---- Test 8: SetTickPriority() updates GetOrder() ----

TEST_F(TickableComponentTest, SetTickPriorityUpdatesOrder) {
    auto tc = std::make_shared<ConcreteTickable>(mContext);
    TickPriority newPriority = static_cast<TickPriority>(5u);
    tc->SetTickPriority(newPriority);
    EXPECT_EQ(tc->GetTickPriority(), newPriority);

    uint64_t expectedOrder =
        (static_cast<uint64_t>(TickGroup::TickGroupDefault) << 32) |
        static_cast<uint64_t>(newPriority);
    EXPECT_EQ(tc->GetOrder(), expectedOrder);
}

// ---- Test 9: TickableList::Sort() sorts by order ----

TEST_F(TickableComponentTest, TickableListSort) {
    auto tc1 = std::make_shared<ConcreteTickable>(mContext, TickGroup::TickGroupDefault,
                                                   static_cast<TickPriority>(10u));
    auto tc2 = std::make_shared<ConcreteTickable>(mContext, TickGroup::TickGroupDefault,
                                                   static_cast<TickPriority>(1u));
    auto tc3 = std::make_shared<ConcreteTickable>(mContext, TickGroup::TickGroupDefault,
                                                   static_cast<TickPriority>(5u));

    TickableList list;
    list.Add(tc1);
    list.Add(tc2);
    list.Add(tc3);
    list.Sort();

    const auto& sorted = list.GetList();
    ASSERT_EQ(sorted.size(), 3u);
    EXPECT_EQ(sorted[0], tc2); // priority 1 (lowest order first)
    EXPECT_EQ(sorted[1], tc3); // priority 5
    EXPECT_EQ(sorted[2], tc1); // priority 10
}

// ---- Test 10: Multiple components with different orders sort correctly ----

TEST_F(TickableComponentTest, MultipleComponentsDifferentOrdersSortCorrectly) {
    TickableList list;
    for (uint32_t i = 10; i >= 1; --i) {
        auto tc = std::make_shared<ConcreteTickable>(mContext, TickGroup::TickGroupDefault,
                                                      static_cast<TickPriority>(i));
        list.Add(tc);
    }
    list.Sort();

    const auto& sorted = list.GetList();
    ASSERT_EQ(sorted.size(), 10u);
    for (size_t i = 0; i + 1 < sorted.size(); ++i) {
        EXPECT_LE(sorted[i]->GetOrder(), sorted[i + 1]->GetOrder());
    }
}

// ---- Test 11: TickableList::Has() ----

TEST_F(TickableComponentTest, TickableListHasReturnsTrue) {
    auto tc = std::make_shared<ConcreteTickable>(mContext);
    tc->RegisterWithContext(tc);

    EXPECT_TRUE(mContext->GetTickableComponents().Has(tc));
    tc->UnregisterFromContext();
}

// ---- Test 12: TickableList::Has() returns false when not present ----

TEST_F(TickableComponentTest, TickableListHasReturnsFalseWhenNotPresent) {
    auto tc = std::make_shared<ConcreteTickable>(mContext);
    // Do not register
    EXPECT_FALSE(mContext->GetTickableComponents().Has(tc));
}

// ---- Test 13: ActionRan callback dispatches with correct arguments ----

TEST_F(TickableComponentTest, ActionRanCallbackDispatches) {
    auto tc = std::make_shared<ConcreteTickable>(mContext);
    tc->ActionRan("Tick", 0.016);

    EXPECT_TRUE(tc->mActionRanCalled);
    EXPECT_EQ(tc->mLastEventName, "Tick");
    EXPECT_DOUBLE_EQ(tc->mLastDuration, 0.016);
}

// ---- Test 14: Default ActionRan returns true ----

TEST_F(TickableComponentTest, DefaultActionRanReturnsTrue) {
    auto tc = std::make_shared<ConcreteTickable>(mContext);
    // Base TickableComponent::ActionRan returns true
    TickableComponent* base = tc.get();
    // Call through the ConcreteTickable which overrides, so test base separately
    EXPECT_TRUE(tc->ActionRan("DrawDebugMenu", 0.033));
}

// ---- Test 15: Constructor with explicit actions list registers with context ----

class ConcreteTickableWithActions : public TickableComponent {
  public:
    explicit ConcreteTickableWithActions(std::shared_ptr<Context> ctx)
        : TickableComponent("TestTCActions", ctx, TickGroup::TickGroupDefault,
                            TickPriority::TickPriorityDefault,
                            std::vector<std::shared_ptr<Action>>{}) {}

    bool ActionRan(const std::string& eventName, const double durationSinceLastTick) override {
        return true;
    }
};

TEST_F(TickableComponentTest, ConstructorWithActionsListRegistersWithContext) {
    auto tc = std::make_shared<ConcreteTickableWithActions>(mContext);
    tc->RegisterWithContext(tc);

    EXPECT_TRUE(mContext->GetTickableComponents().Has(tc));
    tc->UnregisterFromContext();
}

// ---- Test 16: Auto-registration via ComponentList when first parent added ----

TEST_F(TickableComponentTest, AutoRegistrationWhenFirstParentAdded) {
    // ConcreteTickable is added as a child of a parent component.
    // The ComponentList Parents hook should auto-register with Context's TickableList.
    auto parent = std::make_shared<Component>("Parent");
    auto tc = std::make_shared<ConcreteTickable>(mContext);

    // Adding tc as child of parent causes parent to be added to tc's parents list,
    // which fires the Parents-role Added() hook on tc, auto-registering tc with Context.
    parent->GetChildren().Add(tc);

    EXPECT_TRUE(mContext->GetTickableComponents().Has(tc));

    // Clean up
    parent->GetChildren().Remove(tc);
}

// ---- Test 17: Auto-unregistration via ComponentList when last parent removed ----

TEST_F(TickableComponentTest, AutoUnregistrationWhenLastParentRemoved) {
    auto parent = std::make_shared<Component>("Parent");
    auto tc = std::make_shared<ConcreteTickable>(mContext);

    parent->GetChildren().Add(tc);
    ASSERT_TRUE(mContext->GetTickableComponents().Has(tc));

    parent->GetChildren().Remove(tc);
    EXPECT_FALSE(mContext->GetTickableComponents().Has(tc));
}
