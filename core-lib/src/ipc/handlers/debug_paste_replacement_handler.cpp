#include "ipc/handlers/debug_paste_replacement_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Dev-only manual test of the paste half of the round trip — call after a
// DebugCaptureSelection so there's something for the resulting restore to
// restore, e.g.:
//   lancue-debug-cli DebugCaptureSelection
//   lancue-debug-cli DebugPasteReplacement '{"text":"replacement text"}'
//
// Uses SelectionClipboardBridge's async pasteReplacement(), not
// pasteReplacementBlocking() — same reasoning as
// debug_capture_selection_handler.cpp: this runs inside IpcServer's
// dispatch chain, where a nested event loop is unsafe on Windows.
void registerDebugPasteReplacementHandler(Dispatcher& dispatcher, platform::SelectionClipboardBridge& bridge) {
    dispatcher.registerHandler(
        kDebugPasteReplacement, [&bridge](const Message& request, Dispatcher::ReplyCallback reply) {
            if (!request.payload.contains("text") || !request.payload["text"].is_string()) {
                Message error;
                error.type = kError;
                error.payload = {{"reason", "DebugPasteReplacement requires a string 'text' field"}};
                reply(std::move(error));
                return;
            }

            const QString text = QString::fromStdString(request.payload["text"].get<std::string>());
            int restoreDelayMs = platform::SelectionClipboardBridge::kDefaultRestoreDelayMs;
            if (request.payload.contains("restoreDelayMs") &&
                request.payload["restoreDelayMs"].is_number_integer()) {
                restoreDelayMs = request.payload["restoreDelayMs"].get<int>();
            }

            logInfo(
                QStringLiteral("DebugPasteReplacement request received (restoreDelayMs=%1).").arg(restoreDelayMs));
            bridge.pasteReplacement(text, restoreDelayMs, [reply]() {
                Message message;
                message.type = kAck;
                message.payload = {{"acknowledged", kDebugPasteReplacement}};
                reply(std::move(message));
            });
        });
}

} // namespace lancue::ipc::handlers
