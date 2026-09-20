#pragma once

#include <memory>

#include <QString>

#include "corelib/settings/settings_manager.h"
#include "feature.h"

namespace lancue {

class EventBus;

namespace ui {
class FollowerWindow;
}

// Owns the single FollowerWindow instance and decides when it shows
// (LayoutChanged, same trigger as ToastFeature) and where it moves
// (MouseMoved, while visible) — mirrors ToastFeature's own
// EventBus-subscription shape closely (§4.3/§4.4: reuse an established
// pattern), but owns exactly one window rather than one-per-screen,
// since the follower is a single indicator that crosses monitors with
// the cursor rather than something shown per-screen (see the roadmap's
// own Phase 8 wording).
class FollowerFeature final : public IFeature {
public:
    FollowerFeature(EventBus& eventBus, settings::SettingsManager& settingsManager);
    ~FollowerFeature() override;

    bool start() override;
    void stop() override;
    void onEvent(const Event& event) override;

private:
    EventBus& m_eventBus;
    settings::SettingsManager& m_settingsManager;
    std::unique_ptr<ui::FollowerWindow> m_followerWindow;

    // Same startup-seed guard as ToastFeature::m_hasReceivedFirstLayoutEvent
    // — the very first LayoutChanged is LayoutWatcherFeature's own
    // startup seed, not a real switch, and must not show the follower on
    // launch.
    bool m_hasReceivedFirstLayoutEvent = false;
    QString m_currentLayout;
};

} // namespace lancue
