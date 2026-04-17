#include <gtest/gtest.h>
#include "ship/Action.h"
#include "ship/Tickable.h"

using namespace Ship;

// A concrete Action for testing.
class TestAction : public Action {
  public:
    TestAction(uint32_t actionType, std::shared_ptr<Tickable> tickable)
        : Action(actionType, tickable), mRunCount(0), mLastDuration(0.0) {
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
    DerivedTestAction(uint32_t actionType, std::shared_ptr<Tickable> tickable) : TestAction(actionType, tickable) {
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
    auto action = std::make_shared<TestAction>(0, tickable);

    EXPECT_FALSE(action->IsRunning());
    EXPECT_EQ(action->GetType(), 0u);
    EXPECT_NE(action->GetTickable(), nullptr);
}

TEST(ActionTest, StartStop) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(0, tickable);

    EXPECT_TRUE(action->Start());
    EXPECT_TRUE(action->IsRunning());

    EXPECT_TRUE(action->Stop());
    EXPECT_FALSE(action->IsRunning());
}

TEST(ActionTest, StartWhenAlreadyStarted) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(0, tickable);
    action->Start();
    EXPECT_TRUE(action->Start()); // idempotent
    EXPECT_TRUE(action->IsRunning());
}

TEST(ActionTest, StopWhenAlreadyStopped) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(0, tickable);
    EXPECT_TRUE(action->Stop()); // idempotent
    EXPECT_FALSE(action->IsRunning());
}

TEST(ActionTest, RunWhenRunning) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(0, tickable);
    action->Start();

    EXPECT_TRUE(action->Run(0.016));
    EXPECT_EQ(action->mRunCount, 1);
    EXPECT_DOUBLE_EQ(action->mLastDuration, 0.016);
}

TEST(ActionTest, RunWhenNotRunning) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(0, tickable);

    EXPECT_FALSE(action->Run(0.016));
    EXPECT_EQ(action->mRunCount, 0);
}

TEST(ActionTest, GetType) {
    auto tickable = std::make_shared<TestTickable>();
    auto action1 = std::make_shared<TestAction>(100, tickable);
    auto action2 = std::make_shared<TestAction>(200, tickable);

    EXPECT_EQ(action1->GetType(), 100u);
    EXPECT_EQ(action2->GetType(), 200u);
}

// ---- Weak reference tests (Action -> Tickable) ----

TEST(ActionTest, TickableWeakReference) {
    std::shared_ptr<TestAction> action;
    {
        auto tickable = std::make_shared<TestTickable>();
        action = std::make_shared<TestAction>(0, tickable);
        EXPECT_NE(action->GetTickable(), nullptr);
    }
    // After tickable is destroyed, GetTickable() should return nullptr.
    EXPECT_EQ(action->GetTickable(), nullptr);
}

TEST(ActionTest, UniqueIds) {
    auto tickable = std::make_shared<TestTickable>();
    auto a1 = std::make_shared<TestAction>(0, tickable);
    auto a2 = std::make_shared<TestAction>(0, tickable);
    EXPECT_NE(a1->GetId(), a2->GetId());
}

// ---- ActionList tests ----

#include "ship/ActionList.h"

TEST(ActionListTest, AddAndGetByType) {
    ActionList list;
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(static_cast<uint32_t>(ActionType::Tick), tickable);
    list.Add(action);

    auto tickActions = list.Get(static_cast<uint32_t>(ActionType::Tick));
    EXPECT_EQ(tickActions->size(), 1u);

    auto drawActions = list.Get(static_cast<uint32_t>(ActionType::Draw));
    EXPECT_EQ(drawActions->size(), 0u);
}

TEST(ActionListTest, GetByMultipleTypes) {
    ActionList list;
    auto tickable = std::make_shared<TestTickable>();
    list.Add(std::make_shared<TestAction>(static_cast<uint32_t>(ActionType::Tick), tickable));
    list.Add(std::make_shared<TestAction>(static_cast<uint32_t>(ActionType::Draw), tickable));
    list.Add(std::make_shared<TestAction>(static_cast<uint32_t>(ActionType::DrawDebugMenu), tickable));

    auto result = list.Get(std::vector<uint32_t>{ static_cast<uint32_t>(ActionType::Tick),
                                                  static_cast<uint32_t>(ActionType::DrawDebugMenu) });
    EXPECT_EQ(result->size(), 2u);
}
