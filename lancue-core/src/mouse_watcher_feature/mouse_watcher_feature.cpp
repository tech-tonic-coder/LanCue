#include "mouse_watcher_feature.h"

#include <QCursor>

#include "corelib/eventbus/event.h"
#include "corelib/eventbus/event_bus.h"
#include "corelib/logging/logger.h"

namespace lancue {

MouseWatcherFeature::MouseWatcherFeature(EventBus& eventBus) : m_eventBus(eventBus) {}

MouseWatcherFeature::~MouseWatcherFeature() = default;

bool MouseWatcherFeature::start() {
    m_hook = platform::createMouseMoveHook();

    m_hook->setCallback([this]() {
        Event event;
        event.type = EventType::MouseMoved;
        // Resolved once, here, from the one place in this codebase that
        // already reads cursor position for position-dependent behavior
        // (ToastFeature::resolveTargetScreens()'s own CursorScreen case)
        // — every platform hook implementation deliberately carries no
        // position of its own (see IMouseMoveHook.h), so QCursor::pos()
        // is the one authoritative source, already in the same
        // per-monitor DIP coordinate space every QScreen/move() call in
        // this codebase already assumes. Published once here rather than
        // re-queried by every subscriber.
        event.data = QCursor::pos();
        m_eventBus.publish(event);
    });

    if (!m_hook->start()) {
        logError(QStringLiteral("MouseWatcherFeature: platform mouse hook failed to start; the follower window "
                                 "will not track the cursor."));
        return false;
    }

    logInfo(QStringLiteral("MouseWatcherFeature: started."));
    return true;
}

void MouseWatcherFeature::stop() {
    if (m_hook) {
        m_hook->stop();
    }
}

void MouseWatcherFeature::onEvent(const Event& /*event*/) {
    // Produces MouseMoved only — see IFeature::onEvent()'s own comment on
    // why a producer-only feature leaves this empty.
}

} // namespace lancue
