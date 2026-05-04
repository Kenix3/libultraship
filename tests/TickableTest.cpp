#include <gtest/gtest.h>
#include "ship/Tickable.h"
#include "ship/Action.h"
#include "ship/events/EventTypes.h"

using namespace Ship;

static constexpr EventID kEvent0 = 0;
static constexpr EventID kEvent1 = 1;
static constexpr EventID kEvent2 = 2;
static constexpr EventID kEvent3 = 3;
static constexpr EventID kEvent4 = 4;
static constexpr EventID kTickEvent = 10;
static constexpr EventID kDrawEvent = 11;
static constexpr EventID kDrawDebugMenuEvent = 12;

// A concrete Action for testing Tickable interaction.
class CountingAction : public Action {
  public:
    CountingAction(EventID eventId, std::shared_ptr<Tickable> tickable)
        : Action(eventId, tickable), mRunCount(0) {
    }
    int mRunCount;

  protected:
    bool ActionRan(const double durationSinceLastTick) override {
        mRunCount++;
        return true;
    }
};

// Derived action type for template filtering.
class SpecialAction : public CountingAction {
  public:
    SpecialAction(EventID eventId, std::shared_ptr<Tickable> tickable)
        : CountingAction(eventId, tickable) {
    }
};

// A simple Tickable subclass.
class TestTickableObj : public Tickable {
  public:
    TestTickableObj(bool isTicking = true) : Tickable(isTicking) {
    }
};

// Helper: create a Tickable and add some actions.
static std::shared_ptr<TestTickableObj> MakeTickableWithActions(int numActions) {
    static const EventID eventIds[] = { kEvent0, kEvent1, kEvent2, kEvent3, kEvent4 };
    auto t = std::make_shared<TestTickableObj>(true);
    for (int i = 0; i < numActions; i++) {
        auto a = std::make_shared<CountingAction>(eventIds[i % 5], t);
        t->GetActionList().Add(a);
    }
    return t;
}

// ---- Tickable lifecycle tests ----

TEST(TickableTest, DefaultIsTicking) {
    auto t = std::make_shared<TestTickableObj>();
    EXPECT_TRUE(t->IsTicking());
}

TEST(TickableTest, ConstructNotTicking) {
    auto t = std::make_shared<TestTickableObj>(false);
    EXPECT_FALSE(t->IsTicking());
}

TEST(TickableTest, StartStop) {
    auto t = std::make_shared<TestTickableObj>(false);
    EXPECT_FALSE(t->IsTicking());
    EXPECT_TRUE(t->Start());
    EXPECT_TRUE(t->IsTicking());
    EXPECT_TRUE(t->Stop());
    EXPECT_FALSE(t->IsTicking());
}

TEST(TickableTest, StartIdempotent) {
    auto t = std::make_shared<TestTickableObj>(true);
    EXPECT_TRUE(t->Start());
    EXPECT_TRUE(t->IsTicking());
}

TEST(TickableTest, StopIdempotent) {
    auto t = std::make_shared<TestTickableObj>(false);
    EXPECT_TRUE(t->Stop());
    EXPECT_FALSE(t->IsTicking());
}

// ---- Action management via ActionList tests ----

TEST(TickableTest, AddAction) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(kTickEvent, t);

    EXPECT_EQ(t->GetActionList().Add(a), ListReturnCode::Success);
    EXPECT_TRUE(t->GetActionList().Has(a));
    EXPECT_EQ(t->GetActionList().GetCount(), 1u);
}

TEST(TickableTest, AddActionNull) {
    auto t = std::make_shared<TestTickableObj>();
    EXPECT_EQ(t->GetActionList().Add(nullptr), ListReturnCode::Failed);
}

TEST(TickableTest, AddActionDuplicate) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    t->GetActionList().Add(a);
    EXPECT_EQ(t->GetActionList().Add(a), ListReturnCode::Duplicate);
    EXPECT_EQ(t->GetActionList().GetCount(), 1u);
}

