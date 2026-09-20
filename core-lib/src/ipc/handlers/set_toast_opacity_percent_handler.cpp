#include "ipc/handlers/set_toast_opacity_percent_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"
#include "corelib/settings/settings_schema.h"

namespace lancue::ipc::handlers {

// Range is validated here, not just left to fromJson()'s own leniency —
// see kToastOpacityPercentMin/Max's own comment in settings_schema.h for
// where the bounds come from.
void registerSetToastOpacityPercentHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetToastOpacityPercent, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("percent") || !request.payload["percent"].is_number_integer()) {
                message.type = kError;
                message.payload = {{"reason", "SetToastOpacityPercent requires an integer 'percent' field"}};
                reply(std::move(message));
                return;
            }

            const int percent = request.payload["percent"].get<int>();
            if (percent < settings::kToastOpacityPercentMin || percent > settings::kToastOpacityPercentMax) {
                message.type = kError;
                message.payload = {{"reason", "SetToastOpacityPercent: 'percent' must be between " +
                                                   std::to_string(settings::kToastOpacityPercentMin) + " and " +
                                                   std::to_string(settings::kToastOpacityPercentMax)}};
                reply(std::move(message));
                return;
            }

            logInfo(QStringLiteral("SetToastOpacityPercent request received: %1%%").arg(percent));
            settingsManager.setToastOpacityPercent(percent);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetToastOpacityPercent}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
