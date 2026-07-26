#include <gtest/gtest.h>
#include "ship/events/Events.h"
#include "ship/events/EventTypes.h"

using namespace Ship;

// ============================================================
// Events::RegisterEvent tests
// ============================================================

TEST(EventsTest, RegisterEventReturnsValidId) {
    Events events;
    EventID id = events.RegisterEvent("TestEvent");
    EXPECT_GE(id, 0);
}

TEST(EventsTest, RegisterMultipleEventsReturnDistinctIds) {
    Events events;
    EventID a = events.RegisterEvent("EventA");
    EventID b = events.RegisterEvent("EventB");
    EXPECT_NE(a, b);
}

TEST(EventsTest, RegisterEventNullNameUsesDefault) {
    Events events;
    EventID id = events.RegisterEvent(nullptr);
    auto* reg = events.GetEventRegistration(id);
    ASSERT_NE(reg, nullptr);
    EXPECT_STREQ(reg->Name, "Unknown");
}

TEST(EventsTest, RegisterEventStoresName) {
    Events events;
    EventID id = events.RegisterEvent("MyEvent");
    auto* reg = events.GetEventRegistration(id);
    ASSERT_NE(reg, nullptr);
    EXPECT_STREQ(reg->Name, "MyEvent");
}

// ============================================================
// Events::CallEvent -- Callers map uses std::string key
//
// Before the fix the map was keyed on const char* (pointer equality).
// Two call sites using the same string literal could produce different
// pointers in different translation units, silently creating duplicate
// entries. The map key is now std::string so equality is by value.
// ============================================================

// Two calls with the same key string must accumulate in one map entry.
TEST(EventsCallerKeyTest, SameStringProducesOneCallerEntry) {
    Events events;
    EventID id = events.RegisterEvent("TestEvent");

    IEvent ev{ false };
    events.CallEvent(id, &ev, "file.cpp", 10, "file.cpp:10");
    events.CallEvent(id, &ev, "file.cpp", 10, "file.cpp:10");

    auto* reg = events.GetEventRegistration(id);
    ASSERT_NE(reg, nullptr);
    EXPECT_EQ(reg->Callers.size(), 1u);
    EXPECT_EQ(reg->Callers.at(std::string("file.cpp:10")).Count, 2u);
}

// Two calls with different key strings must produce separate entries.
TEST(EventsCallerKeyTest, DifferentKeyStringsProduceSeparateEntries) {
    Events events;
    EventID id = events.RegisterEvent("TestEvent");

    IEvent ev{ false };
    events.CallEvent(id, &ev, "a.cpp", 1, "a.cpp:1");
    events.CallEvent(id, &ev, "b.cpp", 2, "b.cpp:2");

    auto* reg = events.GetEventRegistration(id);
    ASSERT_NE(reg, nullptr);
    EXPECT_EQ(reg->Callers.size(), 2u);
    EXPECT_EQ(reg->Callers.at(std::string("a.cpp:1")).Count, 1u);
    EXPECT_EQ(reg->Callers.at(std::string("b.cpp:2")).Count, 1u);
}

// Passing nullptr as key must be a silent no-op -- no entry created, no crash.
TEST(EventsCallerKeyTest, NullKeyDoesNotCrashAndCreatesNoEntry) {
    Events events;
    EventID id = events.RegisterEvent("TestEvent");
    IEvent ev{ false };
    EXPECT_NO_THROW(events.CallEvent(id, &ev, nullptr, 0, nullptr));
    auto* reg = events.GetEventRegistration(id);
    ASSERT_NE(reg, nullptr);
    EXPECT_TRUE(reg->Callers.empty());
}

// The count for a single key must accumulate correctly across many calls.
TEST(EventsCallerKeyTest, CountAccumulatesAcrossManyCalls) {
    Events events;
    EventID id = events.RegisterEvent("TestEvent");
    IEvent ev{ false };

    constexpr int kCalls = 100;
    for (int i = 0; i < kCalls; i++) {
        events.CallEvent(id, &ev, "f.cpp", 1, "key");
    }

    auto* reg = events.GetEventRegistration(id);
    ASSERT_NE(reg, nullptr);
    EXPECT_EQ(reg->Callers.at(std::string("key")).Count, static_cast<uint64_t>(kCalls));
}

// ============================================================
// Events::GetEventRegistrations
// ============================================================

TEST(EventsTest, GetEventRegistrationsReflectsAllRegistered) {
    Events events;
    events.RegisterEvent("A");
    events.RegisterEvent("B");
    events.RegisterEvent("C");
    EXPECT_EQ(events.GetEventRegistrations().size(), 3u);
}

TEST(EventsTest, GetEventRegistrationReturnsNullForUnknownId) {
    Events events;
    EXPECT_EQ(events.GetEventRegistration(-1), nullptr);
    EXPECT_EQ(events.GetEventRegistration(9999), nullptr);
}
