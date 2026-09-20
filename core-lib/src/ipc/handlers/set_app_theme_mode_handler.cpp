#include "ipc/handlers/set_app_theme_mode_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"
#include "corelib/ui/app_theme_mode.h"

namespace lancue::ipc::handlers {

// Sets whether the app follows the OS theme or a manual light/dark
// choice (corelib/ui/app_theme_mode.h). Same validation shape as
// set_toast_monitor_mode_handler.cpp — an unrecognized code stored as-is
// would silently fall back to Auto every time ui::fromCode() reads it
// back, so a typo gets an explicit error instead.
void registerSetAppThemeModeHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetAppThemeMode, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("mode") || !request.payload["mode"].is_string()) {
                message.type = kError;
                message.payload = {{"reason", "SetAppThemeMode requires a string 'mode' field"}};
                reply(std::move(message));
                return;
            }

            const QString code = QString::fromStdString(request.payload["mode"].get<std::string>());
            const bool known =
                ui::toCode(ui::fromCode(code, ui::AppThemeMode::Auto)).compare(code, Qt::CaseInsensitive) == 0;
            if (!known) {
                message.type = kError;
                message.payload = {{"reason", "SetAppThemeMode: unrecognized mode '" + code.toStdString() +
                                                   "' (expected one of auto, light, dark)"}};
                reply(std::move(message));
                return;
            }

            logInfo(QStringLiteral("SetAppThemeMode request received: %1").arg(code));
            settingsManager.setAppThemeMode(code);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetAppThemeMode}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
