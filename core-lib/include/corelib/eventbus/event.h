#pragma once

#include <QMetaType>
#include <QVariant>

namespace lancue {

// The set of events any daemon feature might care about. New event types
// are added here as later phases introduce real producers (Phase 2's
// layout watcher, Phase 3/8's mouse hook, Phase 4's SettingsManager) — see
// EventBus for how subscribers consume these without polling.
enum class EventType {
    SettingsChanged,
    LayoutChanged,
    MouseMoved,
    // Phase 3: fired once per settled global-hotkey press. Payload is the
    // HotkeyId (QString) of whichever registered hotkey fired — LanCue
    // is expected to grow more than one independently-triggerable action
    // (confirmed 2026-08-05, see the Phase 3 Learnings entry), so the
    // type alone is no longer enough to say which one.
    HotkeyPressed,
    // Phase 3: fired when IGlobalHotkeyManager::registerHotkey() fails
    // (most likely another app already owns the combination). Payload is
    // a QString of the form "id: combo" (e.g. "convertText: Ctrl+Alt+L")
    // identifying both which action's hotkey failed and what combo was
    // attempted — needed now that more than one hotkey can be registered
    // independently (confirmed 2026-08-05). This is this phase's "fail
    // loudly" mechanism (§5 Phase 3 requirements) until Phase 9's
    // settings UI exists to subscribe to it and show a real error to the
    // user — see the Phase 3 Learnings entry for why an event was chosen
    // over building UI that doesn't exist yet.
    HotkeyRegistrationFailed,
};

// Generic envelope for anything broadcast on the EventBus. `data` carries
// the event-specific payload; each producer/consumer pair agrees on its
// shape out of band (e.g. LayoutChanged will carry the normalized layout
// id string Phase 2 defines). QVariant keeps EventBus itself free of a
// per-event-type struct hierarchy, since new event types are expected —
// see roadmap §4.3 on designing for the axes of change that are actually
// known (feature set is one of them).
struct Event {
    EventType type = EventType::SettingsChanged;
    QVariant data;
};

} // namespace lancue

Q_DECLARE_METATYPE(lancue::Event)
