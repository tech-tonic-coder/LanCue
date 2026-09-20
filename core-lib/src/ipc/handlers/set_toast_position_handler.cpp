#include "ipc/handlers/set_toast_position_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"
#include "corelib/ui/toast_position.h"

namespace lancue::ipc::handlers {

// Phase 7 (round 9): sets where on the target monitor the toast is
// anchored (corelib/ui/toast_position.h's 3x3 grid). Validated against
// ui::fromCode()'s known set the same way set_app_language_handler.cpp
// validates language codes — an unrecognized position stored as-is would
// silently fall back to BottomRight every time ui::fromCode() reads it
// back (that function's own designed leniency, meant for an old/corrupt
// settings file), so a live client sending a typo gets an explicit error
// instead of a silent, unannounced fallback.
void registerSetToastPositionHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetToastPosition, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("position") || !request.payload["position"].is_string()) {
                message.type = kError;
                message.payload = {{"reason", "SetToastPosition requires a string 'position' field"}};
                reply(std::move(message));
                return;
            }

            const QString code = QString::fromStdString(request.payload["position"].get<std::string>());
            const bool known = ui::toCode(ui::fromCode(code, ui::ToastPosition::BottomRight))
                                    .compare(code, Qt::CaseInsensitive) == 0;
            if (!known) {
                message.type = kError;
                message.payload = {
                    {"reason", "SetToastPosition: unrecognized position '" + code.toStdString() +
                                   "' (expected one of top-left, top-center, top-right, center-left, center, "
                                   "center-right, bottom-left, bottom-center, bottom-right)"}};
                reply(std::move(message));
                return;
            }

            logInfo(QStringLiteral("SetToastPosition request received: %1").arg(code));
            settingsManager.setToastPosition(code);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetToastPosition}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
