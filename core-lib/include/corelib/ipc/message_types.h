#pragma once

namespace lancue::ipc {

// String constants for known message `type` values. Centralized so a typo
// in a type string is a compile error at the call site rather than a
// silent "unknown message type" at runtime. Add one constant per new
// message type alongside its handler (§4.4) — this file never needs a
// switch/if over these, it's just names.
inline constexpr const char* kPing = "Ping";
inline constexpr const char* kPong = "Pong";
inline constexpr const char* kAck = "Ack";
inline constexpr const char* kError = "Error";

// Phase 1 stub covering the roadmap's own Phase 1 deliverable example
// ("settings asks core to change the toast duration"). Phase 4 rewires
// the handler behind this to go through SettingsManager (live-preview +
// persist-on-Save) instead of just republishing the raw value — see
// set_toast_duration_handler.cpp.
inline constexpr const char* kSetToastDuration = "SetToastDuration";
inline constexpr const char* kSetToastOpacityPercent = "SetToastOpacityPercent";

// Phase 4: settings persistence & live sync. One message type per
// settings field being changed, matching kSetToastDuration's own shape,
// rather than a single generic "SettingsChanged" message internally
// switching on a field name — keeps each handler a small, independently
// registered file per §4.4's dispatch-table philosophy. (An earlier,
// more generic "SettingsChanged" message type was reserved here in
// Phase 1 before this phase's actual design was worked out; superseded
// by the five names below — EventType::SettingsChanged, the EventBus
// event these handlers publish internally, is unrelated and unchanged.)
inline constexpr const char* kSetFollowerDuration = "SetFollowerDuration";
// Phase 8 (round 2): the follower's own opacity — see
// kFollowerOpacityPercentMin/Max's own comment in settings_schema.h.
inline constexpr const char* kSetFollowerOpacityPercent = "SetFollowerOpacityPercent";
inline constexpr const char* kSetHotkey = "SetHotkey";
inline constexpr const char* kRemoveHotkey = "RemoveHotkey";
inline constexpr const char* kSetLayoutConversionPairs = "SetLayoutConversionPairs";

// Phase 7: two more settings fields, same one-message-per-field pattern
// as the four above.
inline constexpr const char* kSetToastEnabled = "SetToastEnabled";
inline constexpr const char* kSetFollowerEnabled = "SetFollowerEnabled";
inline constexpr const char* kSetAppLanguage = "SetAppLanguage";

// Phase 7 (round 9): which of the 3x3 anchor positions
// (corelib/ui/toast_position.h) the toast is shown at — same one-
// message-per-field pattern as the two above.
inline constexpr const char* kSetToastPosition = "SetToastPosition";

// Phase 7 (round 10): which screen(s) the toast targets — same one-
// message-per-field pattern; two separate messages since
// toastMonitorMode and toastMonitorIndex are two separate fields (see
// SettingsManager::setToastMonitorMode()'s own comment on why they
// aren't combined into one message).
inline constexpr const char* kSetToastMonitorMode = "SetToastMonitorMode";
inline constexpr const char* kSetAppThemeMode = "SetAppThemeMode";
inline constexpr const char* kSetToastMonitorIndex = "SetToastMonitorIndex";

// Assigns one layout's badge/stripe swatch, per-layout like kSetHotkey
// rather than a whole-list replace.
inline constexpr const char* kSetLayoutColorAssignment = "SetLayoutColorAssignment";

// Returns the full current in-memory settings (payload: the same shape
// settings::toJson() produces) — lancue-settings uses this once on
// connect to seed its UI with whatever lancue-core currently holds,
// rather than assuming defaults or duplicating SettingsManager's state.
inline constexpr const char* kGetSettings = "GetSettings";

// Persists the current in-memory settings to disk. This is the only
// message that writes to the settings file — every Set* message above
// only updates the in-memory copy and live-previews via
// EventType::SettingsChanged (§2.2).
inline constexpr const char* kSaveSettings = "SaveSettings";

// Phase 5: dev-only messages exercising platform::SelectionClipboardBridge
// (§5 Phase 5's own testing guide calls for "a test hotkey/IPC call" —
// this is the IPC half, reusing tools/lancue-debug-cli's existing
// infrastructure rather than inventing a separate test harness). Named
// "Debug*" deliberately, unlike every message above: Phase 6 drives the
// same SelectionClipboardBridge directly from the hotkey-pressed handler,
// not through IPC, so these two exist only until Phase 6 lands and may be
// removed then rather than becoming permanent product surface.
inline constexpr const char* kDebugCaptureSelection = "DebugCaptureSelection";
inline constexpr const char* kDebugPasteReplacement = "DebugPasteReplacement";

// Phase 8 (integration-test hardening): asks lancue-core to quit its Qt
// event loop cleanly and exit. Added because QProcess::terminate() posts
// WM_CLOSE to the process's top-level windows on Windows, and lancue-core
// (a) has no visible top-level window most of the time (the toast only
// exists transiently) and (b) sets QApplication::setQuitOnLastWindowClosed
// (false) even when one does — so terminate() alone has nothing to act on
// and the process never exits from it. Replies Ack immediately, then quits
// on the next event-loop iteration (shutdown_handler.cpp) so the Ack has
// already been handed to the OS before the process exits.
inline constexpr const char* kShutdown = "Shutdown";

} // namespace lancue::ipc
