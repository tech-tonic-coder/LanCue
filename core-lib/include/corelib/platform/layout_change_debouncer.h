#pragma once

#include <functional>

#include <QString>
#include <QTimer>

namespace lancue::platform {

// Collapses a burst of raw layout-change notifications into a single
// onSettled()-style callback carrying only the final layout id — Phase
// 2's requirement that some OSes fire more than one notification per
// actual user switch. Deliberately not a QObject: it owns the QTimer as a
// plain member and connects to it using the timer itself as the
// lambda-connect context, so it needs no Q_OBJECT/AUTOMOC processing of
// its own (see the Phase 1 Learnings entry on the AUTOMOC/LNK2001 gotcha
// this sidesteps entirely).
//
// The debounce timer is single-shot and only ever running while a burst
// is in flight — idle the rest of the time — which is what makes this an
// acceptable exception to §4.7's "no short-interval repeating timers"
// rule rather than a violation of it.
class LayoutChangeDebouncer {
public:
    using SettledCallback = std::function<void(const QString& layoutId)>;

    explicit LayoutChangeDebouncer(int debounceMs = kDefaultDebounceMs);

    void setSettledCallback(SettledCallback callback);

    // Feeds one raw notification in. Restarts the debounce window each
    // time; once the window elapses without a further call, the last id
    // fed in reaches the callback — but only if it differs from the last
    // id that was already settled on, so an OS re-confirming the same
    // layout doesn't re-fire a spurious LayoutChanged event.
    void onRawChange(const QString& layoutId);

private:
    static constexpr int kDefaultDebounceMs = 120;

    QTimer m_timer;
    SettledCallback m_callback;
    QString m_pendingLayoutId;
    QString m_lastSettledLayoutId;
};

} // namespace lancue::platform
