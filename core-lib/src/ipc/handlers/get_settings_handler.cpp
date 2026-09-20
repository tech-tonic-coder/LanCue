#include "ipc/handlers/get_settings_handler.h"

#include "corelib/ipc/message_types.h"

namespace lancue::ipc::handlers {

// No payload required — returns whatever SettingsManager currently holds
// in memory (which may include unsaved live-preview changes, by design:
// a settings UI reconnecting mid-edit should see the same in-progress
// state lancue-core is already acting on, not the stale on-disk copy).
void registerGetSettingsHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(kGetSettings, [&settingsManager](const Message&, Dispatcher::ReplyCallback reply) {
        Message message;
        message.type = kAck;
        message.payload = settings::toJson(settingsManager.current());
        reply(std::move(message));
    });
}

} // namespace lancue::ipc::handlers
