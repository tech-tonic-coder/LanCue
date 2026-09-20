#include "ipc/handlers/set_layout_color_assignment_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"
#include "corelib/ui/layout_color_palette.h"

namespace lancue::ipc::handlers {

// Validates against the palette range here since it's a fixed constant
// — unlike e.g. screen count, there's no reason to defer that check.
void registerSetLayoutColorAssignmentHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetLayoutColorAssignment, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("layoutId") || !request.payload["layoutId"].is_string() ||
                !request.payload.contains("swatchIndex") || !request.payload["swatchIndex"].is_number_integer()) {
                message.type = kError;
                message.payload = {
                    {"reason", "SetLayoutColorAssignment requires a string 'layoutId' and integer 'swatchIndex' field"}};
                reply(std::move(message));
                return;
            }

            const QString layoutId = QString::fromStdString(request.payload["layoutId"].get<std::string>());
            if (layoutId.trimmed().isEmpty()) {
                message.type = kError;
                message.payload = {{"reason", "SetLayoutColorAssignment: 'layoutId' must not be empty"}};
                reply(std::move(message));
                return;
            }

            const int swatchIndex = request.payload["swatchIndex"].get<int>();
            if (!ui::isValidLayoutColorSwatch(swatchIndex)) {
                message.type = kError;
                message.payload = {{"reason", "SetLayoutColorAssignment: 'swatchIndex' must be between " +
                                                   std::to_string(ui::kLayoutColorSwatchMin) + " and " +
                                                   std::to_string(ui::kLayoutColorSwatchMax)}};
                reply(std::move(message));
                return;
            }

            logInfo(QStringLiteral("SetLayoutColorAssignment request received: '%1' -> swatch %2")
                        .arg(layoutId)
                        .arg(swatchIndex));
            settingsManager.setLayoutColorAssignment(layoutId, swatchIndex);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetLayoutColorAssignment}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
