#include "keyboard_layout_watcher_macos.h"

#include "corelib/logging/logger.h"

namespace lancue::platform::macos {

void KeyboardLayoutWatcherMacos::setCallback(LayoutChangedCallback callback) {
    m_callback = std::move(callback);
}

bool KeyboardLayoutWatcherMacos::start() {
    lancue::logError(QStringLiteral(
        "KeyboardLayoutWatcherMacos: not yet implemented — see the class comment for the planned TIS-based approach."));
    return false;
}

void KeyboardLayoutWatcherMacos::stop() {}

QString KeyboardLayoutWatcherMacos::currentLayout() const {
    return QStringLiteral("unknown");
}

} // namespace lancue::platform::macos

namespace lancue::platform {

std::unique_ptr<IKeyboardLayoutWatcher> createKeyboardLayoutWatcher() {
    return std::make_unique<macos::KeyboardLayoutWatcherMacos>();
}

} // namespace lancue::platform
