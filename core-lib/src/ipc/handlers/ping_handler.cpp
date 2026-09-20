#include "ipc/handlers/ping_handler.h"

#include "corelib/ipc/message_types.h"

namespace lancue::ipc::handlers {

void registerPingHandler(Dispatcher& dispatcher) {
    dispatcher.registerHandler(kPing, [](const Message&, Dispatcher::ReplyCallback reply) {
        Message message;
        message.type = kPong;
        reply(std::move(message));
    });
}

} // namespace lancue::ipc::handlers
