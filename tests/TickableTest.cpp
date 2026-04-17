#include <gtest/gtest.h>
#include "ship/Tickable.h"
#include "ship/Action.h"

using namespace Ship;

// A concrete Action for testing Tickable interaction.
class CountingAction : public Action {
  public:
    CountingAction(uint32_t actionType, std::shared_ptr<Tickable> tickable)
        : Action(actionType, tickable), mRunCount(0) {
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
    SpecialAction(uint32_t actionType, std::shared_ptr<Tickable> tickable) : CountingAction(actionType, tickable) {
    }
};

// A simple Tickable subclass.
class TestTickableObj : public Tickable {
  public:
    TestTickableObj(bool isTicking = true) : Tickable(isTicking) {
    }
};

// Helper: create a Tickable and add some actions.
static std::shared_ptr<TestTickableObj> MakeTickableWithActions(int numActions, uint32_t baseType = 0) {
    auto t = std::make_shared<TestTickableObj>(true);
    for (int i = 0; i < numActions; i++) {
        auto a = std::make_shared<CountingAction>(baseType + static_cast<uint32_t>(i), t);
        t->AddAction(a);
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

// ---- Action management tests ----

TEST(TickableTest, AddAction) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(0, t);

    EXPECT_TRUE(t->AddAction(a));
    EXPECT_TRUE(t->HasAction(a));
    EXPECT_EQ(t->CountActions(), 1u);
}

TEST(TickableTest, AddActionNull) {
    auto t = std::make_shared<TestTickableObj>();
    EXPECT_FALSE(t->AddAction(nullptr));
}

TEST(TickableTest, AddActionDuplicate) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(0, t);
    t->AddAction(a);
    EXPECT_TRUE(t->AddAction(a)); // Duplicate returns true (already there)
    EXPECT_EQ(t->CountActions(), 1u);
}

TEST(TickableTest, RemoveAction) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(0, t);
    t->AddAction(a);
    EXPECT_TRUE(t->RemoveAction(a));
    EXPECT_FALSE(t->HasAction(a));
    EXPECT_EQ(t->CountActions(), 0u);
}

TEST(TickableTest, RemoveActionNull) {
    auto t = std::make_shared<TestTickableObj>();
    EXPECT_FALSE(t->RemoveAction(nullptr));
}

TEST(TickableTest, RemoveActionNotFound) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(0, t);
    // Not added, so removing it should succeed (it's already not there)
    EXPECT_TRUE(t->RemoveAction(a));
}

TEST(TickableTest, ActionStartedOnAdd) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(0, t);
    t->AddAction(a);
    EXPECT_TRUE(a->IsRunning());
}

TEST(TickableTest, ActionStoppedOnRemove) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(0, t);
    t->AddAction(a);
    t->RemoveAction(a);
    EXPECT_FALSE(a->IsRunning());
}

// ---- Run tests ----

TEST(TickableTest, TickRunsAllActions) {
    auto t = MakeTickableWithActions(3);
    t->Run(0.016);

    auto actions = t->GetActions();
    for (const auto& a : *actions) {
        auto ca = std::dynamic_pointer_cast<CountingAction>(a);
        ASSERT_NE(ca, nullptr);
        EXPECT_EQ(ca->mRunCount, 1);
    }
}

TEST(TickableTest, TickDoesNothingWhenNotTicking) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto a = std::make_shared<CountingAction>(0, t);
    t->AddAction(a);
    t->Run(0.016);
    EXPECT_EQ(a->mRunCount, 0);
}

TEST(TickableTest, TickByActionType) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto tick = std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::Tick), t);
    auto draw = std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::Draw), t);
    t->AddAction(tick);
    t->AddAction(draw);

    t->Run(0.016, static_cast<uint32_t>(ActionType::Tick));
    EXPECT_EQ(tick->mRunCount, 1);
    EXPECT_EQ(draw->mRunCount, 0);
}

TEST(TickableTest, TickByMultipleActionTypes) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto tick = std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::Tick), t);
    auto draw = std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::Draw), t);
    auto debug = std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::DrawDebugMenu), t);
    t->AddAction(tick);
    t->AddAction(draw);
    t->AddAction(debug);

    t->Run(0.016,
            std::vector<uint32_t>{ static_cast<uint32_t>(ActionType::Tick),
                                   static_cast<uint32_t>(ActionType::DrawDebugMenu) });
    EXPECT_EQ(tick->mRunCount, 1);
    EXPECT_EQ(draw->mRunCount, 0);
    EXPECT_EQ(debug->mRunCount, 1);
}

TEST(TickableTest, TickByActionTypeDoesNothingWhenStopped) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto a = std::make_shared<CountingAction>(0, t);
    t->AddAction(a);
    t->Run(0.016, 0u);
    EXPECT_EQ(a->mRunCount, 0);
}

TEST(TickableTest, TickByMultipleTypesDoesNothingWhenStopped) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto a = std::make_shared<CountingAction>(0, t);
    t->AddAction(a);
    t->Run(0.016, std::vector<uint32_t>{ 0u });
    EXPECT_EQ(a->mRunCount, 0);
}

