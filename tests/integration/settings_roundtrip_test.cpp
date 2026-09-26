#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QLocalSocket>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QThread>

#include "corelib/ipc/frame_reader.h"
#include "corelib/ipc/message.h"
#include "corelib/ipc/message_types.h"
#include "corelib/ipc/socket_name.h"
#include "corelib/settings/settings_schema.h"

namespace {

// Points lancue-core's settings file at a throwaway temp path instead of
// this developer machine's real one — this test writes a real settings
// file to disk on purpose (that's the point), and it must not be the same
// file `lancue-core` would use outside of a test run.
//
// This used to override LOCALAPPDATA/XDG_CONFIG_HOME and rely on
// QStandardPaths::AppConfigLocation picking it up. Confirmed on real
// Windows hardware (2026-09-25) that this silently does nothing on
// Windows: QStandardPaths::AppConfigLocation resolves via the native
// SHGetKnownFolderPath() API there, which reads the real per-user profile
// directly from the OS and never looks at the LOCALAPPDATA environment
// variable — so every test process was actually reading/writing this
// developer machine's real settings.json the whole time, silently leaking
// state between test runs (see the Phase 8 Learnings entry). Replaced
// with LANCUE_SETTINGS_FILE_OVERRIDE (main.cpp), which lancue-core uses
// verbatim as the settings file path, bypassing QStandardPaths entirely —
// works identically on every OS, so the old Linux-only XDG_CONFIG_HOME
// branch is gone too.
QProcessEnvironment isolatedConfigEnvironment(const QString& configDir) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LANCUE_SETTINGS_FILE_OVERRIDE"), configDir + QStringLiteral("/lancue-core/settings.json"));
    return env;
}

QString findSettingsJson(const QString& rootDir) {
    QDirIterator it(rootDir, QStringList{QStringLiteral("settings.json")}, QDir::Files, QDirIterator::Subdirectories);
    if (it.hasNext()) {
        return it.next();
    }
    return QString();
}

bool waitForConnection(QLocalSocket& socket) {
    for (int attempt = 0; attempt < 50; ++attempt) {
        socket.connectToServer(lancue::ipc::socketName());
        if (socket.waitForConnected(200)) {
            return true;
        }
        socket.abort();
        QThread::msleep(100);
    }
    return false;
}

lancue::ipc::Message sendAndWait(QLocalSocket& socket, const lancue::ipc::Message& request) {
    const std::string frame = lancue::ipc::frameMessage(request);
    socket.write(frame.data(), static_cast<qint64>(frame.size()));
    REQUIRE(socket.waitForReadyRead(2000));
    const QByteArray chunk = socket.readAll();

    lancue::ipc::FrameReader reader;
    const auto messages = reader.feed(std::string(chunk.constData(), static_cast<size_t>(chunk.size())));
    REQUIRE(messages.size() == 1);
    return messages[0];
}

// Asks lancue-core to shut down over IPC (kShutdown) instead of
// QProcess::terminate(): on Windows, terminate() posts WM_CLOSE to the
// process's top-level windows, but lancue-core usually has none open (the
// toast is transient) and sets setQuitOnLastWindowClosed(false) even when
// one exists, so terminate() alone never made it exit — see the Phase 8
// Learnings entry.
void shutdownAndWait(QLocalSocket& socket, QProcess& process) {
    lancue::ipc::Message shutdown;
    shutdown.type = lancue::ipc::kShutdown;
    const lancue::ipc::Message reply = sendAndWait(socket, shutdown);
    CHECK(reply.type == lancue::ipc::kAck);
    socket.disconnectFromServer();
    REQUIRE(process.waitForFinished(3000));
}

} // namespace

