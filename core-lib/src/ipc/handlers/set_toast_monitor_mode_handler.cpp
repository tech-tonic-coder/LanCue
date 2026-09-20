#include "ipc/handlers/set_toast_monitor_mode_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"
#include "corelib/ui/toast_monitor_mode.h"

namespace lancue::ipc::handlers {

// Phase 7 (round 10): sets which screen(s) the toast targets
// (corelib/ui/toast_monitor_mode.h). Validated against ui::fromCode()'s
// known set the same way set_toast_position_handler.cpp validates
// positions — an unrecognized mode stored as-is would silently fall back
// to CursorScreen every time ui::fromCode() reads it back, so a live
// client sending a typo gets an explicit error instead.
void registerSetToastMonitorModeHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetToastMonitorMode, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("mode") || !request.payload["mode"].is_string()) {
                message.type = kError;
                message.payload = {{"reason", "SetToastMonitorMode requires a string 'mode' field"}};
                reply(std::move(message));
                return;
            }

            const QString code = QString::fromStdString(request.payload["mode"].get<std::string>());
            const bool known = ui::toCode(ui::fromCode(code, ui::ToastMonitorMode::CursorScreen))
                                    .compare(code, Qt::CaseInsensitive) == 0;
            if (!known) {
                message.type = kError;
                message.payload = {{"reason", "SetToastMonitorMode: unrecognized mode '" + code.toStdString() +
                                                   "' (expected one of primary, all, specific, cursor)"}};
                reply(std::move(message));
                return;
            }

            logInfo(QStringLiteral("SetToastMonitorMode request received: %1").arg(code));
            settingsManager.setToastMonitorMode(code);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetToastMonitorMode}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
