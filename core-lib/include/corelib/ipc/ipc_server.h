#pragma once

#include <QHash>
#include <QLocalServer>
#include <QObject>

#include "corelib/ipc/dispatcher.h"
#include "corelib/ipc/frame_reader.h"

class QLocalSocket;

namespace lancue::ipc {

// The lancue-core side of the transport: listens on a well-known
// QLocalServer name (named pipe on Windows, Unix domain socket on
// macOS/Linux — see socket_name.h for why QLocalSocket/Server was picked
// over rolling a per-OS transport by hand) and routes every complete
// incoming frame through the given Dispatcher, writing the handler's
// reply back on the same connection. Entirely signal-driven — there is no
// polling loop anywhere in this class, per §4.7.
class IpcServer : public QObject {
    Q_OBJECT

public:
    explicit IpcServer(Dispatcher& dispatcher, QObject* parent = nullptr);

    // Returns false if the server name could not be claimed. Independent
    // of SingleInstanceGuard on purpose — see the roadmap Learnings entry
    // for why a stale QLocalServer name is cleared here rather than folded
    // into the lock-file guard.
    bool start();
    void stop();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    Dispatcher& m_dispatcher;
    QLocalServer m_server;
    QHash<QLocalSocket*, FrameReader> m_readers;
};

} // namespace lancue::ipc
