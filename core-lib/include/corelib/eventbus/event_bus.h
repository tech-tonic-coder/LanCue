#pragma once

#include <functional>

#include <QHash>
#include <QList>
#include <QObject>

#include "corelib/eventbus/event.h"

namespace lancue {

// In-process publish/subscribe bus used by lancue-core to fan a single
// OS-level event (layout change, mouse move, settings change) out to
// whichever IFeature modules care about it, without those modules polling
// or knowing about each other (roadmap §2.2). Producers call publish()
// from inside their own OS-callback handler (e.g. Phase 2's
// WM_INPUTLANGCHANGE hook) — the bus itself never polls anything, it only
// reacts to publish() calls, so it adds no idle CPU cost of its own (§4.7).
//
// QObject-based (not a template) so a subscriber can also connect with
// Qt's own signal/slot syntax via eventPublished() if that ends up more
// convenient than the Handler callback form for a given feature; both
// paths reach the same publish() call.
class EventBus : public QObject {
    Q_OBJECT

public:
    using Handler = std::function<void(const Event&)>;

    explicit EventBus(QObject* parent = nullptr);

    // Returns a subscription id for use with unsubscribe(). No Phase 1
    // consumer needs to unsubscribe mid-run, but the id is returned now so
    // a future IFeature::stop() can unsubscribe cleanly without this
    // signature changing later.
    int subscribe(EventType type, Handler handler);
    void unsubscribe(EventType type, int subscriptionId);

    void publish(const Event& event);

signals:
    // Mirrors publish() as a Qt signal for consumers that prefer connect()
    // over the Handler callback form.
    void eventPublished(const Event& event);

private:
    struct Subscription {
        int id;
        Handler handler;
    };

    QHash<EventType, QList<Subscription>> m_subscribers;
    int m_nextSubscriptionId = 1;
};

} // namespace lancue
