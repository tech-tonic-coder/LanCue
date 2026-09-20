#include "ipc/handlers/set_follower_duration_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Mirrors set_toast_duration_handler.cpp exactly — Phase 8's real
// follower window doesn't exist yet either, so this phase's job is only
// to make the setting persist and live-preview correctly, same as toast.
void registerSetFollowerDurationHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetFollowerDuration, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("durationMs") || !request.payload["durationMs"].is_number_integer()) {
                message.type = kError;
                message.payload = {{"reason", "SetFollowerDuration requires an integer 'durationMs' field"}};
                reply(std::move(message));
                return;
            }

            const int durationMs = request.payload["durationMs"].get<int>();
            logInfo(QStringLiteral("SetFollowerDuration request received: %1ms").arg(durationMs));
            settingsManager.setFollowerDurationMs(durationMs);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetFollowerDuration}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
