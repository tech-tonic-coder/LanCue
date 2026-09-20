#include "global_hotkey_feature.h"

#include "corelib/eventbus/event.h"
#include "corelib/eventbus/event_bus.h"
#include "corelib/logging/logger.h"

namespace lancue {

const platform::HotkeyId& GlobalHotkeyFeature::kConvertAutoHotkeyId = platform::kConvertAutoHotkeyId;

GlobalHotkeyFeature::GlobalHotkeyFeature(EventBus& eventBus, settings::SettingsManager& settingsManager)
    : m_eventBus(eventBus), m_settingsManager(settingsManager) {}

bool GlobalHotkeyFeature::start() {
    m_manager = platform::createGlobalHotkeyManager();

    m_manager->setCallback([this](const platform::HotkeyId& id) {
        logInfo(QStringLiteral("GlobalHotkeyFeature: hotkey '%1' pressed.").arg(id));
        Event event;
        event.type = EventType::HotkeyPressed;
        event.data = id;
        m_eventBus.publish(event);
    });

    m_eventBus.subscribe(EventType::SettingsChanged, [this](const Event& event) {
        if (event.data.toString() == QString::fromUtf8(settings::kFieldHotkeys)) {
            reconcileWithSettings();
        }
    });

    reconcileWithSettings();

    // A genuinely empty configured hotkey map (every default removed) is
    // a valid registration outcome, not a failure — start() only fails
    // loudly here if the manager itself couldn't be created, which
    // reconcileWithSettings()'s own registerHotkey() calls already handle
    // per-id via HotkeyRegistrationFailed events (§5 Phase 3's "fail
    // loudly" requirement, unchanged by this phase).
    return m_manager != nullptr;
}

void GlobalHotkeyFeature::stop() {
    if (m_manager) {
        m_manager->stop();
    }
    m_registeredIds.clear();
}

void GlobalHotkeyFeature::onEvent(const Event& /*event*/) {
    // This feature only produces HotkeyPressed/HotkeyRegistrationFailed
    // events; it reacts to SettingsChanged via its own EventBus
    // subscription in start() instead of this override (see the class
    // comment on IFeature::onEvent() — subscribing directly is the
    // documented alternative path).
}

bool GlobalHotkeyFeature::registerHotkey(const platform::HotkeyId& id, const platform::HotkeyCombo& combo) {
    if (!m_manager->registerHotkey(id, combo)) {
        // The platform manager already logged the specific OS-level
        // reason (e.g. RegisterHotKey's error code on Windows) — this is
        // this phase's "fail loudly" requirement (§5 Phase 3): a
        // HotkeyRegistrationFailed event so a future settings UI can
        // surface it to the user, not just a log line only a developer
        // would ever see.
        logError(QStringLiteral("GlobalHotkeyFeature: failed to register hotkey '%1' for id='%2'.")
                     .arg(combo.toString(), id));
        Event event;
        event.type = EventType::HotkeyRegistrationFailed;
        event.data = QStringLiteral("%1: %2").arg(id, combo.toString());
        m_eventBus.publish(event);
        return false;
    }

    logInfo(QStringLiteral("GlobalHotkeyFeature: registered hotkey '%1' for id='%2'.").arg(combo.toString(), id));
    m_registeredIds.insert(id);
    return true;
}

void GlobalHotkeyFeature::unregisterHotkey(const platform::HotkeyId& id) {
    if (m_manager) {
        m_manager->unregisterHotkey(id);
    }
    m_registeredIds.remove(id);
}

void GlobalHotkeyFeature::reconcileWithSettings() {
    const auto& configured = m_settingsManager.current().hotkeys;

    // A genuinely empty configured hotkey map is a legitimate state — the
    // user removed every hotkey via RemoveHotkey — and is respected here,
    // not silently overridden. (An earlier version of this method treated
    // "configured is empty" as "settings must not have loaded yet" and
    // re-registered kConvertTextHotkeyId's default combo automatically;
    // caught via lancue-debug-cli manual testing on 2026-08-07 — see the
    // Phase 4 Learnings entry — since defaultSettings() always seeds that
    // id, the only real way to reach an empty map is deliberate removal,
    // which this silently undid without even reflecting the re-added
    // hotkey back into SettingsManager. Removed rather than special-cased
    // further.)
    if (configured.isEmpty()) {
        logInfo(QStringLiteral("GlobalHotkeyFeature: no hotkeys configured."));
    }

    for (auto it = configured.constBegin(); it != configured.constEnd(); ++it) {
        registerHotkey(it.key(), it.value().toCombo());
    }

    // Snapshot before mutating: unregisterHotkey() below removes from
    // m_registeredIds, so iterating that same container live while
    // modifying it would be undefined behavior.
    const QSet<platform::HotkeyId> configuredIds(configured.keyBegin(), configured.keyEnd());
    const QSet<platform::HotkeyId> previouslyRegistered = m_registeredIds;
    for (const auto& id : previouslyRegistered) {
        if (!configuredIds.contains(id)) {
            unregisterHotkey(id);
        }
    }
}

} // namespace lancue
