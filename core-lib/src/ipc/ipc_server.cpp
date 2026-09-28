#include "corelib/ipc/ipc_server.h"

#include <QLocalSocket>
#include <QPointer>

#include "corelib/ipc/socket_name.h"
#include "corelib/logging/logger.h"

namespace lancue::ipc {

IpcServer::IpcServer(Dispatcher& dispatcher, QObject* parent)
    : QObject(parent), m_dispatcher(dispatcher) {
    connect(&m_server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);
}

bool IpcServer::start() {
    // A previous crashed instance can leave a stale socket/pipe name
    // registered on some platforms; removeServer() clears that before
    // listen() so a crash doesn't permanently block the daemon from
    // restarting. No-op if nothing stale is there.
    QLocalServer::removeServer(socketName());

    if (!m_server.listen(socketName())) {
        logError(QStringLiteral("IpcServer: failed to listen on '%1': %2")
                     .arg(socketName(), m_server.errorString()));
        return false;
    }
    logInfo(QStringLiteral("IpcServer: listening on '%1'").arg(socketName()));
    return true;
}

void IpcServer::stop() {
    m_server.close();
}

void IpcServer::onNewConnection() {
    while (QLocalSocket* socket = m_server.nextPendingConnection()) {
        m_readers.insert(socket, FrameReader{});
        connect(socket, &QLocalSocket::readyRead, this, &IpcServer::onReadyRead);
        connect(socket, &QLocalSocket::disconnected, this, &IpcServer::onDisconnected);
    }
}

void IpcServer::onReadyRead() {
    auto* rawSocket = qobject_cast<QLocalSocket*>(sender());
    if (!rawSocket) {
        return;
    }

    // QPointer, not a raw pointer: the ReplyCallback below can be invoked
    // later, from this same top-level event loop, after this function has
    // already returned (any handler that genuinely needs to wait on
    // something — Phase 5's clipboard capture — does exactly this). By
    // then the connection may have legitimately closed, and QPointer lets
    // the callback detect that instead of writing to a stale QObject.
    QPointer<QLocalSocket> socket = rawSocket;

    const QByteArray chunk = rawSocket->readAll();
    auto it = m_readers.find(rawSocket);
    if (it == m_readers.end()) {
        return;
    }

    const auto messages = it.value().feed(std::string(chunk.constData(), static_cast<size_t>(chunk.size())));
    for (const auto& request : messages) {
        const std::string requestType = request.type;
        m_dispatcher.dispatch(request, [socket, requestType](Message reply) {
            if (!socket) {
                logWarning(QStringLiteral("IpcServer: connection closed before a reply to '%1' could be sent; "
                                           "dropping it.")
                               .arg(QString::fromStdString(requestType)));
                return;
            }
            const std::string frame = frameMessage(reply);
            socket->write(frame.data(), static_cast<qint64>(frame.size()));

            // Pumps the event loop until Qt has actually handed these
            // bytes to the OS (write() alone only queues them in Qt's own
            // buffer) — added because kShutdown's handler queues
            // QCoreApplication::quit() for the very next event-loop turn,
            // and without this, that turn could arrive before Qt's socket
            // notifier ever got a chance to flush this reply, so the
            // client's waitForReadyRead() timed out even though the
            // handler had already replied. Confirmed on a real GitHub
            // Actions Linux run (not just this sandbox) that the 0ms timer
            // alone is not a hard guarantee. A bounded timeout rather than
            // an unbounded wait, so a client that stops reading can never
            // hang the daemon on a reply it will never pick up.
            socket->waitForBytesWritten(1000);
        });
    }
}

void IpcServer::onDisconnected() {
    auto* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) {
        return;
    }
    // Logged at INFO rather than removed: a normal client disconnect
    // (lancue-debug-cli exiting after reading its reply, e.g.) shows up
    // here too, not just abnormal ones, so this is routine connection-
    // lifecycle visibility, not an error condition by itself.
    logInfo(QStringLiteral("IpcServer: connection disconnected (error=%1, errorString='%2').")
                .arg(static_cast<int>(socket->error()))
                .arg(socket->errorString()));
    m_readers.remove(socket);
    socket->deleteLater();
}

} // namespace lancue::ipc
