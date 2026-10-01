#include <cstdio>

#include <QCoreApplication>
#include <QLocalSocket>

#include "corelib/ipc/frame_reader.h"
#include "corelib/ipc/message.h"
#include "corelib/ipc/message_types.h"
#include "corelib/ipc/socket_name.h"

// Dev-only manual testing aid, not part of the shipped product: sends one
// IPC message to a running lancue-core and prints the reply, so a message
// type like SetHotkey/RemoveHotkey can be exercised by hand from the
// command line before Phase 9's real settings UI exists to do it through
// a GUI. Deliberately synchronous/blocking (a single request, wait for
// the one reply, exit) — no need for the async signal/slot style
// lancue-settings/main.cpp uses for its multi-step demo flow, since this
// tool only ever sends one message per invocation.
//
// Usage:
//   lancue-debug-cli <MessageType> [JSON payload]
//
// Examples:
//   lancue-debug-cli GetSettings
//   lancue-debug-cli SetHotkey "{\"id\":\"convertAuto\",\"modifiers\":201326592,\"key\":75}"
//   lancue-debug-cli RemoveHotkey "{\"id\":\"convertAuto\"}"
//   lancue-debug-cli SaveSettings
//   lancue-debug-cli DebugCaptureSelection
//   lancue-debug-cli DebugCaptureSelection "{\"timeoutMs\":50}"
//   lancue-debug-cli DebugPasteReplacement "{\"text\":\"replacement text\"}"
//   lancue-debug-cli SetToastEnabled "{\"enabled\":false}"
//   lancue-debug-cli SetAppLanguage "{\"language\":\"fa\"}"
//   lancue-debug-cli SetToastPosition "{\"position\":\"top-left\"}"
//   lancue-debug-cli SetToastMonitorMode "{\"mode\":\"all\"}"
//   lancue-debug-cli SetAppThemeMode "{\"mode\":\"dark\"}"
//   lancue-debug-cli SetToastMonitorIndex "{\"index\":1}"
//   lancue-debug-cli SetLayoutColorAssignment "{\"layoutId\":\"fa-ir\",\"swatchIndex\":2}"
//   lancue-debug-cli SetToastOpacityPercent "{\"percent\":75}"
//
// From PowerShell specifically: the two examples above (backslash-escaped
// JSON) can fail to parse — PowerShell's own argument-quoting doesn't
// reliably hand a literal `\"` through to this tool's argv the way
// cmd.exe/bash do (worse as of PowerShell 7.3+; see the Phase 7
// Learnings entry). From PowerShell, use the `--%` stop-parsing token
// instead:
//   lancue-debug-cli.exe --% SetAppLanguage "{\"language\":\"fa\"}"
// or build the JSON via PowerShell itself:
//   $json = (@{language='fa'} | ConvertTo-Json -Compress); lancue-debug-cli.exe SetAppLanguage $json
// or just run this tool from a plain cmd.exe window instead.
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        std::fprintf(stderr, "usage: lancue-debug-cli <MessageType> [JSON payload]\n");
        return 2;
    }

    lancue::ipc::Message request;
    request.type = argv[1];

    if (argc >= 3) {
        try {
            request.payload = nlohmann::json::parse(argv[2]);
        } catch (const nlohmann::json::parse_error& e) {
            std::fprintf(stderr, "invalid JSON payload: %s\n", e.what());
            return 2;
        }
    }

    QLocalSocket socket;
    socket.connectToServer(lancue::ipc::socketName());
    // 15s, not the more typical 2s: a real GitHub Actions Windows run
    // (round 8) showed a freshly Start-Process'd lancue-core.exe can take
    // noticeably longer than 2s to reach IpcServer::start() on a cold
    // launch — this tool's own CI smoke-test usage hit that directly.
    // One bounded blocking wait, not a retry loop: when lancue-core is
    // already running (this tool's normal interactive use case),
    // waitForConnected() returns as soon as connected, so this only ever
    // costs the extra time when something is actually slow to start.
    if (!socket.waitForConnected(15000)) {
        std::fprintf(stderr, "could not connect to lancue-core: %s. Is it running?\n",
                      qPrintable(socket.errorString()));
        return 1;
    }

    const std::string frame = lancue::ipc::frameMessage(request);
    socket.write(frame.data(), static_cast<qint64>(frame.size()));
    if (!socket.waitForBytesWritten(3000)) {
        std::fprintf(stderr, "failed to send request: %s (state=%d)\n", qPrintable(socket.errorString()),
                      static_cast<int>(socket.state()));
        return 1;
    }

    if (!socket.waitForReadyRead(5000)) {
        // Reports the actual QLocalSocket error/state rather than
        // assuming this was a genuine 5-second timeout — it also returns
        // false immediately if the connection was closed/reset before the
        // full 5s elapsed, which looks identical to a real timeout unless
        // this is printed. Needed to tell those two cases apart, since a
        // real timeout and an early disconnect have very different causes
        // (and, for an early disconnect, the socket's error() and
        // errorString() below pinpoint which QLocalSocket-level condition
        // fired).
        std::fprintf(stderr, "did not receive a reply: %s (error=%d, state=%d)\n",
                      qPrintable(socket.errorString()), static_cast<int>(socket.error()),
                      static_cast<int>(socket.state()));
        return 1;
    }

    const QByteArray chunk = socket.readAll();
    lancue::ipc::FrameReader reader;
    const auto messages = reader.feed(std::string(chunk.constData(), static_cast<size_t>(chunk.size())));

    if (messages.empty()) {
        std::fprintf(stderr, "no complete reply frame received\n");
        return 1;
    }

    const lancue::ipc::Message& reply = messages.front();
    std::printf("type: %s\n", reply.type.c_str());
    std::printf("payload: %s\n", reply.payload.dump(2).c_str());

    return reply.type == lancue::ipc::kError ? 1 : 0;
}
