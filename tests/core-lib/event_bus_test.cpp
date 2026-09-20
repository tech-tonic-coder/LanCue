#include <catch2/catch_test_macros.hpp>

#include "corelib/eventbus/event_bus.h"

using lancue::Event;
using lancue::EventBus;
using lancue::EventType;

TEST_CASE("EventBus delivers a published event to subscribers of that type", "[eventbus]") {
    EventBus bus;
    bool called = false;
    int received = 0;

    bus.subscribe(EventType::SettingsChanged, [&](const Event& e) {
        called = true;
        received = e.data.toInt();
    });

    Event event;
    event.type = EventType::SettingsChanged;
    event.data = 3000;
    bus.publish(event);

    CHECK(called);
    CHECK(received == 3000);
}

TEST_CASE("EventBus does not deliver to subscribers of a different type", "[eventbus]") {
    EventBus bus;
    bool called = false;

    bus.subscribe(EventType::LayoutChanged, [&](const Event&) { called = true; });

    Event event;
    event.type = EventType::SettingsChanged;
    bus.publish(event);

    CHECK_FALSE(called);
}

TEST_CASE("unsubscribe stops further delivery", "[eventbus]") {
    EventBus bus;
    int count = 0;
    const int id = bus.subscribe(EventType::MouseMoved, [&](const Event&) { ++count; });

    Event event;
    event.type = EventType::MouseMoved;
    bus.publish(event);
    bus.unsubscribe(EventType::MouseMoved, id);
    bus.publish(event);

    CHECK(count == 1);
}
