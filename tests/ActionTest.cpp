#include <gtest/gtest.h>
#include "ship/Action.h"
#include "ship/Tickable.h"

using namespace Ship;

// A concrete Action for testing.
class TestAction : public Action {
  public:
    TestAction(const std::string& eventName, std::shared_ptr<Tickable> tickable)
        : Action(eventName, tickable), mRunCount(0), mLastDuration(0.0) {
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
    DerivedTestAction(const std::string& eventName, std::shared_ptr<Tickable> tickable)
        : TestAction(eventName, tickable) {
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
    auto action = std::make_shared<TestAction>("Tick", tickable);

    EXPECT_FALSE(action->IsRunning());
    EXPECT_EQ(action->GetEventName(), "Tick");
    EXPECT_NE(action->GetTickable(), nullptr);
}

TEST(ActionTest, StartStop) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>("Tick", tickable);

    EXPECT_TRUE(action->Start());
    EXPECT_TRUE(action->IsRunning());

    EXPECT_TRUE(action->Stop());
    EXPECT_FALSE(action->IsRunning());
}

TEST(ActionTest, StartWhenAlreadyStarted) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>("Tick", tickable);
    action->Start();
    EXPECT_TRUE(action->Start()); // idempotent
    EXPECT_TRUE(action->IsRunning());
}

TEST(ActionTest, StopWhenAlreadyStopped) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>("Tick", tickable);
    EXPECT_TRUE(action->Stop()); // idempotent
    EXPECT_FALSE(action->IsRunning());
}

TEST(ActionTest, RunWhenRunning) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>("Tick", tickable);
    action->Start();

    EXPECT_TRUE(action->Run(0.016));
    EXPECT_EQ(action->mRunCount, 1);
    EXPECT_DOUBLE_EQ(action->mLastDuration, 0.016);
}

TEST(ActionTest, RunWhenNotRunning) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>("Tick", tickable);

    EXPECT_FALSE(action->Run(0.016));
    EXPECT_EQ(action->mRunCount, 0);
}

TEST(ActionTest, GetEventName) {
    auto tickable = std::make_shared<TestTickable>();
    auto action1 = std::make_shared<TestAction>("Tick", tickable);
    auto action2 = std::make_shared<TestAction>("Draw", tickable);

    EXPECT_EQ(action1->GetEventName(), "Tick");
    EXPECT_EQ(action2->GetEventName(), "Draw");
}

TEST(ActionTest, DifferentEventNamesHaveDifferentTypes) {
    auto tickable = std::make_shared<TestTickable>();
    auto action1 = std::make_shared<TestAction>("Tick", tickable);
    auto action2 = std::make_shared<TestAction>("Draw", tickable);
    // Hash-based types should differ for different event names
    EXPECT_NE(action1->GetType(), action2->GetType());
}

// ---- Weak reference tests (Action -> Tickable) ----

TEST(ActionTest, TickableWeakReference) {
    std::shared_ptr<TestAction> action;
    {
        auto tickable = std::make_shared<TestTickable>();
        action = std::make_shared<TestAction>("Tick", tickable);
        EXPECT_NE(action->GetTickable(), nullptr);
    }
    // After tickable is destroyed, GetTickable() should return nullptr.
    EXPECT_EQ(action->GetTickable(), nullptr);
}

TEST(ActionTest, UniqueIds) {
    auto tickable = std::make_shared<TestTickable>();
    auto a1 = std::make_shared<TestAction>("Tick", tickable);
    auto a2 = std::make_shared<TestAction>("Tick", tickable);
    EXPECT_NE(a1->GetId(), a2->GetId());
}

// ---- ActionList tests ----

#include "ship/ActionList.h"

TEST(ActionListTest, AddAndGetByEventName) {
    ActionList list;
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>("Tick", tickable);
    list.Add(action);

    auto tickActions = list.Get(std::string("Tick"));
    EXPECT_EQ(tickActions->size(), 1u);

    auto drawActions = list.Get(std::string("Draw"));
    EXPECT_EQ(drawActions->size(), 0u);
}

TEST(ActionListTest, GetByMultipleEventNames) {
    ActionList list;
    auto tickable = std::make_shared<TestTickable>();
    list.Add(std::make_shared<TestAction>("Tick", tickable));
    list.Add(std::make_shared<TestAction>("Draw", tickable));
    list.Add(std::make_shared<TestAction>("DrawDebugMenu", tickable));

    auto result = list.Get(std::vector<std::string>{ "Tick", "DrawDebugMenu" });
    EXPECT_EQ(result->size(), 2u);
}

TEST(ActionListTest, HasByEventName) {
    ActionList list;
    auto tickable = std::make_shared<TestTickable>();
    list.Add(std::make_shared<TestAction>("Tick", tickable));

    EXPECT_TRUE(list.Has(std::string("Tick")));
    EXPECT_FALSE(list.Has(std::string("Draw")));
}
