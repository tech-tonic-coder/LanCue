#include "corelib/ipc/dispatcher.h"

namespace lancue::ipc {

void Dispatcher::registerHandler(const std::string& type, Handler handler) {
    m_handlers[type] = std::move(handler);
}

void Dispatcher::dispatch(const Message& request, ReplyCallback reply) const {
    const auto it = m_handlers.find(request.type);
    if (it == m_handlers.end()) {
        Message error;
        error.type = "Error";
        error.payload = {{"reason", "unknown message type: " + request.type}};
        reply(std::move(error));
        return;
    }
    it->second(request, std::move(reply));
}

} // namespace lancue::ipc
