#pragma once

#include <QLocalSocket>
#include <QObject>

#include "corelib/ipc/frame_reader.h"
#include "corelib/ipc/message.h"

namespace lancue::ipc {

// The lancue-settings side of the transport. Thin wrapper around
// QLocalSocket that adds the same length-prefixed framing IpcServer uses,
// so message.h's frame format is the only thing either side needs to agree
// on — the socket type itself is symmetric (a QLocalSocket connecting to a
// QLocalServer has no client-vs-server protocol difference beyond who
// calls listen() vs connectToServer()).
class IpcClient : public QObject {
    Q_OBJECT

public:
    explicit IpcClient(QObject* parent = nullptr);

    void connectToServer();
    void send(const Message& message);

signals:
    void connected();
    void replyReceived(const Message& reply);
    void connectionFailed(const QString& reason);

private slots:
    void onReadyRead();

private:
    QLocalSocket m_socket;
    FrameReader m_reader;
};

} // namespace lancue::ipc
