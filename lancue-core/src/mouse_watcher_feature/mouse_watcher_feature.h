#pragma once

#include <memory>

#include "corelib/platform/IMouseMoveHook.h"
#include "feature.h"

namespace lancue {

class EventBus;

// Owns the OS-level mouse-move hook (Phase 8) and republishes every real
// cursor movement as EventType::MouseMoved on the EventBus — the same
// producer/consumer split LayoutWatcherFeature/ToastFeature already
// established for keyboard-layout changes (§4.3: reuse an existing
// pattern rather than inventing a new one), so any future feature that
// also cares about cursor movement (not just FollowerFeature) can
// subscribe without needing its own OS hook. EventType::MouseMoved was
// already anticipated for exactly this back in Phase 1's own event.h
// comment.
//
// Unlike LayoutWatcherFeature, this needs no debouncer and no manual
// main-thread hand-off of its own: IMouseMoveHook's own contract already
// guarantees the registered callback runs on the thread that called
// start() (see that header's comment), so setCallback()'s lambda below
// can call QCursor::pos()/EventBus::publish() directly.
class MouseWatcherFeature final : public IFeature {
public:
    explicit MouseWatcherFeature(EventBus& eventBus);
    ~MouseWatcherFeature() override;

    bool start() override;
    void stop() override;
    void onEvent(const Event& event) override;

private:
    EventBus& m_eventBus;
    std::unique_ptr<platform::IMouseMoveHook> m_hook;
};

} // namespace lancue
