#include "clipboard_controller_macos.h"

#include "corelib/logging/logger.h"

namespace lancue::platform::macos {

void ClipboardControllerMacos::setCallback(ClipboardChangedCallback callback) {
    m_callback = std::move(callback);
}

bool ClipboardControllerMacos::start() {
    lancue::logError(QStringLiteral("ClipboardControllerMacos: not yet implemented — see the class comment for "
                                     "the planned NSPasteboard-based approach."));
    return false;
}

void ClipboardControllerMacos::stop() {}

bool ClipboardControllerMacos::hasText() const {
    return false;
}

std::optional<QString> ClipboardControllerMacos::readText() const {
    return std::nullopt;
}

bool ClipboardControllerMacos::writeText(const QString& /*text*/) {
    return false;
}

} // namespace lancue::platform::macos

namespace lancue::platform {

std::unique_ptr<IClipboardController> createClipboardController() {
    return std::make_unique<macos::ClipboardControllerMacos>();
}

} // namespace lancue::platform
