#include "ipc/handlers/set_follower_opacity_percent_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"
#include "corelib/settings/settings_schema.h"

namespace lancue::ipc::handlers {

// Mirrors registerSetToastOpacityPercentHandler() exactly — see that
// file's own comment; the two fields are independently validated and
// stored (kFollowerOpacityPercentMin/Max, not the toast's), per Mahdi's
// own request that each window's opacity be separately settable.
void registerSetFollowerOpacityPercentHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetFollowerOpacityPercent, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("percent") || !request.payload["percent"].is_number_integer()) {
                message.type = kError;
                message.payload = {{"reason", "SetFollowerOpacityPercent requires an integer 'percent' field"}};
                reply(std::move(message));
                return;
            }

            const int percent = request.payload["percent"].get<int>();
            if (percent < settings::kFollowerOpacityPercentMin || percent > settings::kFollowerOpacityPercentMax) {
                message.type = kError;
                message.payload = {{"reason", "SetFollowerOpacityPercent: 'percent' must be between " +
                                                   std::to_string(settings::kFollowerOpacityPercentMin) + " and " +
                                                   std::to_string(settings::kFollowerOpacityPercentMax)}};
                reply(std::move(message));
                return;
            }

            logInfo(QStringLiteral("SetFollowerOpacityPercent request received: %1%%").arg(percent));
            settingsManager.setFollowerOpacityPercent(percent);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetFollowerOpacityPercent}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
