#include <gtest/gtest.h>
#include "ship/core/Tickable.h"
#include "ship/core/Action.h"
#include "ship/actions/EventAction.h"
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

struct RawCallbackState {
    int runCount = 0;
    EventID lastEventId = static_cast<EventID>(-1);
    double lastDuration = 0.0;
};

static bool RawEventActionCallback(EventID eventId, const double durationSinceLastTick, uintptr_t userData) {
    auto* state = reinterpret_cast<RawCallbackState*>(userData);
    if (state == nullptr) {
        return false;
    }

    state->runCount++;
    state->lastEventId = eventId;
    state->lastDuration = durationSinceLastTick;
    return true;
}

// A concrete EventAction for testing Tickable interaction.
class CountingEventAction : public EventAction {
  public:
    CountingEventAction(EventID eventId, std::shared_ptr<Tickable> tickable)
        : EventAction(eventId, tickable), mRunCount(0) {
    }
    int mRunCount;

  protected:
    bool ActionRan(const double durationSinceLastTick) override {
        mRunCount++;
        return EventAction::ActionRan(durationSinceLastTick);
    }
};

// Derived action type for template filtering.
class SpecialAction : public CountingEventAction {
  public:
    SpecialAction(EventID eventId, std::shared_ptr<Tickable> tickable)
        : CountingEventAction(eventId, tickable) {
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
        auto a = std::make_shared<CountingEventAction>(eventIds[i % 5], t);
        t->GetActionList().Add(a);
    }
    return t;
}

// ---- Tickable lifecycle tests ----

TEST(TickableTest, DefaultIsTicking) {
    auto t = std::make_shared<TestTickableObj>();
    EXPECT_TRUE(t->IsTicking());
}

TEST(TickableTest, StartStop) {
    auto t = std::make_shared<TestTickableObj>(false);
    EXPECT_FALSE(t->IsTicking());
    EXPECT_TRUE(t->Start());
    EXPECT_TRUE(t->IsTicking());
    EXPECT_TRUE(t->Stop());
    EXPECT_FALSE(t->IsTicking());
}

// ---- Action list tests ----

TEST(TickableTest, AddRemoveActions) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingEventAction>(kTickEvent, t);

    EXPECT_EQ(t->GetActionList().Add(a), ListReturnCode::Success);
    EXPECT_TRUE(t->GetActionList().Has(a));
    EXPECT_EQ(t->GetActionList().Remove(a), ListReturnCode::Success);
    EXPECT_FALSE(t->GetActionList().Has(a));
}

TEST(TickableTest, ActionListOrderedByEventID) {
    auto t = std::make_shared<TestTickableObj>();
    std::vector<std::shared_ptr<CountingEventAction>> actions;
    std::vector<EventID> eventIds = { kEvent2, kEvent0, kEvent4, kEvent1, kEvent3 };

    for (EventID id : eventIds) {
        auto a = std::make_shared<CountingEventAction>(id, t);
        t->GetActionList().Add(a);
        actions.push_back(a);
    }

    // Actions should be sorted by EventID internally.
    auto list = t->GetActionList().Get();
    EXPECT_EQ(list->size(), 5);
    for (size_t i = 1; i < list->size(); i++) {
        auto* ea = dynamic_cast<EventAction*>((*list)[i].get());
        auto* eb = dynamic_cast<EventAction*>((*list)[i - 1].get());
        if (ea && eb) { EXPECT_TRUE(ea->GetEventId() >= eb->GetEventId()); }
    }
}

// ---- Run tests ----

TEST(TickableTest, RunSingleEventIDExplicitly) {
    auto t = MakeTickableWithActions(3);
    double dt = 0.016;
    t->Tick(dt, kEvent0);

    auto list = t->GetActionList().Get();
    for (const auto& action : *list) {
        auto* ca = dynamic_cast<CountingEventAction*>(action.get());
        auto* ea = dynamic_cast<EventAction*>(action.get());
        if (ca && ea && ea->GetEventId() == kEvent0) {
            EXPECT_EQ(ca->mRunCount, 1);
        } else if (ca && ea) {
            EXPECT_EQ(ca->mRunCount, 0);
        }
    }
}

TEST(TickableTest, RunSpecificEventID) {
    auto t = MakeTickableWithActions(5);
    double dt = 0.016;
    t->Tick(dt, kEvent2);

    auto list = t->GetActionList().Get();
    for (const auto& action : *list) {
        auto* ca = dynamic_cast<CountingEventAction*>(action.get());
        auto* ea = dynamic_cast<EventAction*>(action.get());
        if (ca && ea && ea->GetEventId() == kEvent2) {
            EXPECT_EQ(ca->mRunCount, 1);
        } else if (ca && ea) {
            EXPECT_EQ(ca->mRunCount, 0);
        }
    }
}

TEST(TickableTest, RunMultipleEventIDs) {
    auto t = MakeTickableWithActions(5);
    double dt = 0.016;
    std::vector<EventID> targetIds = { kEvent1, kEvent3 };
    t->Tick(dt, targetIds);

    auto list = t->GetActionList().Get();
    for (const auto& action : *list) {
        auto* ca = dynamic_cast<CountingEventAction*>(action.get());
        auto* ea = dynamic_cast<EventAction*>(action.get());
        if (ca && ea) {
            bool isTarget = std::find(targetIds.begin(), targetIds.end(), ea->GetEventId()) != targetIds.end();
            EXPECT_EQ(ca->mRunCount, isTarget ? 1 : 0);
        }
    }
}

