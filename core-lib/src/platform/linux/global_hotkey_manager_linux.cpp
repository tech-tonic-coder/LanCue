#include "global_hotkey_manager_linux.h"

#include "corelib/logging/logger.h"

namespace lancue::platform::linux_ {

void GlobalHotkeyManagerLinux::setCallback(HotkeyPressedCallback callback) {
    m_callback = std::move(callback);
}

bool GlobalHotkeyManagerLinux::registerHotkey(const HotkeyId& id, const HotkeyCombo& combo) {
    lancue::logError(
        QStringLiteral("GlobalHotkeyManagerLinux: not yet implemented (wanted '%1' for id='%2') — see the class "
                        "comment for the planned XGrabKey-based approach.")
            .arg(combo.toString(), id));
    return false;
}

void GlobalHotkeyManagerLinux::unregisterHotkey(const HotkeyId& /*id*/) {}

void GlobalHotkeyManagerLinux::stop() {}

} // namespace lancue::platform::linux_

namespace lancue::platform {

std::unique_ptr<IGlobalHotkeyManager> createGlobalHotkeyManager() {
    return std::make_unique<linux_::GlobalHotkeyManagerLinux>();
}

} // namespace lancue::platform
