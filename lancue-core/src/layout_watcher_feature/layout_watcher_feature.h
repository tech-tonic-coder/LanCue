#pragma once

#include <memory>

#include "corelib/platform/IKeyboardLayoutWatcher.h"
#include "corelib/platform/layout_change_debouncer.h"
#include "feature.h"

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

namespace lancue {

class EventBus;

// Phase 2's feature: owns the platform's IKeyboardLayoutWatcher, feeds its
// raw notifications through a LayoutChangeDebouncer, and publishes exactly
// one EventType::LayoutChanged event per settled user-initiated switch.
// Produces events only — it doesn't need onEvent(), which is why that
// override is empty rather than doing anything.
//
// Phase 7 revision (2026-08-30): the hook itself now runs on its own
// dedicated QThread rather than the main thread — see start()'s own
// comment for why. This is purely an internal implementation detail:
// every consumer of EventType::LayoutChanged (ConversionFeature,
// ToastFeature) is completely unaffected and still receives it on the
// main thread exactly as before.
class LayoutWatcherFeature final : public IFeature {
public:
    explicit LayoutWatcherFeature(EventBus& eventBus);

    bool start() override;
    void stop() override;
    void onEvent(const Event& event) override;

private:
    EventBus& m_eventBus;
    std::unique_ptr<platform::IKeyboardLayoutWatcher> m_watcher;
    platform::LayoutChangeDebouncer m_debouncer;
    QThread* m_hookThread = nullptr;
};

} // namespace lancue

