#pragma once

#include <memory>
#include <vector>

#include <QString>

#include "corelib/settings/settings_manager.h"
#include "feature.h"

QT_BEGIN_NAMESPACE
class QScreen;
QT_END_NAMESPACE

namespace lancue {

class EventBus;

namespace ui {
class ToastWindow;
}

// Owns however many ToastWindow instances the current toastMonitorMode
// needs, and decides when they show — reacting to LayoutChanged (auto
// toast, only if toastEnabled) and the show/dismiss hotkeys (always
// available, regardless of that setting).
class ToastFeature final : public IFeature {
public:
    ToastFeature(EventBus& eventBus, settings::SettingsManager& settingsManager);
    ~ToastFeature() override;

    bool start() override;
    void stop() override;
    void onEvent(const Event& event) override;

private:
    // Shared by both the automatic and manual triggers: resolves target
    // screen(s) and current settings, then shows one window per screen.
    void showForCurrentLayout();

    // toastMonitorMode decides which screen(s) get a toast: the one
    // under the cursor, the primary one, one specific screen (falls
    // back to primary if that index no longer exists), or all of them.
    // Never empty as long as at least one screen exists.
    std::vector<QScreen*> resolveTargetScreens() const;

    // Grows/shrinks m_toastWindows to `count` entries — the only place
    // window count changes, so everything else just iterates the list.
    void ensureWindowCount(size_t count);

    EventBus& m_eventBus;
    settings::SettingsManager& m_settingsManager;
    std::vector<std::unique_ptr<ui::ToastWindow>> m_toastWindows;

    // The very first LayoutChanged is just a startup seed, not a real
    // switch — must not trigger a toast on launch.
    bool m_hasReceivedFirstLayoutEvent = false;
    QString m_currentLayout;
};

} // namespace lancue
