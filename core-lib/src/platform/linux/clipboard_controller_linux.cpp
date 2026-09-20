#include "clipboard_controller_linux.h"

#include "corelib/logging/logger.h"

namespace lancue::platform::linux_ {

void ClipboardControllerLinux::setCallback(ClipboardChangedCallback callback) {
    m_callback = std::move(callback);
}

bool ClipboardControllerLinux::start() {
    lancue::logError(QStringLiteral("ClipboardControllerLinux: not yet implemented — see the class comment for "
                                     "the planned X11/XFixes (or Wayland data-control) approach."));
    return false;
}

void ClipboardControllerLinux::stop() {}

bool ClipboardControllerLinux::hasText() const {
    return false;
}

std::optional<QString> ClipboardControllerLinux::readText() const {
    return std::nullopt;
}

bool ClipboardControllerLinux::writeText(const QString& /*text*/) {
    return false;
}

} // namespace lancue::platform::linux_

namespace lancue::platform {

std::unique_ptr<IClipboardController> createClipboardController() {
    return std::make_unique<linux_::ClipboardControllerLinux>();
}

} // namespace lancue::platform
