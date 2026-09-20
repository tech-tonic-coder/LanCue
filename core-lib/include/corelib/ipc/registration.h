#pragma once

#include "corelib/ipc/dispatcher.h"
#include "corelib/platform/selection_clipboard_bridge.h"
#include "corelib/settings/settings_manager.h"

namespace lancue::ipc {

// Registers every built-in message handler against `dispatcher`. This is
// the one call site lancue-core/main.cpp needs — adding a new message type
// means adding a new handler file under core-lib/src/ipc/handlers/ and one
// call inside registration.cpp, per §4.4 — main.cpp itself never grows.
//
// Phase 4 swaps the EventBus parameter this used to take (Phase 1) for
// settings::SettingsManager: every current handler is settings-related,
// and SettingsManager itself now owns publishing EventType::SettingsChanged
// (§2.2) — no handler touches EventBus directly anymore. Threading
// SettingsManager through here rather than a handler reaching for some
// global keeps its lifetime owned entirely by main.cpp, same as EventBus
// was.
//
// Phase 5 adds a platform::SelectionClipboardBridge& parameter for the two
// dev-only Debug* handlers (message_types.h's own comment on why they're
// named that way) — main.cpp owns the bridge's lifetime the same way it
// already owns settingsManager's.
void registerBuiltinHandlers(Dispatcher& dispatcher, settings::SettingsManager& settingsManager,
                              platform::SelectionClipboardBridge& selectionBridge);

} // namespace lancue::ipc
