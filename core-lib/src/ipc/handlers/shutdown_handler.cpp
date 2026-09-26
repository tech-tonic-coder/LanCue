#include "ipc/handlers/shutdown_handler.h"

#include <QCoreApplication>
#include <QTimer>

#include "corelib/ipc/message_types.h"

namespace lancue::ipc::handlers {

void registerShutdownHandler(Dispatcher& dispatcher) {
    dispatcher.registerHandler(kShutdown, [](const Message&, Dispatcher::ReplyCallback reply) {
        Message message;
        message.type = kAck;
        message.payload = {{"acknowledged", kShutdown}};
        reply(std::move(message));

        // Deferred to the next event-loop turn rather than calling
        // QCoreApplication::quit() right here: the Ack above still needs
        // this same event loop running a moment longer for IpcServer's
        // socket->write() to actually hand its bytes to the OS. A 0ms
        // QTimer::singleShot runs after the current call stack (and the
        // write it just queued) unwinds, same idea as Dispatcher's own
        // "never a nested event loop, always defer through the running
        // one" rule (see dispatcher.h).
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    });
}

} // namespace lancue::ipc::handlers
