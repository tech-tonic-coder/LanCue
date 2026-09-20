#include "ipc/handlers/set_toast_monitor_index_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Phase 7 (round 10): sets which screen "specific" monitor mode targets,
// by index into QGuiApplication::screens(). Only checked for being a
// non-negative integer here — core-lib doesn't link QtGui (see
// core-lib/CMakeLists.txt's own comment on why), so it has no way to
// know how many screens actually exist right now to bounds-check
// against; ToastFeature::resolveTargetScreens() falls back to the
// primary screen at show time if this index is out of range then (see
// that method's own comment, and kFieldToastMonitorIndex's in
// settings_schema.h).
void registerSetToastMonitorIndexHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetToastMonitorIndex, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("index") || !request.payload["index"].is_number_integer()) {
                message.type = kError;
                message.payload = {{"reason", "SetToastMonitorIndex requires an integer 'index' field"}};
                reply(std::move(message));
                return;
            }

            const int index = request.payload["index"].get<int>();
            if (index < 0) {
                message.type = kError;
                message.payload = {{"reason", "SetToastMonitorIndex: 'index' must be non-negative"}};
                reply(std::move(message));
                return;
            }

            logInfo(QStringLiteral("SetToastMonitorIndex request received: %1").arg(index));
            settingsManager.setToastMonitorIndex(index);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetToastMonitorIndex}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
