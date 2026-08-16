#include <gtest/gtest.h>
#include "ship/core/Action.h"
#include "ship/core/Tickable.h"
#include "ship/actions/EventAction.h"
#include "ship/events/EventTypes.h"

using namespace Ship;

// Test EventIDs used throughout this file
static constexpr EventID kTickEvent = 1;
static constexpr EventID kDrawEvent = 2;
static constexpr EventID kDrawDebugMenuEvent = 3;

// A concrete Action for testing (without EventID).
class TestAction : public Action {
  public:
    TestAction(std::shared_ptr<Tickable> tickable)
        : Action(tickable), mRunCount(0) {
    }

    int mRunCount;

  protected:
    bool ActionRan() override {
        mRunCount++;
        return true;
    }
};

// A concrete EventAction for testing.
class TestEventAction : public EventAction {
  public:
    TestEventAction(EventID eventId, std::shared_ptr<Tickable> tickable)
        : EventAction(eventId, tickable), mRunCount(0) {
    }

    int mRunCount;

  protected:
    bool ActionRan() override {
        mRunCount++;
        return true;
    }
};

// A simple Tickable for testing.
class TestTickable : public Tickable {
};

// ---- Action lifecycle tests ----

TEST(ActionTest, InitialState) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(tickable);

    EXPECT_FALSE(action->IsRunning());
    EXPECT_NE(action->GetTickable(), nullptr);
}

TEST(ActionTest, StartStop) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(tickable);

    EXPECT_TRUE(action->Start());
    EXPECT_TRUE(action->IsRunning());

    EXPECT_TRUE(action->Stop());
    EXPECT_FALSE(action->IsRunning());
}

TEST(ActionTest, StartWhenAlreadyStarted) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(tickable);
    action->Start();
    EXPECT_TRUE(action->Start()); // idempotent
    EXPECT_TRUE(action->IsRunning());
}

TEST(ActionTest, StopWhenAlreadyStopped) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(tickable);
    EXPECT_TRUE(action->Stop()); // idempotent
    EXPECT_FALSE(action->IsRunning());
}

TEST(ActionTest, RunWhenRunning) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(tickable);
    action->Start();

    EXPECT_TRUE(action->Run());
    EXPECT_EQ(action->mRunCount, 1);
}

TEST(ActionTest, RunWhenNotRunning) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<TestAction>(tickable);

    EXPECT_FALSE(action->Run());
    EXPECT_EQ(action->mRunCount, 0);
}

TEST(ActionTest, GetEventId) {
    auto tickable = std::make_shared<TestTickable>();
    auto action1 = std::make_shared<TestEventAction>(kTickEvent, tickable);
    auto action2 = std::make_shared<TestEventAction>(kDrawEvent, tickable);

    EXPECT_EQ(action1->GetEventId(), kTickEvent);
    EXPECT_EQ(action2->GetEventId(), kDrawEvent);
}

TEST(ActionTest, DifferentEventIdsAreDifferent) {
    auto tickable = std::make_shared<TestTickable>();
    auto action1 = std::make_shared<TestEventAction>(kTickEvent, tickable);
    auto action2 = std::make_shared<TestEventAction>(kDrawEvent, tickable);
    EXPECT_NE(action1->GetEventId(), action2->GetEventId());
}

// ---- Weak reference tests (Action -> Tickable) ----

TEST(ActionTest, TickableWeakReference) {
    std::shared_ptr<TestAction> action;
    {
        auto tickable = std::make_shared<TestTickable>();
        action = std::make_shared<TestAction>(tickable);

        EXPECT_NE(action->GetTickable(), nullptr);
        // tickable goes out of scope here
    }

    // The Action still holds a weak reference, and since the Tickable is gone,
    // GetTickable() should return nullptr.
    EXPECT_EQ(action->GetTickable(), nullptr);
}

// ---- Permission hook tests ----

class RestrictiveAction : public Action {
  public:
    RestrictiveAction(std::shared_ptr<Tickable> tickable)
        : Action(tickable), mCanStart(true), mCanStop(true) {
    }

    bool mCanStart;
    bool mCanStop;

  protected:
    bool ActionRan() override {
        return true;
    }

    bool CanStart() override {
        return mCanStart;
    }

    bool CanStop() override {
        return mCanStop;
    }
};

TEST(ActionTest, CanStartPreventStart) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<RestrictiveAction>(tickable);
    action->mCanStart = false;

    EXPECT_FALSE(action->Start());
    EXPECT_FALSE(action->IsRunning());
}

TEST(ActionTest, CanStopPreventStop) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<RestrictiveAction>(tickable);
    action->mCanStop = false;

    action->Start();
    EXPECT_FALSE(action->Stop());
    EXPECT_TRUE(action->IsRunning());
}

TEST(ActionTest, ForceStartBypassesCanStart) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<RestrictiveAction>(tickable);
    action->mCanStart = false;

    EXPECT_TRUE(action->Start(true)); // Force start
    EXPECT_TRUE(action->IsRunning());
}

TEST(ActionTest, ForceStopBypassesCanStop) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<RestrictiveAction>(tickable);
    action->Start();
    action->mCanStop = false;

    EXPECT_TRUE(action->Stop(true)); // Force stop
    EXPECT_FALSE(action->IsRunning());
}

// ---- Notification hook tests ----

class InstrumentedAction : public Action {
  public:
    InstrumentedAction(std::shared_ptr<Tickable> tickable)
        : Action(tickable), mStartedCalls(0), mStoppedCalls(0), mLastStartedForced(false),
          mLastStoppedForced(false) {
    }

    int mStartedCalls;
    int mStoppedCalls;
    bool mLastStartedForced;
    bool mLastStoppedForced;

  protected:
    bool ActionRan() override {
        return true;
    }

    void Started(const bool forced) override {
        mStartedCalls++;
        mLastStartedForced = forced;
    }

    void Stopped(const bool forced) override {
        mStoppedCalls++;
        mLastStoppedForced = forced;
    }
};

TEST(ActionTest, StartedNotificationCalled) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<InstrumentedAction>(tickable);

    action->Start();
    EXPECT_EQ(action->mStartedCalls, 1);
    EXPECT_FALSE(action->mLastStartedForced);
}

TEST(ActionTest, StartedNotificationForced) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<RestrictiveAction>(tickable);
    // Create instrumented action for forced start
    auto instr = std::make_shared<InstrumentedAction>(tickable);
    instr->Start(true); // Force start

    EXPECT_EQ(instr->mStartedCalls, 1);
    EXPECT_TRUE(instr->mLastStartedForced);
}

TEST(ActionTest, StoppedNotificationCalled) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<InstrumentedAction>(tickable);

    action->Start();
    action->Stop();
    EXPECT_EQ(action->mStoppedCalls, 1);
    EXPECT_FALSE(action->mLastStoppedForced);
}

TEST(ActionTest, StoppedNotificationForced) {
    auto tickable = std::make_shared<TestTickable>();
    auto action = std::make_shared<InstrumentedAction>(tickable);

    action->Start();
    action->Stop(true); // Force stop

    EXPECT_EQ(action->mStoppedCalls, 1);
    EXPECT_TRUE(action->mLastStoppedForced);
}
