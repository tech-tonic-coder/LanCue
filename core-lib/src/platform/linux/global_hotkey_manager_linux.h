#pragma once

#include "corelib/platform/IGlobalHotkeyManager.h"

namespace lancue::platform::linux_ {

// Not yet implemented (§4.5: handle unsupported/failed gracefully rather
// than silently no-op). The real mechanism, when this platform is
// actually being built and tested, is X11's XGrabKey on the root window
// for each registered combo, with its own id<->combo tracking mirroring
// GlobalHotkeyManagerWindows's approach (XGrabKey doesn't hand back an
// OS-assigned integer id the way RegisterHotKey does — X delivers
// KeyPress events carrying the raw keycode+modifiers grabbed, so a real
// implementation would map that back to the HotkeyId itself, similar to
// the Windows class's winId<->HotkeyId maps but keyed on keycode+
// modifiers instead). Wayland has no compositor-agnostic equivalent
// (same limitation Phase 2 documented for layout-change notification): a
// compositor generally must broker global shortcuts itself via a portal
// API (e.g. the GlobalShortcuts portal), which is a fundamentally
// different integration than XGrabKey and would need its own
// implementation, not a fallback path inside this class. Left as a stub
// for now since there's no Linux desktop machine in this project's
// current dev loop to verify either path against.
class GlobalHotkeyManagerLinux final : public IGlobalHotkeyManager {
public:
    void setCallback(HotkeyPressedCallback callback) override;
    bool registerHotkey(const HotkeyId& id, const HotkeyCombo& combo) override;
    void unregisterHotkey(const HotkeyId& id) override;
    void stop() override;

private:
    HotkeyPressedCallback m_callback;
};

} // namespace lancue::platform::linux_
