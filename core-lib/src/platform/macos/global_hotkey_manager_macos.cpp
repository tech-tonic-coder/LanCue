#include "global_hotkey_manager_macos.h"

#include "corelib/logging/logger.h"

namespace lancue::platform::macos {

void GlobalHotkeyManagerMacos::setCallback(HotkeyPressedCallback callback) {
    m_callback = std::move(callback);
}

bool GlobalHotkeyManagerMacos::registerHotkey(const HotkeyId& id, const HotkeyCombo& combo) {
    lancue::logError(
        QStringLiteral("GlobalHotkeyManagerMacos: not yet implemented (wanted '%1' for id='%2') — see the class "
                        "comment for the planned CGEventTap-based approach.")
            .arg(combo.toString(), id));
    return false;
}

void GlobalHotkeyManagerMacos::unregisterHotkey(const HotkeyId& /*id*/) {}

void GlobalHotkeyManagerMacos::stop() {}

} // namespace lancue::platform::macos

namespace lancue::platform {

std::unique_ptr<IGlobalHotkeyManager> createGlobalHotkeyManager() {
    return std::make_unique<macos::GlobalHotkeyManagerMacos>();
}

} // namespace lancue::platform
