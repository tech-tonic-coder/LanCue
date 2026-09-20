#pragma once

#include <memory>

#include <QSet>

#include "corelib/platform/IGlobalHotkeyManager.h"
#include "corelib/settings/settings_manager.h"
#include "feature.h"

namespace lancue {

class EventBus;

// Owns the platform's IGlobalHotkeyManager and publishes exactly one
// EventType::HotkeyPressed event (payload: the HotkeyId that fired) per
// press, for any number of independently-registered hotkeys.
//
// Phase 4 wires this to settings::SettingsManager, per the Phase 3
// Learnings entry that flagged this as the next phase's job: start()
// registers whatever's in settings().current().hotkeys instead of just
// the one hardcoded default, and the feature subscribes to
// EventType::SettingsChanged (field "hotkeys") to reconcile its own
// registrations live whenever settings change — settings is "what's
// configured", this feature is "what's actually claimed with the OS"
// (§2.2), and this is the sync between the two. A settings hotkey map
// with everything removed is respected as zero registered hotkeys, not
// silently overridden back to a default (see reconcileWithSettings()'s
// own comment for why an earlier version of this method did the
// opposite).
class GlobalHotkeyFeature final : public IFeature {
public:
    // The generic auto-detect conversion hotkey's id. An alias for
    // platform::kConvertAutoHotkeyId (moved to core-lib in Phase 4 so
    // settings::defaultSettings() can reach it too — see that constant's
    // own comment; renamed from kConvertTextHotkeyId in Phase 6 when the
    // single placeholder hotkey became the real multi-hotkey scheme)
    // rather than its own static member. Later phases adding their own
    // actions should define their own similarly-named id constant near
    // wherever that action lives, not extend this one.
    static const platform::HotkeyId& kConvertAutoHotkeyId;

    GlobalHotkeyFeature(EventBus& eventBus, settings::SettingsManager& settingsManager);

    bool start() override;
    void stop() override;
    void onEvent(const Event& event) override;

    // Registers (or, if `id` is already registered, replaces) the combo
    // for a named hotkey without restarting the daemon or the feature.
    // Called both from start()/the SettingsChanged reconciliation above
    // and directly by tests exercising the underlying mechanism.
    bool registerHotkey(const platform::HotkeyId& id, const platform::HotkeyCombo& combo);

    // Releases a previously-registered hotkey. Safe to call for an id
    // that was never registered or whose last registration failed.
    void unregisterHotkey(const platform::HotkeyId& id);

private:
    // Registers/updates/removes hotkeys so the manager's live
    // registrations match settings().current().hotkeys exactly.
    void reconcileWithSettings();

    EventBus& m_eventBus;
    settings::SettingsManager& m_settingsManager;
    std::unique_ptr<platform::IGlobalHotkeyManager> m_manager;
    QSet<platform::HotkeyId> m_registeredIds;
};

} // namespace lancue