TEST(TickableTest, RunFilterByTypeAndEventIDs) {
    auto t = std::make_shared<TestTickableObj>();
    auto count = std::make_shared<CountingEventAction>(kTickEvent, t);
    auto special = std::make_shared<SpecialAction>(kDrawEvent, t);

    t->GetActionList().Add(count);
    t->GetActionList().Add(special);

    double dt = 0.016;
    t->Tick<SpecialAction>(dt, std::vector<EventID>{ kTickEvent, kDrawEvent });

    EXPECT_EQ(count->mRunCount, 0);
    EXPECT_EQ(special->mRunCount, 1);
}

TEST(TickableTest, RunFilterByTypeAndEventID) {
    auto t = std::make_shared<TestTickableObj>();
    auto count1 = std::make_shared<CountingEventAction>(kTickEvent, t);
    auto special1 = std::make_shared<SpecialAction>(kTickEvent, t);
    auto special2 = std::make_shared<SpecialAction>(kDrawEvent, t);

    t->GetActionList().Add(count1);
    t->GetActionList().Add(special1);
    t->GetActionList().Add(special2);

    double dt = 0.016;
    t->Tick<SpecialAction>(dt, kTickEvent);

    EXPECT_EQ(count1->mRunCount, 0);
    EXPECT_EQ(special1->mRunCount, 1);
    EXPECT_EQ(special2->mRunCount, 0);
}

TEST(TickableTest, RunWhenNotTicking) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto action = std::make_shared<CountingEventAction>(kTickEvent, t);
    t->GetActionList().Add(action);

    t->Tick(0.016, kTickEvent);

    EXPECT_EQ(action->mRunCount, 0);
}

TEST(TickableTest, EventActionCppCallbackDispatchesDirectly) {
    auto t = std::make_shared<TestTickableObj>();
    int runCount = 0;
    EventID lastEventId = static_cast<EventID>(-1);
    double lastDuration = 0.0;

    auto action = std::make_shared<EventAction>(
        kTickEvent, t, [&](EventID eventId, const double durationSinceLastTick, uintptr_t callbackPointerData) {
            runCount++;
            lastEventId = eventId;
            lastDuration = durationSinceLastTick;
            return callbackPointerData == 0;
        });

    t->GetActionList().Add(action);
    t->Tick(0.016, kTickEvent);

    EXPECT_EQ(runCount, 1);
    EXPECT_EQ(lastEventId, kTickEvent);
    EXPECT_DOUBLE_EQ(lastDuration, 0.016);
}

TEST(TickableTest, EventActionRawCallbackDispatchesDirectly) {
    auto t = std::make_shared<TestTickableObj>();
    RawCallbackState state{};

    auto action = std::make_shared<EventAction>(
        kDrawEvent, t, reinterpret_cast<uintptr_t>(&RawEventActionCallback), reinterpret_cast<uintptr_t>(&state));

    t->GetActionList().Add(action);
    t->Tick(0.033, kDrawEvent);

    EXPECT_EQ(state.runCount, 1);
    EXPECT_EQ(state.lastEventId, kDrawEvent);
    EXPECT_DOUBLE_EQ(state.lastDuration, 0.033);
}

TEST(TickableTest, EventActionCallbackGettersAndSetters) {
    auto t = std::make_shared<TestTickableObj>();
    auto action = std::make_shared<EventAction>(kEvent0, t);

    EXPECT_FALSE(action->HasCallback());
    EXPECT_FALSE(action->GetHasCppCallback());
    EXPECT_FALSE(action->GetHasRawCallback());
    EXPECT_TRUE(std::holds_alternative<std::monostate>(action->GetCallback()));
    EXPECT_EQ(action->GetCallbackPointerData(), static_cast<uintptr_t>(0));

    action->SetCallback(EventActionCppCallback([](EventID, const double, uintptr_t callbackPointerData) {
        return callbackPointerData == static_cast<uintptr_t>(123);
    }), static_cast<uintptr_t>(123));
    EXPECT_TRUE(action->HasCallback());
    EXPECT_TRUE(action->GetHasCppCallback());
    EXPECT_FALSE(action->GetHasRawCallback());
    EXPECT_TRUE(std::holds_alternative<EventActionCppCallback>(action->GetCallback()));
    EXPECT_EQ(action->GetCallbackPointerData(), static_cast<uintptr_t>(123));

    RawCallbackState state{};
    EventActionCallback callbackVariant = reinterpret_cast<uintptr_t>(&RawEventActionCallback);
    action->SetCallback(callbackVariant, reinterpret_cast<uintptr_t>(&state));
    EXPECT_TRUE(action->HasCallback());
    EXPECT_FALSE(action->GetHasCppCallback());
    EXPECT_TRUE(action->GetHasRawCallback());
    EXPECT_TRUE(std::holds_alternative<uintptr_t>(action->GetCallback()));
    EXPECT_EQ(std::get<uintptr_t>(action->GetCallback()), reinterpret_cast<uintptr_t>(&RawEventActionCallback));
    EXPECT_EQ(action->GetCallbackPointerData(), reinterpret_cast<uintptr_t>(&state));

    action->SetCallbackPointerData(static_cast<uintptr_t>(456));
    EXPECT_EQ(action->GetCallbackPointerData(), static_cast<uintptr_t>(456));

    action->ClearCallback();
    EXPECT_FALSE(action->HasCallback());
    EXPECT_FALSE(action->GetHasCppCallback());
    EXPECT_FALSE(action->GetHasRawCallback());
    EXPECT_TRUE(std::holds_alternative<std::monostate>(action->GetCallback()));
    EXPECT_EQ(action->GetCallbackPointerData(), static_cast<uintptr_t>(0));
}





