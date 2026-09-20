#pragma once

#include "corelib/platform/IKeyboardLayoutWatcher.h"

namespace lancue::platform::macos {

// Not yet implemented (§4.5: handle unsupported/failed gracefully rather
// than silently no-op). The real mechanism, when this platform is
// actually being built and tested, is TIS input-source change
// notifications (kTISNotifySelectedKeyboardInputSourceChanged via
// CFNotificationCenter) — not polling — matching the event-driven
// approach the Windows implementation already uses. Left as a stub for
// now rather than a partial/untested implementation, since there's no
// macOS machine in this project's current dev loop to verify it against.
class KeyboardLayoutWatcherMacos final : public IKeyboardLayoutWatcher {
public:
    void setCallback(LayoutChangedCallback callback) override;
    bool start() override;
    void stop() override;
    QString currentLayout() const override;

private:
    LayoutChangedCallback m_callback;
};

} // namespace lancue::platform::macos
