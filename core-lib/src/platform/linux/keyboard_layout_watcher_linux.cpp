#include "keyboard_layout_watcher_linux.h"

#include "corelib/logging/logger.h"

namespace lancue::platform::linux_ {

void KeyboardLayoutWatcherLinux::setCallback(LayoutChangedCallback callback) {
    m_callback = std::move(callback);
}

bool KeyboardLayoutWatcherLinux::start() {
    lancue::logError(QStringLiteral(
        "KeyboardLayoutWatcherLinux: not yet implemented — see the class comment for the planned XKB-based approach."));
    return false;
}

void KeyboardLayoutWatcherLinux::stop() {}

QString KeyboardLayoutWatcherLinux::currentLayout() const {
    return QStringLiteral("unknown");
}

} // namespace lancue::platform::linux_

namespace lancue::platform {

std::unique_ptr<IKeyboardLayoutWatcher> createKeyboardLayoutWatcher() {
    return std::make_unique<linux_::KeyboardLayoutWatcherLinux>();
}

} // namespace lancue::platform
