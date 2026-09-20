#include "ipc/handlers/set_toast_duration_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Phase 7 (the real toast window) doesn't exist yet, so there's nothing to
// actually resize here. Phase 4 upgrades this from a bare EventBus
// republish (Phase 1) to going through SettingsManager: the new duration
// is live-previewed immediately (SettingsManager::setToastDurationMs()
// publishes EventType::SettingsChanged itself) but only reaches disk once
// SaveSettings is sent — see §2.2's "preview live, persist on demand".
void registerSetToastDurationHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetToastDuration, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("durationMs") || !request.payload["durationMs"].is_number_integer()) {
                message.type = kError;
                message.payload = {{"reason", "SetToastDuration requires an integer 'durationMs' field"}};
                reply(std::move(message));
                return;
            }

            const int durationMs = request.payload["durationMs"].get<int>();
            logInfo(QStringLiteral("SetToastDuration request received: %1ms").arg(durationMs));
            settingsManager.setToastDurationMs(durationMs);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetToastDuration}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
