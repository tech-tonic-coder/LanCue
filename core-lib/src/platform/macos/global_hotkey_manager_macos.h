#pragma once

#include "corelib/platform/IGlobalHotkeyManager.h"

namespace lancue::platform::macos {

// Not yet implemented (§4.5: handle unsupported/failed gracefully rather
// than silently no-op). The real mechanism, when this platform is
// actually being built and tested, is a CGEventTap watching for whichever
// modifier+key combinations are currently registered system-wide, with
// its own id<->combo tracking mirroring GlobalHotkeyManagerWindows's
// approach (see that class's own comment on why an id<->integer mapping
// is needed there; CGEventTap doesn't have an equivalent OS-assigned
// integer id at all, so a real implementation would generate its own).
// This is the same approach this phase's own plan (§5 Phase 3) calls
// for, and the standard mechanism real macOS hotkey utilities use, since
// Carbon's RegisterEventHotKey is deprecated and CGEventTap is what
// replaced it. Requires the process to have Accessibility (or Input
// Monitoring) permission granted in System Settings — note this
// explicitly in the real implementation's start-up failure path so a
// denied-permission failure doesn't look identical to "combination
// already in use" (this stub's own failure reason doesn't need to
// distinguish these yet, but a real implementation will). Left as a stub
// for now rather than a partial/untested implementation, since there's
// no macOS machine in this project's current dev loop to verify it
// against.
class GlobalHotkeyManagerMacos final : public IGlobalHotkeyManager {
public:
    void setCallback(HotkeyPressedCallback callback) override;
    bool registerHotkey(const HotkeyId& id, const HotkeyCombo& combo) override;
    void unregisterHotkey(const HotkeyId& id) override;
    void stop() override;

private:
    HotkeyPressedCallback m_callback;
};

} // namespace lancue::platform::macos
