#include "ipc/handlers/debug_capture_selection_handler.h"

#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc::handlers {

// Dev-only manual test of the copy half of the round trip (see
// message_types.h's own comment on why this is a "Debug*" message).
// Optional "timeoutMs" payload field overrides
// SelectionClipboardBridge::kDefaultCaptureTimeoutMs for exercising the
// timeout path itself without waiting the full default — e.g.
// `lancue-debug-cli DebugCaptureSelection '{"timeoutMs":50}'` against a
// window with nothing selected.
//
// Uses SelectionClipboardBridge's async captureSelection(), not
// captureSelectionBlocking() — this handler runs inside IpcServer's
// dispatch chain, and captureSelectionBlocking()'s own header comment
// explains exactly why a nested event loop there is unsafe on Windows.
// `reply` is stored inside the completion lambda and invoked whenever the
// capture actually finishes, which may be well after this handler
// function itself has returned — Dispatcher's ReplyCallback contract
// supports that by design (see dispatcher.h's own comment).
void registerDebugCaptureSelectionHandler(Dispatcher& dispatcher, platform::SelectionClipboardBridge& bridge) {
    dispatcher.registerHandler(
        kDebugCaptureSelection, [&bridge](const Message& request, Dispatcher::ReplyCallback reply) {
            int timeoutMs = platform::SelectionClipboardBridge::kDefaultCaptureTimeoutMs;
            if (request.payload.contains("timeoutMs") && request.payload["timeoutMs"].is_number_integer()) {
                timeoutMs = request.payload["timeoutMs"].get<int>();
            }

            logInfo(QStringLiteral("DebugCaptureSelection request received (timeoutMs=%1).").arg(timeoutMs));
            bridge.captureSelection(timeoutMs, [reply](platform::SelectionClipboardBridge::CaptureResult result) {
                Message message;
                message.type = kAck;
                message.payload = {{"acknowledged", kDebugCaptureSelection},
                                    {"success", result.success},
                                    {"text", result.text.toStdString()}};
                reply(std::move(message));
            });
        });
}

} // namespace lancue::ipc::handlers