TEST(TickableTest, RemoveAction) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    t->GetActionList().Add(a);
    EXPECT_EQ(t->GetActionList().Remove(a), ListReturnCode::Success);
    EXPECT_FALSE(t->GetActionList().Has(a));
    EXPECT_EQ(t->GetActionList().GetCount(), 0u);
}

TEST(TickableTest, RemoveActionNull) {
    auto t = std::make_shared<TestTickableObj>();
    EXPECT_EQ(t->GetActionList().Remove(std::shared_ptr<Action>(nullptr)), ListReturnCode::Failed);
}

TEST(TickableTest, RemoveActionNotFound) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    EXPECT_EQ(t->GetActionList().Remove(a), ListReturnCode::NotFound);
}

TEST(TickableTest, ActionStartedOnAdd) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    t->GetActionList().Add(a);
    EXPECT_TRUE(a->IsRunning());
}

TEST(TickableTest, ActionStoppedOnRemove) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    t->GetActionList().Add(a);
    t->GetActionList().Remove(a);
    EXPECT_FALSE(a->IsRunning());
}

// ---- Run tests ----

TEST(TickableTest, RunRunsAllActions) {
    auto t = MakeTickableWithActions(3);
    t->Run(0.016);

    auto actions = t->GetActionList().Get();
    for (const auto& a : *actions) {
        auto ca = std::dynamic_pointer_cast<CountingAction>(a);
        ASSERT_NE(ca, nullptr);
        EXPECT_EQ(ca->mRunCount, 1);
    }
}

TEST(TickableTest, RunDoesNothingWhenNotTicking) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    t->GetActionList().Add(a);
    t->Run(0.016);
    EXPECT_EQ(a->mRunCount, 0);
}

TEST(TickableTest, RunByEventId) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto tick = std::make_shared<CountingAction>(kTickEvent, t);
    auto draw = std::make_shared<CountingAction>(kDrawEvent, t);
    t->GetActionList().Add(tick);
    t->GetActionList().Add(draw);

    t->Run(0.016, kTickEvent);
    EXPECT_EQ(tick->mRunCount, 1);
    EXPECT_EQ(draw->mRunCount, 0);
}

TEST(TickableTest, RunByMultipleEventIds) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto tick = std::make_shared<CountingAction>(kTickEvent, t);
    auto draw = std::make_shared<CountingAction>(kDrawEvent, t);
    auto debug = std::make_shared<CountingAction>(kDrawDebugMenuEvent, t);
    t->GetActionList().Add(tick);
    t->GetActionList().Add(draw);
    t->GetActionList().Add(debug);

    t->Run(0.016, std::vector<EventID>{ kTickEvent, kDrawDebugMenuEvent });
    EXPECT_EQ(tick->mRunCount, 1);
    EXPECT_EQ(draw->mRunCount, 0);
    EXPECT_EQ(debug->mRunCount, 1);
}

TEST(TickableTest, RunByEventIdDoesNothingWhenStopped) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    t->GetActionList().Add(a);
    t->Run(0.016, kTickEvent);
    EXPECT_EQ(a->mRunCount, 0);
}

TEST(TickableTest, RunByMultipleIdsDoesNothingWhenStopped) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    t->GetActionList().Add(a);
    t->Run(0.016, std::vector<EventID>{ kTickEvent });
    EXPECT_EQ(a->mRunCount, 0);
}

// ---- Template Run tests ----

TEST(TickableTest, RunByTemplateType) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto regular = std::make_shared<CountingAction>(kTickEvent, t);
    auto special = std::make_shared<SpecialAction>(kTickEvent, t);
    t->GetActionList().Add(regular);
    t->GetActionList().Add(special);

    t->Run<SpecialAction>(0.016);
    EXPECT_EQ(regular->mRunCount, 0);
    EXPECT_EQ(special->mRunCount, 1);
}

TEST(TickableTest, RunByTemplateTypeDoesNothingWhenStopped) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto special = std::make_shared<SpecialAction>(kTickEvent, t);
    t->GetActionList().Add(special);
    t->Run<SpecialAction>(0.016);
    EXPECT_EQ(special->mRunCount, 0);
}

