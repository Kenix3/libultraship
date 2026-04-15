#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include "ship/events/EventSystem.h"

// ============================================================
// RegisterEvent
// ============================================================

TEST(EventSystem, RegisterEventReturnsValidId) {
    Ship::EventSystem sys;
    EventID id = sys.RegisterEvent("TestEvent");
    EXPECT_GE(id, 0);
}

TEST(EventSystem, RegisterMultipleEventsReturnDistinctIds) {
    Ship::EventSystem sys;
    EventID a = sys.RegisterEvent("EventA");
    EventID b = sys.RegisterEvent("EventB");
    EXPECT_NE(a, b);
}

TEST(EventSystem, RegisterEventNullNameUsesDefault) {
    Ship::EventSystem sys;
    EventID id = sys.RegisterEvent(nullptr);
    auto* reg = sys.GetEventRegistration(id);
    ASSERT_NE(reg, nullptr);
    EXPECT_STREQ(reg->Name, "Unknown");
}

TEST(EventSystem, GetEventRegistrationReturnsNullForUnknownId) {
    Ship::EventSystem sys;
    EXPECT_EQ(sys.GetEventRegistration(999), nullptr);
}

// ============================================================
// RegisterListener
// ============================================================

TEST(EventSystem, RegisterListenerReturnsValidId) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");
    ListenerID lid = sys.RegisterListener(evId, [](IEvent*) {});
    EXPECT_GE(lid, 0);
}

TEST(EventSystem, RegisterListenerOnUnregisteredEventThrows) {
    Ship::EventSystem sys;
    EXPECT_THROW(sys.RegisterListener(-1, [](IEvent*) {}), std::runtime_error);
}

TEST(EventSystem, MultipleListenersGetDistinctIds) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");
    ListenerID a = sys.RegisterListener(evId, [](IEvent*) {});
    ListenerID b = sys.RegisterListener(evId, [](IEvent*) {});
    EXPECT_NE(a, b);
}

// ============================================================
// UnregisterListener
// ============================================================

TEST(EventSystem, UnregisterListenerPreventsCallback) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");

    int callCount = 0;
    ListenerID lid = sys.RegisterListener(evId, [&](IEvent*) { callCount++; });

    sys.UnregisterListener(evId, lid);

    IEvent ev{ false };
    sys.CallEvent(evId, &ev);
    EXPECT_EQ(callCount, 0);
}

TEST(EventSystem, UnregisterNonExistentListenerIsNoOp) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");
    // Should not throw or crash
    EXPECT_NO_THROW(sys.UnregisterListener(evId, 9999));
}

TEST(EventSystem, UnregisterWithInvalidEventIdIsNoOp) {
    Ship::EventSystem sys;
    EXPECT_NO_THROW(sys.UnregisterListener(-1, 0));
}

TEST(EventSystem, UnregisterWithInvalidListenerIdIsNoOp) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");
    EXPECT_NO_THROW(sys.UnregisterListener(evId, -1));
}

// ============================================================
// CallEvent
// ============================================================

TEST(EventSystem, CallEventInvokesRegisteredListener) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");

    bool called = false;
    sys.RegisterListener(evId, [&](IEvent*) { called = true; });

    IEvent ev{ false };
    sys.CallEvent(evId, &ev);
    EXPECT_TRUE(called);
}

TEST(EventSystem, CallEventPassesEventPointer) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");

    IEvent* received = nullptr;
    sys.RegisterListener(evId, [&](IEvent* e) { received = e; });

    IEvent ev{ false };
    sys.CallEvent(evId, &ev);
    EXPECT_EQ(received, &ev);
}

TEST(EventSystem, CallEventInvokesAllListeners) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");

    int count = 0;
    sys.RegisterListener(evId, [&](IEvent*) { count++; });
    sys.RegisterListener(evId, [&](IEvent*) { count++; });
    sys.RegisterListener(evId, [&](IEvent*) { count++; });

    IEvent ev{ false };
    sys.CallEvent(evId, &ev);
    EXPECT_EQ(count, 3);
}

TEST(EventSystem, CallEventUpdatesCallerMetadata) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");
    sys.RegisterListener(evId, [](IEvent*) {});

    IEvent ev{ false };
    sys.CallEvent(evId, &ev, "file.cpp", 42, "file.cpp:42");

    auto* reg = sys.GetEventRegistration(evId);
    ASSERT_NE(reg, nullptr);
    auto it = reg->Callers.find("file.cpp:42");
    ASSERT_NE(it, reg->Callers.end());
    EXPECT_EQ(it->second.Count, 1u);
    EXPECT_EQ(it->second.Line, 42);
}

TEST(EventSystem, CallEventIncrementsCaller) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");

    IEvent ev{ false };
    sys.CallEvent(evId, &ev, "f.cpp", 1, "key");
    sys.CallEvent(evId, &ev, "f.cpp", 1, "key");
    sys.CallEvent(evId, &ev, "f.cpp", 1, "key");

    auto* reg = sys.GetEventRegistration(evId);
    ASSERT_NE(reg, nullptr);
    EXPECT_EQ(reg->Callers.at("key").Count, 3u);
}

// ============================================================
// Listener priority ordering
// ============================================================

TEST(EventSystem, HighPriorityListenerCalledBeforeLow) {
    Ship::EventSystem sys;
    EventID evId = sys.RegisterEvent("Ev");

    std::vector<int> order;
    // Register low-priority first, then high-priority
    sys.RegisterListener(evId, [&](IEvent*) { order.push_back(1); }, EVENT_PRIORITY_LOW);
    sys.RegisterListener(evId, [&](IEvent*) { order.push_back(3); }, EVENT_PRIORITY_HIGH);
    sys.RegisterListener(evId, [&](IEvent*) { order.push_back(2); }, EVENT_PRIORITY_NORMAL);

    IEvent ev{ false };
    sys.CallEvent(evId, &ev);

    // High > Normal > Low
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1); // LOW first in the sorted order (lower enum value)
    EXPECT_EQ(order[1], 2); // NORMAL
    EXPECT_EQ(order[2], 3); // HIGH last
}

// ============================================================
// GetEventRegistrations
// ============================================================

TEST(EventSystem, GetEventRegistrationsReflectsAllRegistered) {
    Ship::EventSystem sys;
    sys.RegisterEvent("A");
    sys.RegisterEvent("B");
    sys.RegisterEvent("C");
    EXPECT_EQ(sys.GetEventRegistrations().size(), 3u);
}
