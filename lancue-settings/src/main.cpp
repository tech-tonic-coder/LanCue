#include <QApplication>

#include "corelib/ipc/ipc_client.h"
#include "corelib/ipc/message_types.h"
#include "corelib/logging/logger.h"

// No tray icon or settings window yet (that's Phase 9) — this phase
// proves the full settings round trip over the real IPC transport:
// fetch the current settings, live-preview a toast-duration change,
// persist it, then fetch again to confirm the new value stuck. The
// hardcoded 3000ms value stands in for a UI control that doesn't exist
// yet, same as Phase 1's own version of this file.
namespace {

enum class Step { Connecting, GotInitialSettings, SetToastDuration, Saved, Confirmed };
Step g_step = Step::Connecting;

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("lancue-settings"));

    lancue::Logger::init(QStringLiteral("lancue-settings"));

    auto* client = new lancue::ipc::IpcClient(&app);

    QObject::connect(client, &lancue::ipc::IpcClient::connected, [client]() {
        lancue::logInfo(QStringLiteral("lancue-settings: connected to lancue-core"));

        lancue::ipc::Message request;
        request.type = lancue::ipc::kGetSettings;
        g_step = Step::Connecting;
        client->send(request);
    });

    QObject::connect(client, &lancue::ipc::IpcClient::replyReceived, [client](const lancue::ipc::Message& reply) {
        lancue::logInfo(QStringLiteral("lancue-settings: reply received, type=%1")
                             .arg(QString::fromStdString(reply.type)));

        switch (g_step) {
            case Step::Connecting: {
                lancue::logInfo(QStringLiteral("lancue-settings: current settings: %1")
                                     .arg(QString::fromStdString(reply.payload.dump())));

                lancue::ipc::Message setToast;
                setToast.type = lancue::ipc::kSetToastDuration;
                setToast.payload = {{"durationMs", 3000}};
                g_step = Step::GotInitialSettings;
                client->send(setToast);
                break;
            }
            case Step::GotInitialSettings: {
                lancue::ipc::Message save;
                save.type = lancue::ipc::kSaveSettings;
                g_step = Step::SetToastDuration;
                client->send(save);
                break;
            }
            case Step::SetToastDuration: {
                lancue::ipc::Message getAgain;
                getAgain.type = lancue::ipc::kGetSettings;
                g_step = Step::Saved;
                client->send(getAgain);
                break;
            }
            case Step::Saved: {
                lancue::logInfo(QStringLiteral("lancue-settings: settings after save: %1")
                                     .arg(QString::fromStdString(reply.payload.dump())));
                g_step = Step::Confirmed;
                QCoreApplication::quit();
                break;
            }
            case Step::Confirmed:
                break;
        }
    });

    QObject::connect(client, &lancue::ipc::IpcClient::connectionFailed, [](const QString& reason) {
        lancue::logError(
            QStringLiteral("lancue-settings: could not connect to lancue-core: %1. Is lancue-core running?")
                .arg(reason));
        QCoreApplication::quit();
    });

    client->connectToServer();

    return app.exec();
}
