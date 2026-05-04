#include <gtest/gtest.h>
#include "ship/Action.h"
#include "ship/Tickable.h"
#include "ship/events/EventTypes.h"

using namespace Ship;

// Test EventIDs used throughout this file
static constexpr EventID kTickEvent = 1;
static constexpr EventID kDrawEvent = 2;
static constexpr EventID kDrawDebugMenuEvent = 3;

// A concrete Action for testing.
class TestAction : public Action {
  public:
    TestAction(EventID eventId, std::shared_ptr<Tickable> tickable)
        : Action(eventId, tickable), mRunCount(0), mLastDuration(0.0) {
    }

    int mRunCount;
    double mLastDuration;

  protected:
    bool ActionRan(const double durationSinceLastTick) override {
        mRunCount++;
        mLastDuration = durationSinceLastTick;
        return true;
    }
};

// A derived TestAction for type filtering.
class DerivedTestAction : public TestAction {
  public:
    DerivedTestAction(EventID eventId, std::shared_ptr<Tickable> tickable)
        : TestAction(eventId, tickable) {
    }
};

// A simple Tickable to act as the owner.
class TestTickable : public Tickable {
  public:
    TestTickable(bool isTicking = true) : Tickable(isTicking) {
    }
};

// ---- Action lifecycle tests ----

TEST(ActionTest, InitialState) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(kTickEvent, tickable);

    EXPECT_FALSE(action->IsRunning());
    EXPECT_EQ(action->GetEventId(), kTickEvent);
    EXPECT_NE(action->GetTickable(), nullptr);
}

TEST(ActionTest, StartStop) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(kTickEvent, tickable);

    EXPECT_TRUE(action->Start());
    EXPECT_TRUE(action->IsRunning());

    EXPECT_TRUE(action->Stop());
    EXPECT_FALSE(action->IsRunning());
}

TEST(ActionTest, StartWhenAlreadyStarted) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(kTickEvent, tickable);
    action->Start();
    EXPECT_TRUE(action->Start()); // idempotent
    EXPECT_TRUE(action->IsRunning());
}

TEST(ActionTest, StopWhenAlreadyStopped) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(kTickEvent, tickable);
    EXPECT_TRUE(action->Stop()); // idempotent
    EXPECT_FALSE(action->IsRunning());
}

TEST(ActionTest, RunWhenRunning) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(kTickEvent, tickable);
    action->Start();

    EXPECT_TRUE(action->Run(0.016));
    EXPECT_EQ(action->mRunCount, 1);
    EXPECT_DOUBLE_EQ(action->mLastDuration, 0.016);
}

TEST(ActionTest, RunWhenNotRunning) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(kTickEvent, tickable);

    EXPECT_FALSE(action->Run(0.016));
    EXPECT_EQ(action->mRunCount, 0);
}

TEST(ActionTest, GetEventId) {
    auto tickable = std::make_shared<TestTickable>();
    auto action1 = std::make_shared<TestAction>(kTickEvent, tickable);
    auto action2 = std::make_shared<TestAction>(kDrawEvent, tickable);

    EXPECT_EQ(action1->GetEventId(), kTickEvent);
    EXPECT_EQ(action2->GetEventId(), kDrawEvent);
}

TEST(ActionTest, DifferentEventIdsAreDifferent) {
    auto tickable = std::make_shared<TestTickable>();
    auto action1 = std::make_shared<TestAction>(kTickEvent, tickable);
    auto action2 = std::make_shared<TestAction>(kDrawEvent, tickable);
    EXPECT_NE(action1->GetEventId(), action2->GetEventId());
}

// ---- Weak reference tests (Action -> Tickable) ----

TEST(ActionTest, TickableWeakReference) {
    std::shared_ptr<TestAction> action;
    {
        auto tickable = std::make_shared<TestTickable>();
        action = std::make_shared<TestAction>(kTickEvent, tickable);
        EXPECT_NE(action->GetTickable(), nullptr);
    }
    // After tickable is destroyed, GetTickable() should return nullptr.
    EXPECT_EQ(action->GetTickable(), nullptr);
}

TEST(ActionTest, UniqueIds) {
    auto tickable = std::make_shared<TestTickable>();
    auto a1 = std::make_shared<TestAction>(kTickEvent, tickable);
    auto a2 = std::make_shared<TestAction>(kTickEvent, tickable);
    EXPECT_NE(a1->GetId(), a2->GetId());
}

// ---- ActionList tests ----

#include "ship/ActionList.h"

TEST(ActionListTest, AddAndGetByEventId) {
    ActionList list;
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(kTickEvent, tickable);
    list.Add(action);

    auto tickActions = list.Get(kTickEvent);
    EXPECT_EQ(tickActions->size(), 1u);

    auto drawActions = list.Get(kDrawEvent);
    EXPECT_EQ(drawActions->size(), 0u);
}

TEST(ActionListTest, GetByMultipleEventIds) {
    ActionList list;
    auto tickable = std::make_shared<TestTickable>();
    list.Add(std::make_shared<TestAction>(kTickEvent, tickable));
    list.Add(std::make_shared<TestAction>(kDrawEvent, tickable));
    list.Add(std::make_shared<TestAction>(kDrawDebugMenuEvent, tickable));

    auto result = list.Get(std::vector<EventID>{ kTickEvent, kDrawDebugMenuEvent });
    EXPECT_EQ(result->size(), 2u);
}

TEST(ActionListTest, HasByEventId) {
    ActionList list;
    auto tickable = std::make_shared<TestTickable>();
    list.Add(std::make_shared<TestAction>(kTickEvent, tickable));

    EXPECT_TRUE(list.Has(kTickEvent));
    EXPECT_FALSE(list.Has(kDrawEvent));
}
