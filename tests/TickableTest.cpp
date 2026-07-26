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

    EXPECT_TRUE(t->GetActionList().Add(a));
    EXPECT_TRUE(t->GetActionList().Has(a));
    EXPECT_TRUE(t->GetActionList().Remove(a));
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

TEST(TickableTest, RunAllActions) {
    auto t = MakeTickableWithActions(3);
    double dt = 0.016;
    t->Run(dt);

    auto list = t->GetActionList().Get();
    for (const auto& action : *list) {
        auto* ca = dynamic_cast<CountingEventAction*>(action.get());
        if (ca) {
            EXPECT_EQ(ca->mRunCount, 1);
        }
    }
}

TEST(TickableTest, RunSpecificEventID) {
    auto t = MakeTickableWithActions(5);
    double dt = 0.016;
    t->Run(dt, kEvent2);

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
    t->Run(dt, targetIds);

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

TEST(TickableTest, RunFilterByType) {
    auto t = std::make_shared<TestTickableObj>();
    auto count = std::make_shared<CountingEventAction>(kTickEvent, t);
    auto special = std::make_shared<SpecialAction>(kDrawEvent, t);

    t->GetActionList().Add(count);
    t->GetActionList().Add(special);

    double dt = 0.016;
    t->Run<SpecialAction>(dt);

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
    t->Run<SpecialAction>(dt, kTickEvent);

    EXPECT_EQ(count1->mRunCount, 0);
    EXPECT_EQ(special1->mRunCount, 1);
    EXPECT_EQ(special2->mRunCount, 0);
}

TEST(TickableTest, RunWhenNotTicking) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto action = std::make_shared<CountingEventAction>(kTickEvent, t);
    t->GetActionList().Add(action);

    t->Run(0.016);

    EXPECT_EQ(action->mRunCount, 0);
}





