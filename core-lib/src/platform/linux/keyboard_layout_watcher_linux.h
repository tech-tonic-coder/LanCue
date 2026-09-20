#pragma once

#include "corelib/platform/IKeyboardLayoutWatcher.h"

namespace lancue::platform::linux_ {

// Not yet implemented (§4.5: handle unsupported/failed gracefully rather
// than silently no-op). The real mechanism, when this platform is
// actually being built and tested, is the XKB extension's state-notify
// events on X11 (XkbSelectEventDetails on XkbStateNotify, watching the
// group field) — event-driven via the X connection's own event queue,
// not polling. Wayland has no equivalent compositor-agnostic signal, so
// that path will likely need a per-compositor protocol or a portal API;
// left for whichever session actually needs it. Left as a stub for now
// since there's no Linux desktop machine in this project's current dev
// loop to verify either path against.
class KeyboardLayoutWatcherLinux final : public IKeyboardLayoutWatcher {
public:
    void setCallback(LayoutChangedCallback callback) override;
    bool start() override;
    void stop() override;
    QString currentLayout() const override;

private:
    LayoutChangedCallback m_callback;
};

} // namespace lancue::platform::linux_
