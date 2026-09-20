#include "corelib/ipc/ipc_client.h"

#include "corelib/ipc/socket_name.h"

namespace lancue::ipc {

IpcClient::IpcClient(QObject* parent) : QObject(parent) {
    connect(&m_socket, &QLocalSocket::connected, this, &IpcClient::connected);
    connect(&m_socket, &QLocalSocket::readyRead, this, &IpcClient::onReadyRead);
    connect(&m_socket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError) {
        emit connectionFailed(m_socket.errorString());
    });
}

void IpcClient::connectToServer() {
    m_socket.connectToServer(socketName());
}

void IpcClient::send(const Message& message) {
    const std::string frame = frameMessage(message);
    m_socket.write(frame.data(), static_cast<qint64>(frame.size()));
}

void IpcClient::onReadyRead() {
    const QByteArray chunk = m_socket.readAll();
    const auto messages = m_reader.feed(std::string(chunk.constData(), static_cast<size_t>(chunk.size())));
    for (const auto& reply : messages) {
        emit replyReceived(reply);
    }
}

} // namespace lancue::ipc