// ---- Template Run tests ----

TEST(TickableTest, TickByTemplateType) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto regular = std::make_shared<CountingAction>(0, t);
    auto special = std::make_shared<SpecialAction>(0, t);
    t->AddAction(regular);
    t->AddAction(special);

    t->Run<SpecialAction>(0.016);
    EXPECT_EQ(regular->mRunCount, 0);
    EXPECT_EQ(special->mRunCount, 1);
}

TEST(TickableTest, TickByTemplateTypeDoesNothingWhenStopped) {
    auto t = std::make_shared<TestTickableObj>(false);
    auto special = std::make_shared<SpecialAction>(0, t);
    t->AddAction(special);
    t->Run<SpecialAction>(0.016);
    EXPECT_EQ(special->mRunCount, 0);
}

TEST(TickableTest, TickByTemplateTypeAndSingleActionType) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto regular = std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::Tick), t);
    auto special = std::make_shared<SpecialAction>(static_cast<uint32_t>(ActionType::Tick), t);
    auto specialDraw = std::make_shared<SpecialAction>(static_cast<uint32_t>(ActionType::Draw), t);
    t->AddAction(regular);
    t->AddAction(special);
    t->AddAction(specialDraw);

    t->Run<SpecialAction>(0.016, static_cast<uint32_t>(ActionType::Tick));
    EXPECT_EQ(regular->mRunCount, 0);
    EXPECT_EQ(special->mRunCount, 1);
    EXPECT_EQ(specialDraw->mRunCount, 0);
}

TEST(TickableTest, TickByTemplateTypeAndMultipleActionTypes) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto regular = std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::Tick), t);
    auto special1 = std::make_shared<SpecialAction>(static_cast<uint32_t>(ActionType::Tick), t);
    auto special2 = std::make_shared<SpecialAction>(static_cast<uint32_t>(ActionType::Draw), t);
    auto special3 = std::make_shared<SpecialAction>(static_cast<uint32_t>(ActionType::DrawDebugMenu), t);
    t->AddAction(regular);
    t->AddAction(special1);
    t->AddAction(special2);
    t->AddAction(special3);

    t->Run<SpecialAction>(0.016,
                           std::vector<uint32_t>{ static_cast<uint32_t>(ActionType::Tick),
                                                  static_cast<uint32_t>(ActionType::DrawDebugMenu) });
    EXPECT_EQ(regular->mRunCount, 0);
    EXPECT_EQ(special1->mRunCount, 1);
    EXPECT_EQ(special2->mRunCount, 0);
    EXPECT_EQ(special3->mRunCount, 1);
}

// ---- GetActions tests ----

TEST(TickableTest, GetAllActions) {
    auto t = MakeTickableWithActions(3);
    auto actions = t->GetActions();
    EXPECT_EQ(actions->size(), 3u);
}

TEST(TickableTest, GetActionsByType) {
    auto t = std::make_shared<TestTickableObj>(true);
    t->AddAction(std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::Tick), t));
    t->AddAction(std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::Draw), t));
    t->AddAction(std::make_shared<CountingAction>(static_cast<uint32_t>(ActionType::Tick), t));

    auto tickActions = t->GetActions(static_cast<uint32_t>(ActionType::Tick));
    EXPECT_EQ(tickActions->size(), 2u);
}

TEST(TickableTest, GetActionsByTemplateType) {
    auto t = std::make_shared<TestTickableObj>(true);
    t->AddAction(std::make_shared<CountingAction>(0, t));
    t->AddAction(std::make_shared<SpecialAction>(0, t));
    t->AddAction(std::make_shared<CountingAction>(0, t));

    auto specialActions = t->GetActions<SpecialAction>();
    EXPECT_EQ(specialActions->size(), 1u);
}

// ---- Multiple ticks test ----

TEST(TickableTest, MultipleTicks) {
    auto t = std::make_shared<TestTickableObj>(true);
    auto a = std::make_shared<CountingAction>(0, t);
    t->AddAction(a);

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
    auto a = std::make_shared<CountingAction>(0, t);
    EXPECT_TRUE(t->AddAction(a, true));
    EXPECT_TRUE(t->HasAction(a));
}

TEST(TickableTest, ForceRemoveAction) {
    auto t = std::make_shared<TestTickableObj>();
    auto a = std::make_shared<CountingAction>(0, t);
    t->AddAction(a);
    EXPECT_TRUE(t->RemoveAction(a, true));
    EXPECT_FALSE(t->HasAction(a));
}

// ---- Construction with actions test ----

TEST(TickableTest, ConstructWithActions) {
    auto t = std::make_shared<TestTickableObj>();
    auto a1 = std::make_shared<CountingAction>(0, t);
    auto a2 = std::make_shared<CountingAction>(1, t);

    auto t2 = std::make_shared<TestTickableObj>(true);
    // Manually add since we can't use constructor with actions easily due to shared_from_this
    t2->AddAction(a1);
    t2->AddAction(a2);
    EXPECT_EQ(t2->CountActions(), 2u);
}
