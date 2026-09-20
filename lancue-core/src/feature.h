#pragma once

#include "corelib/eventbus/event.h"

namespace lancue {

// Every lancue-core daemon feature (toast, follower, update-checker, and
// now the layout watcher — §3's project structure) implements this, so
// main.cpp only ever constructs a list of features and hands them to a
// FeatureRegistry — it never branches on what a feature actually does.
// Removing a feature is deleting its registration line and its folder;
// adding one is the reverse (§4.4).
class IFeature {
public:
    virtual ~IFeature() = default;

    // Called once at startup, after the EventBus exists. Features do
    // their own OS-hook/watcher setup here rather than in a constructor,
    // so a failed start() — e.g. a platform interface's own start()
    // returning false — can be logged and skipped without leaving a
    // half-constructed object in the registry.
    virtual bool start() = 0;

    virtual void stop() = 0;

    // Features that only produce events onto the EventBus (like Phase
    // 2's layout watcher) have nothing to do here and just leave the
    // override empty; features that consume events subscribe to the
    // EventBus directly in start() instead of routing through this —
    // onEvent() exists for a feature's own internal use if it needs it,
    // not as the only way to receive events.
    virtual void onEvent(const Event& event) = 0;
};

} // namespace lancue
