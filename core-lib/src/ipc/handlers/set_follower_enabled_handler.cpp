#include "ipc/handlers/set_follower_enabled_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Mirrors registerSetToastEnabledHandler() exactly — see that file's own
// comment. Lets a test/debug client (lancue-debug-cli today; Phase 9's
// settings UI later) flip whether a keyboard-layout change shows the
// follower, live-previewed the same as every other Set* handler (§2.2).
// FollowerFeature's own SettingsChanged subscription immediately hides
// an already-visible follower when this turns false, the same way
// ToastFeature's does for toastEnabled — added together after Mahdi's
// own question about exactly this interaction with a permanent-duration
// window.
void registerSetFollowerEnabledHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetFollowerEnabled, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("enabled") || !request.payload["enabled"].is_boolean()) {
                message.type = kError;
                message.payload = {{"reason", "SetFollowerEnabled requires a boolean 'enabled' field"}};
                reply(std::move(message));
                return;
            }

            const bool enabled = request.payload["enabled"].get<bool>();
            logInfo(QStringLiteral("SetFollowerEnabled request received: %1").arg(enabled ? "true" : "false"));
            settingsManager.setFollowerEnabled(enabled);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetFollowerEnabled}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
