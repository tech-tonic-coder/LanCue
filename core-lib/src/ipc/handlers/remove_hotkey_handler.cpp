#include "ipc/handlers/remove_hotkey_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

void registerRemoveHotkeyHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kRemoveHotkey, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("id") || !request.payload["id"].is_string()) {
                message.type = kError;
                message.payload = {{"reason", "RemoveHotkey requires a string 'id' field"}};
                reply(std::move(message));
                return;
            }

            const QString id = QString::fromStdString(request.payload["id"].get<std::string>());
            logInfo(QStringLiteral("RemoveHotkey request received: id='%1'").arg(id));
            settingsManager.removeHotkey(id);

            message.type = kAck;
            message.payload = {{"acknowledged", kRemoveHotkey}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
