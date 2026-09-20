#include "ipc/handlers/set_layout_conversion_pairs_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Replaces the whole list in one request (§ settings_manager.h's own
// comment on setLayoutConversionPairs() for why: Phase 9's UI submits the
// full edited list as one form, not one pair per round-trip).
void registerSetLayoutConversionPairsHandler(Dispatcher& dispatcher, settings::SettingsManager& settingsManager) {
    dispatcher.registerHandler(
        kSetLayoutConversionPairs, [&settingsManager](const Message& request, Dispatcher::ReplyCallback reply) {
            Message message;

            if (!request.payload.contains("pairs") || !request.payload["pairs"].is_array()) {
                message.type = kError;
                message.payload = {{"reason", "SetLayoutConversionPairs requires an array 'pairs' field"}};
                reply(std::move(message));
                return;
            }

            QVector<settings::ConversionPair> pairs;
            for (const auto& entry : request.payload["pairs"]) {
                if (!entry.is_object() || !entry.contains("layoutA") || !entry["layoutA"].is_string() ||
                    !entry.contains("layoutB") || !entry["layoutB"].is_string()) {
                    message.type = kError;
                    message.payload = {{"reason", "each entry in 'pairs' needs string 'layoutA'/'layoutB' fields"}};
                    reply(std::move(message));
                    return;
                }
                settings::ConversionPair pair;
                pair.layoutA = QString::fromStdString(entry["layoutA"].get<std::string>());
                pair.layoutB = QString::fromStdString(entry["layoutB"].get<std::string>());
                pairs.push_back(pair);
            }

            logInfo(QStringLiteral("SetLayoutConversionPairs request received: %1 pair(s)").arg(pairs.size()));
            settingsManager.setLayoutConversionPairs(pairs);

            message.type = kAck;
            message.payload = {{"acknowledged", kSetLayoutConversionPairs}};
            reply(std::move(message));
        });
}

} // namespace lancue::ipc::handlers
