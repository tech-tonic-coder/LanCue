#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QLocalSocket>
#include <QProcess>
#include <QThread>

#include "corelib/ipc/frame_reader.h"
#include "corelib/ipc/message.h"
#include "corelib/ipc/message_types.h"
#include "corelib/ipc/socket_name.h"

// Exercises the real cross-process path: launches the actual lancue-core
// binary and connects to it exactly the way lancue-settings does, rather
// than an in-process stub server.
TEST_CASE("a client can Ping a real lancue-core process and get Pong back", "[integration]") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    QProcess coreProcess;
    coreProcess.start(QString::fromUtf8(LANCUE_CORE_EXECUTABLE));
    REQUIRE(coreProcess.waitForStarted(5000));

    // lancue-core needs a moment to acquire its lock and start listening.
    // Retrying the connection (instead of one fixed sleep) avoids making
    // this test flaky against a startup time that isn't guaranteed.
    QLocalSocket socket;
    bool connected = false;
    for (int attempt = 0; attempt < 50 && !connected; ++attempt) {
        socket.connectToServer(lancue::ipc::socketName());
        connected = socket.waitForConnected(200);
        if (!connected) {
            socket.abort();
            QThread::msleep(100);
        }
    }
    REQUIRE(connected);

    lancue::ipc::Message ping;
    ping.type = lancue::ipc::kPing;
    const std::string frame = lancue::ipc::frameMessage(ping);
    socket.write(frame.data(), static_cast<qint64>(frame.size()));

    REQUIRE(socket.waitForReadyRead(2000));
    const QByteArray chunk = socket.readAll();

    lancue::ipc::FrameReader reader;
    const auto messages = reader.feed(std::string(chunk.constData(), static_cast<size_t>(chunk.size())));

    REQUIRE(messages.size() == 1);
    CHECK(messages[0].type == lancue::ipc::kPong);

    // kShutdown, not QProcess::terminate(): on Windows, terminate() posts
    // WM_CLOSE to the process's top-level windows, but lancue-core
    // usually has none open (the toast is transient) and sets
    // setQuitOnLastWindowClosed(false) even when one exists, so
    // terminate() alone never made it exit — see the Phase 8 Learnings
    // entry.
    lancue::ipc::Message shutdown;
    shutdown.type = lancue::ipc::kShutdown;
    const std::string shutdownFrame = lancue::ipc::frameMessage(shutdown);
    socket.write(shutdownFrame.data(), static_cast<qint64>(shutdownFrame.size()));
    REQUIRE(socket.waitForReadyRead(2000));
    socket.disconnectFromServer();
    REQUIRE(coreProcess.waitForFinished(3000));
}
