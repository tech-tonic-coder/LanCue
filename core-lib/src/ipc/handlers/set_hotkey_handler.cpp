#include "ipc/handlers/set_hotkey_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Payload carries the two raw Qt enum ints, not a HotkeyCombo::toString()
// display string — that string form is explicitly documented as
// non-round-tripping (see IGlobalHotkeyManager.h), so the wire format
// matches settings::HotkeyBinding's own on-disk representation exactly.
// SettingsManager::setHotkey() publishes EventType::SettingsChanged
// itself, which GlobalHotkeyFeature is subscribed to and reconciles its
// live registration from — this handler never touches the platform
// hotkey manager directly (§2.2 platform isolation).
void registerSetHotkeyHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetHotkey, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("id") || !request.payload["id"].is_string() ||
                !request.payload.contains("modifiers") || !request.payload["modifiers"].is_number_integer() ||
                !request.payload.contains("key") || !request.payload["key"].is_number_integer()) {
                message.type = kError;
                message.payload = {
                    {"reason", "SetHotkey requires a string 'id' and integer 'modifiers'/'key' fields"}};
                reply(std::move(message));
                return;
            }

            const QString id = QString::fromStdString(request.payload["id"].get<std::string>());
            platform::HotkeyCombo combo;
            combo.modifiers = static_cast<Qt::KeyboardModifiers>(request.payload["modifiers"].get<int>());
            combo.key = static_cast<Qt::Key>(request.payload["key"].get<int>());

            logInfo(QStringLiteral("SetHotkey request received: id='%1', combo='%2'").arg(id, combo.toString()));
            settingsManager.setHotkey(id, combo);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetHotkey}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
