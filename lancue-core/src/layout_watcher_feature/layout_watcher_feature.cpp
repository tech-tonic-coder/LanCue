#include "layout_watcher_feature.h"

#include <QCoreApplication>
#include <QSemaphore>
#include <QThread>

#include "corelib/eventbus/event.h"
#include "corelib/eventbus/event_bus.h"
#include "corelib/logging/logger.h"

namespace lancue {

namespace {

// Runs the platform IKeyboardLayoutWatcher's hook/notification mechanism
// on its own dedicated thread, with its own message loop, instead of the
// main thread — see LayoutWatcherFeature::start()'s own comment for why.
// QThread::run() is overridden directly (no moveToThread() dance) since
// IKeyboardLayoutWatcher isn't a QObject; there's nothing to move, only a
// thread to run its start()/exec()/stop() sequence on. No new
// signals/slots are declared here, so no Q_OBJECT/AUTOMOC processing is
// needed for this class beyond what QThread already provides.
class LayoutHookThread : public QThread {
public:
    LayoutHookThread(platform::IKeyboardLayoutWatcher& watcher, QSemaphore& readySemaphore, bool& outStartSucceeded)
        : m_watcher(watcher), m_readySemaphore(readySemaphore), m_outStartSucceeded(outStartSucceeded) {}

protected:
    void run() override {
        m_outStartSucceeded = m_watcher.start();
        // Unblocks LayoutWatcherFeature::start(), which is waiting
        // synchronously on this semaphore — see that method's own
        // comment for why a semaphore rather than a signal/slot
        // round-trip is used for this one specific hand-off.
        m_readySemaphore.release();

        if (!m_outStartSucceeded) {
            return;
        }

        // Required for the hook to actually be pumped at all — per
        // Microsoft's own documentation for WH_KEYBOARD_LL, "the thread
        // that installed the hook must have a message loop" — and for
        // stop()'s own quit()/wait() below to have something to quit.
        exec();

        // Unhooking on the same thread that installed the hook, after
        // exec() has actually returned (i.e. after quit() was called),
        // mirrors the "tied to the installing thread" model this whole
        // mechanism is built on.
        m_watcher.stop();
    }

private:
    platform::IKeyboardLayoutWatcher& m_watcher;
    QSemaphore& m_readySemaphore;
    bool& m_outStartSucceeded;
};

} // namespace

LayoutWatcherFeature::LayoutWatcherFeature(EventBus& eventBus) : m_eventBus(eventBus) {}

bool LayoutWatcherFeature::start() {
    m_watcher = platform::createKeyboardLayoutWatcher();

    m_debouncer.setSettledCallback([this](const QString& layoutId) {
        logInfo(QStringLiteral("LayoutWatcherFeature: layout changed to '%1'.").arg(layoutId));
        Event event;
        event.type = EventType::LayoutChanged;
        event.data = layoutId;
        m_eventBus.publish(event);
    });

    // The hook callback itself now fires on m_hookThread (below), never
    // on this (main) thread — per Microsoft's own documentation,
    // WH_KEYBOARD_LL's callback runs *as part of* the installing thread's
    // own message retrieval (GetMessage/PeekMessage), nested inside
    // whatever that thread happens to be doing at the time it fires.
    // Installing on a small, dedicated thread whose only job is pumping
    // that hook keeps it from ever being nested inside the *main*
    // thread's own event-dispatcher call — which is what caused a
    // confirmed, reproducible delayed/stale toast repaint on real
    // hardware when EventType::LayoutChanged was previously published
    // directly from inside that nested call (see the Phase 7 Learnings
    // entry for the two same-thread deferral attempts, both insufficient,
    // that led here — Mahdi's own question, 2026-08-30, on whether a
    // separate thread would fix this properly is what prompted this
    // revision, replacing an earlier, less correct workaround that
    // resorted to polling instead).
    //
    // The callback below marshals back onto the *main* thread via
    // QMetaObject::invokeMethod(qApp, ..., Qt::QueuedConnection) at the
    // single point where the hook thread hands off to the rest of this
    // feature's logic. This hand-off point specifically — before
    // touching m_debouncer at all — matters: m_debouncer's own QTimer
    // (LayoutChangeDebouncer, see that class's own comment) has
    // main-thread affinity (constructed here, in this feature's
    // constructor, which always runs on the main thread), and Qt timers
    // can only be started/stopped from their own thread — so
    // onRawChange() itself must run on the main thread, not the hook
    // thread. Nothing past this one hand-off point needs to know a
    // second thread exists at all: m_debouncer, EventBus::publish(), and
    // every subscriber (including ToastFeature's real GUI work) still run
    // entirely on the main thread, unchanged — this fixes the underlying
    // timing bug for every consumer of this event at its true root,
    // rather than requiring each consumer to work around it individually
    // (or, worse, resorting to polling and losing this project's own
    // event-driven-by-default philosophy, §4.7).
    m_watcher->setCallback([this](const QString& rawLayoutId) {
        QMetaObject::invokeMethod(
            qApp, [this, rawLayoutId]() { m_debouncer.onRawChange(rawLayoutId); }, Qt::QueuedConnection);
    });

    QSemaphore hookReady(0);
    bool hookStartSucceeded = false;
    m_hookThread = new LayoutHookThread(*m_watcher, hookReady, hookStartSucceeded);
    m_hookThread->start();
    // Blocks only until the hook thread has attempted SetWindowsHookExW
    // (a single Win32 call) — sub-millisecond in practice, and the
    // simplest way to keep this method's own bool-return-on-failure
    // contract (IFeature::start()) working across the thread boundary,
    // for what's fundamentally a one-shot startup check rather than
    // something worth a more elaborate async-failure-reporting path.
    hookReady.acquire();

    if (!hookStartSucceeded) {
        logError(QStringLiteral(
            "LayoutWatcherFeature: platform watcher failed to start; layout changes will not be detected."));
        m_hookThread->quit();
        m_hookThread->wait();
        return false;
    }

    const QString initialLayout = m_watcher->currentLayout();
    logInfo(QStringLiteral("LayoutWatcherFeature: started, current layout is '%1'.").arg(initialLayout));

    // Phase 6 needs a known "current layout" the moment the daemon is
    // ready, not only after the user's first real switch (its generic
    // auto-detect hotkey has nothing to look up otherwise) — this is
    // exactly the one-time startup seed IKeyboardLayoutWatcher::
    // currentLayout()'s own doc comment describes, just published on the
    // bus instead of only logged, so any feature can learn it without
    // owning a second watcher instance. currentLayout() itself is a
    // stateless, thread-agnostic query (a plain OS call, no dependency on
    // the hook/thread above) — safe to call here, on the main thread,
    // regardless of which thread now owns the hook.
    Event initialEvent;
    initialEvent.type = EventType::LayoutChanged;
    initialEvent.data = initialLayout;
    m_eventBus.publish(initialEvent);

    return true;
}

void LayoutWatcherFeature::stop() {
    if (m_hookThread) {
        // quit() posts a request for the hook thread to leave exec();
        // that thread then calls m_watcher->stop() (UnhookWindowsHookEx)
        // itself, on the same thread that installed the hook, before its
        // run() returns — wait() blocks until that has actually finished.
        m_hookThread->quit();
        m_hookThread->wait();
        delete m_hookThread;
        m_hookThread = nullptr;
    }
}

void LayoutWatcherFeature::onEvent(const Event& /*event*/) {
    // This feature only produces LayoutChanged events; it doesn't need to
    // react to anything else on the bus.
}

} // namespace lancue