TEST_CASE("settings live-preview over IPC, persist on SaveSettings, survive a restart", "[integration][settings]") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    QTemporaryDir configDir;
    REQUIRE(configDir.isValid());
    const QProcessEnvironment env = isolatedConfigEnvironment(configDir.path());

    QProcess coreProcess;
    coreProcess.setProcessEnvironment(env);
    coreProcess.start(QString::fromUtf8(LANCUE_CORE_EXECUTABLE));
    REQUIRE(coreProcess.waitForStarted(5000));

    QLocalSocket socket;
    REQUIRE(waitForConnection(socket));

    // 1) Fresh process, no file written yet -> GetSettings should reflect
    // defaultSettings() exactly.
    lancue::ipc::Message getSettings;
    getSettings.type = lancue::ipc::kGetSettings;
    lancue::ipc::Message reply = sendAndWait(socket, getSettings);
    REQUIRE(reply.type == lancue::ipc::kAck);
    CHECK(reply.payload.value(lancue::settings::kFieldToastDuration, -1) == lancue::settings::kDefaultToastDurationMs);

    // 2) SetToastDuration live-previews immediately...
    lancue::ipc::Message setToast;
    setToast.type = lancue::ipc::kSetToastDuration;
    setToast.payload = {{"durationMs", 9999}};
    reply = sendAndWait(socket, setToast);
    REQUIRE(reply.type == lancue::ipc::kAck);

    reply = sendAndWait(socket, getSettings);
    CHECK(reply.payload.value(lancue::settings::kFieldToastDuration, -1) == 9999);

    // ...but nothing is written to disk yet — this is the "preview live,
    // persist on demand" guarantee (§2.2), checked here the same way a
    // developer would: looking at the actual file on disk.
    CHECK(findSettingsJson(configDir.path()).isEmpty());

    // 2b) SetLayoutColorAssignment over the real socket, same as above.
    lancue::ipc::Message setLayoutColor;
    setLayoutColor.type = lancue::ipc::kSetLayoutColorAssignment;
    setLayoutColor.payload = {{"layoutId", "fa-ir"}, {"swatchIndex", 2}};
    reply = sendAndWait(socket, setLayoutColor);
    REQUIRE(reply.type == lancue::ipc::kAck);

    reply = sendAndWait(socket, getSettings);
    REQUIRE(reply.payload.contains(lancue::settings::kFieldLayoutColorAssignments));
    CHECK(reply.payload[lancue::settings::kFieldLayoutColorAssignments].value("fa-ir", -1) == 2);

    // An out-of-palette-range swatch is rejected immediately, not
    // silently accepted (set_layout_color_assignment_handler.cpp).
    lancue::ipc::Message setInvalidLayoutColor;
    setInvalidLayoutColor.type = lancue::ipc::kSetLayoutColorAssignment;
    setInvalidLayoutColor.payload = {{"layoutId", "fa-ir"}, {"swatchIndex", 99}};
    reply = sendAndWait(socket, setInvalidLayoutColor);
    CHECK(reply.type == lancue::ipc::kError);

    // 3) SaveSettings writes it.
    lancue::ipc::Message save;
    save.type = lancue::ipc::kSaveSettings;
    reply = sendAndWait(socket, save);
    REQUIRE(reply.type == lancue::ipc::kAck);

    const QString settingsPath = findSettingsJson(configDir.path());
    REQUIRE_FALSE(settingsPath.isEmpty());

    QFile file(settingsPath);
    REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QByteArray fileBytes = file.readAll();
    file.close();
    const auto onDisk = nlohmann::json::parse(fileBytes.constData(), fileBytes.constData() + fileBytes.size());
    CHECK(onDisk.value(lancue::settings::kFieldToastDuration, -1) == 9999);
    CHECK(onDisk[lancue::settings::kFieldLayoutColorAssignments].value("fa-ir", -1) == 2);

    shutdownAndWait(socket, coreProcess);

    // 4) A second, fresh process pointed at the same config directory
    // should load exactly what the first one saved — the real point of
    // this phase (§5's own "restart and confirm settings reloaded
    // correctly" checklist item).
    QProcess secondCoreProcess;
    secondCoreProcess.setProcessEnvironment(env);
    secondCoreProcess.start(QString::fromUtf8(LANCUE_CORE_EXECUTABLE));
    REQUIRE(secondCoreProcess.waitForStarted(5000));

    QLocalSocket secondSocket;
    REQUIRE(waitForConnection(secondSocket));

    reply = sendAndWait(secondSocket, getSettings);
    REQUIRE(reply.type == lancue::ipc::kAck);
    CHECK(reply.payload.value(lancue::settings::kFieldToastDuration, -1) == 9999);
    CHECK(reply.payload[lancue::settings::kFieldLayoutColorAssignments].value("fa-ir", -1) == 2);

    shutdownAndWait(secondSocket, secondCoreProcess);
}

TEST_CASE("a corrupt settings file is replaced in memory by defaults, not propagated", "[integration][settings]") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    QTemporaryDir configDir;
    REQUIRE(configDir.isValid());

    // Pre-seed a garbage settings.json exactly where lancue-core will
    // look for it (mirroring QStandardPaths::AppConfigLocation's own
    // "<config>/<applicationName>" layout for the applicationName main.cpp
    // sets — "lancue-core").
    const QString appConfigDir = configDir.path() + QStringLiteral("/lancue-core");
    QDir().mkpath(appConfigDir);
    QFile corruptFile(appConfigDir + QStringLiteral("/settings.json"));
    REQUIRE(corruptFile.open(QIODevice::WriteOnly | QIODevice::Text));
    corruptFile.write("not valid json at all {{{");
    corruptFile.close();

    QProcess coreProcess;
    coreProcess.setProcessEnvironment(isolatedConfigEnvironment(configDir.path()));
    coreProcess.start(QString::fromUtf8(LANCUE_CORE_EXECUTABLE));
    // The daemon must not crash on a corrupt config file (§5 Phase 4)
    // — waitForStarted() alone doesn't prove that, so this also confirms
    // it's still alive and answering IPC a moment later, below.
    REQUIRE(coreProcess.waitForStarted(5000));

    QLocalSocket socket;
    REQUIRE(waitForConnection(socket));

    lancue::ipc::Message getSettings;
    getSettings.type = lancue::ipc::kGetSettings;
    const lancue::ipc::Message reply = sendAndWait(socket, getSettings);

    REQUIRE(reply.type == lancue::ipc::kAck);
    CHECK(reply.payload.value(lancue::settings::kFieldToastDuration, -1) == lancue::settings::kDefaultToastDurationMs);

    shutdownAndWait(socket, coreProcess);
}