TEST(TickableTest, RunByTemplateTypeAndSingleEventId) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto regular = std::make_shared<CountingAction>(kTickEvent, t);
    auto special = std::make_shared<SpecialAction>(kTickEvent, t);
    auto specialDraw = std::make_shared<SpecialAction>(kDrawEvent, t);
    t->GetActionList().Add(regular);
    t->GetActionList().Add(special);
    t->GetActionList().Add(specialDraw);

    t->Run<SpecialAction>(0.016, kTickEvent);
    EXPECT_EQ(regular->mRunCount, 0);
    EXPECT_EQ(special->mRunCount, 1);
    EXPECT_EQ(specialDraw->mRunCount, 0);
}

TEST(TickableTest, RunByTemplateTypeAndMultipleEventIds) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto regular = std::make_shared<CountingAction>(kTickEvent, t);
    auto special1 = std::make_shared<SpecialAction>(kTickEvent, t);
    auto special2 = std::make_shared<SpecialAction>(kDrawEvent, t);
    auto special3 = std::make_shared<SpecialAction>(kDrawDebugMenuEvent, t);
    t->GetActionList().Add(regular);
    t->GetActionList().Add(special1);
    t->GetActionList().Add(special2);
    t->GetActionList().Add(special3);

    t->Run<SpecialAction>(0.016, std::vector<EventID>{ kTickEvent, kDrawDebugMenuEvent });
    EXPECT_EQ(regular->mRunCount, 0);
    EXPECT_EQ(special1->mRunCount, 1);
    EXPECT_EQ(special2->mRunCount, 0);
    EXPECT_EQ(special3->mRunCount, 1);
}

// ---- GetActionList tests ----

TEST(TickableTest, GetAllActions) {
    auto t = MakeTickableWithActions(3);
    auto actions = t->GetActionList().Get();
    EXPECT_EQ(actions->size(), 3u);
}

TEST(TickableTest, GetActionsByEventId) {
    auto t = std::make_shared<TestTickableObj>(true);
    t->GetActionList().Add(std::make_shared<CountingAction>(kTickEvent, t));
    t->GetActionList().Add(std::make_shared<CountingAction>(kDrawEvent, t));
    t->GetActionList().Add(std::make_shared<CountingAction>(kTickEvent, t));

    auto tickActions = t->GetActionList().Get(kTickEvent);
    EXPECT_EQ(tickActions->size(), 2u);
}

// ---- Multiple runs test ----

TEST(TickableTest, MultipleRuns) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    t->GetActionList().Add(a);

    for (int i = 0; i < 10; i++) {
        t->Run(0.016);
    }
    EXPECT_EQ(a->mRunCount, 10);
}

// ---- Force start/stop tests ----

TEST(TickableTest, ForceStart) {
    auto t = std::make_shared<TestTickableObj>(false);
    EXPECT_TRUE(t->Start(true));
    EXPECT_TRUE(t->IsTicking());
}

TEST(TickableTest, ForceStop) {
    auto t = std::make_shared<TestTickableObj>(true);
    EXPECT_TRUE(t->Stop(true));
    EXPECT_FALSE(t->IsTicking());
}

TEST(TickableTest, ForceAddAction) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    auto result = t->GetActionList().Add(a, true);
    EXPECT_TRUE(static_cast<int32_t>(result) >= 0);
    EXPECT_TRUE(t->GetActionList().Has(a));
}

TEST(TickableTest, ForceRemoveAction) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(kTickEvent, t);
    t->GetActionList().Add(a);
    auto result = t->GetActionList().Remove(a, true);
    EXPECT_TRUE(static_cast<int32_t>(result) >= 0);
    EXPECT_FALSE(t->GetActionList().Has(a));
}

// ---- Construction with actions test ----

TEST(TickableTest, ConstructWithActions) {
    auto t = std::make_shared<TestTickableObj>();
    auto a1 = std::make_shared<CountingAction>(kTickEvent, t);
    auto a2 = std::make_shared<CountingAction>(kDrawEvent, t);

    auto t2 = std::make_shared<TestTickableObj>(true);
    t2->GetActionList().Add(a1);
    t2->GetActionList().Add(a2);
    EXPECT_EQ(t2->GetActionList().GetCount(), 2u);
}
