#include "ipc/handlers/set_toast_enabled_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Phase 7: lets a test/debug client (lancue-debug-cli today; Phase 9's
// settings UI later) flip whether a keyboard-layout change shows the
// toast, live-previewed the same as every other Set* handler (§2.2).
void registerSetToastEnabledHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetToastEnabled, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("enabled") || !request.payload["enabled"].is_boolean()) {
                message.type = kError;
                message.payload = {{"reason", "SetToastEnabled requires a boolean 'enabled' field"}};
                reply(std::move(message));
                return;
            }

            const bool enabled = request.payload["enabled"].get<bool>();
            logInfo(QStringLiteral("SetToastEnabled request received: %1").arg(enabled ? "true" : "false"));
            settingsManager.setToastEnabled(enabled);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetToastEnabled}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
