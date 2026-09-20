#include "corelib/eventbus/event_bus.h"

namespace lancue {

EventBus::EventBus(QObject* parent) : QObject(parent) {}

int EventBus::subscribe(EventType type, Handler handler) {
    const int id = m_nextSubscriptionId++;
    m_subscribers[type].append(Subscription{id, std::move(handler)});
    return id;
}

void EventBus::unsubscribe(EventType type, int subscriptionId) {
    auto it = m_subscribers.find(type);
    if (it == m_subscribers.end()) {
        return;
    }
    auto& list = it.value();
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].id == subscriptionId) {
            list.removeAt(i);
            break;
        }
    }
}

void EventBus::publish(const Event& event) {
    emit eventPublished(event);

    const auto it = m_subscribers.constFind(event.type);
    if (it == m_subscribers.constEnd()) {
        return;
    }
    // Copy the subscriber list before invoking: a handler that subscribes
    // or unsubscribes in reaction to this event must not corrupt the
    // iteration we're in the middle of.
    const auto handlers = it.value();
    for (const auto& sub : handlers) {
        sub.handler(event);
    }
}

} // namespace lancue
