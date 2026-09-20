#include "ipc/handlers/save_settings_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// The only handler that writes to disk (§2.2) — every Set* handler above
// only ever touches the in-memory copy.
void registerSaveSettingsHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(kSaveSettings, [&settingsManager](const Message&, Dispatcher::ReplyCallback reply) {
        Message message;
        if (!settingsManager.save()) {
            message.type = kError;
            message.payload = {{"reason", "failed to write the settings file, see the lancue-core log"}};
            reply(std::move(message));
            return;
        }

        logInfo(QStringLiteral("SaveSettings request received and persisted."));
        message.type = kAck;
        message.payload = {{"acknowledged", kSaveSettings}};
        reply(std::move(message));
    });
}

} // namespace lancue::ipc::handlers
