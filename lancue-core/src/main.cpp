#include <memory>

#ifndef _WIN32
#include <csignal>
#endif

#include <QApplication>

#include "corelib/eventbus/event_bus.h"
#include "corelib/ipc/dispatcher.h"
#include "corelib/ipc/ipc_server.h"
#include "corelib/ipc/registration.h"
#include "corelib/lifecycle/single_instance_guard.h"
#include "corelib/logging/logger.h"
#include "corelib/platform/IClipboardController.h"
#include "corelib/platform/IInputSimulator.h"
#include "corelib/platform/selection_clipboard_bridge.h"
#include "corelib/settings/settings_manager.h"
#include "conversion_feature/conversion_feature.h"
#include "feature_registry/feature_registry.h"
#include "follower_feature/follower_feature.h"
#include "global_hotkey_feature/global_hotkey_feature.h"
#include "layout_watcher_feature/layout_watcher_feature.h"
#include "mouse_watcher_feature/mouse_watcher_feature.h"
#include "toast_feature/toast_feature.h"
#include "ui/fonts.h"

// lancue-core's daemon lifecycle, established in Phase 1: acquire the
// single-instance lock, start logging (stdout is gone once this becomes a
// WIN32_EXECUTABLE on Windows — see CMakeLists.txt), open the IPC server,
// and run the Qt event loop.
//
// Phase 2 adds the FeatureRegistry that Phase 1's own comment here said
// would arrive "with the first real feature" — LayoutWatcherFeature is
// that feature. main.cpp only ever registers features and calls
// startAll()/stopAll(); it never touches a feature directly (§4.4).
//
// Phase 3 adds GlobalHotkeyFeature the same way — a second feature
// registered alongside the first, neither one touching the other.
//
// Phase 4 adds SettingsManager, loaded right after the EventBus exists
// (mutators publish through it) and before the Dispatcher/handlers or any
// feature are constructed, since both now depend on it.
//
// Phase 5 adds the clipboard controller + input simulator + the
// SelectionClipboardBridge that composes them, constructed before the
// Dispatcher since two of its handlers (Debug*, message_types.h's own
// comment on why they're dev-only) need a reference to the bridge. Not
// registered as an IFeature itself: it produces no events of its own and
// has nothing to start()/stop() beyond what it already does per-call.
//
// Phase 6 adds ConversionFeature, the EventBus-reacting feature that
// actually drives selectionBridge from real hotkey presses (subscribing
// to HotkeyPressed) — replacing Phase 5's own temporary diagnostic
// wiring that lived directly in this function.
//
// Phase 7 switches QCoreApplication to QApplication: the daemon now owns
// a real QWidget (the toast) rather than being purely headless, and
// QWidget construction requires QApplication specifically (QGuiApplication
// alone isn't enough — Widgets needs its own application object). Two
// consequences of that switch, both handled explicitly below rather than
// left to Qt's own defaults: setQuitOnLastWindowClosed(false), since a
// toast window closing (every single time one auto-dismisses) must never
// be mistaken for "the application should exit" the way it would for an
// app whose main window is its toast; and ui::loadApplicationFonts(),
// which must run before the first ToastWindow is ever constructed so
// Manrope/Vazirmatn are already registered by the time any QLabel asks
// for them. That first construction no longer happens inside
// ToastFeature::start() itself (round 10 made it lazy — see
// ToastFeature::ensureWindowCount()'s own comment for why), but loading
// fonts here, at the very start of main() before the event loop is even
// running, is still trivially early enough for whenever it does happen.
//
// Phase 8 adds MouseWatcherFeature (owns the OS-level mouse hook,
// publishes EventType::MouseMoved) and FollowerFeature (the window that
// actually reacts to it) — the same producer/consumer split
// LayoutWatcherFeature/ToastFeature already established. FollowerFeature
// is registered alongside ToastFeature for the same reason (see the
// comment just above registerFeature() below); MouseWatcherFeature's own
// position doesn't matter, since — unlike LayoutWatcherFeature — it
// never publishes a synchronous startup seed event.
int main(int argc, char* argv[]) {
#ifndef _WIN32
    // Must happen before anything touches a socket — IpcServer's very
    // first accepted connection is still a POSIX socket underneath
    // QLocalSocket on Linux/macOS. Without this, a client that exits
    // right after reading its reply (lancue-debug-cli's own normal
    // shutdown, or any other client doing the same) can race
    // IpcServer::onReadyRead()'s socket->write() and terminate this
    // entire daemon via the default SIGPIPE disposition — found during
    // this project's own concurrent-client stress-testing, unrelated to
    // any of Phase 5's clipboard-specific work (reproduces with plain
    // Ping requests too). Windows named pipes have no equivalent signal,
    // hence this whole block being POSIX-only.
    std::signal(SIGPIPE, SIG_IGN);
#endif

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("lancue-core"));
    QApplication::setQuitOnLastWindowClosed(false);

    lancue::ui::loadApplicationFonts();

    lancue::Logger::init(QStringLiteral("lancue-core"));
    lancue::logInfo(QStringLiteral("lancue-core starting"));

    lancue::SingleInstanceGuard guard(QStringLiteral("lancue-core.lock"));
    if (!guard.tryAcquire()) {
        lancue::logWarning(QStringLiteral("lancue-core: another instance is already running, exiting."));
        return 1;
    }

    lancue::EventBus eventBus;

    // LANCUE_SETTINGS_FILE_OVERRIDE, when set, is used verbatim as the
    // settings file path instead of the real per-OS QStandardPaths
    // location — this exists solely for the integration tests
    // (tests/integration/settings_roundtrip_test.cpp), which need a
    // throwaway settings file instead of this machine's real one.
    // Deliberately not done by having the tests override the
    // LOCALAPPDATA/XDG_CONFIG_HOME environment variable instead: on
    // Windows, QStandardPaths::AppConfigLocation resolves via the native
    // SHGetKnownFolderPath() API, which reads the real per-user profile
    // directly from the OS and ignores the LOCALAPPDATA environment
    // variable entirely, so that approach silently pointed every test
    // process at this developer machine's real settings.json instead of
    // an isolated one. qEnvironmentVariable() returns an empty string
    // when unset, which is exactly the "no override" sentinel
    // SettingsManager::filePath() already checks for.
    lancue::settings::SettingsManager settingsManager(eventBus, qEnvironmentVariable("LANCUE_SETTINGS_FILE_OVERRIDE"));
    settingsManager.load();

    auto clipboardController = lancue::platform::createClipboardController();
    auto inputSimulator = lancue::platform::createInputSimulator();
    lancue::platform::SelectionClipboardBridge selectionBridge(*clipboardController, *inputSimulator);

    lancue::ipc::Dispatcher dispatcher;
    lancue::ipc::registerBuiltinHandlers(dispatcher, settingsManager, selectionBridge);

    lancue::ipc::IpcServer server(dispatcher);
    if (!server.start()) {
        lancue::logError(QStringLiteral("lancue-core: failed to start the IPC server, exiting."));
        return 1;
    }

    lancue::FeatureRegistry featureRegistry;
    // ConversionFeature, ToastFeature, and FollowerFeature are all
    // registered (and therefore started) before LayoutWatcherFeature
    // deliberately: LayoutWatcherFeature::start() publishes an initial
    // EventType::LayoutChanged synchronously as its very last step (see
    // that feature's own comment), and all three must already be
    // subscribed for that one-time seed to reach them — a subscriber that
    // starts after the publish simply never sees it, same as any pub/sub
    // bus. GlobalHotkeyFeature's and MouseWatcherFeature's own positions
    // don't matter the same way: hotkey presses and mouse moves only ever
    // happen later from real user input, never synchronously during any
    // feature's start().
    featureRegistry.registerFeature(
        std::make_unique<lancue::ConversionFeature>(eventBus, settingsManager, selectionBridge));
    featureRegistry.registerFeature(std::make_unique<lancue::ToastFeature>(eventBus, settingsManager));
    featureRegistry.registerFeature(std::make_unique<lancue::FollowerFeature>(eventBus, settingsManager));
    featureRegistry.registerFeature(std::make_unique<lancue::LayoutWatcherFeature>(eventBus));
    featureRegistry.registerFeature(std::make_unique<lancue::GlobalHotkeyFeature>(eventBus, settingsManager));
    featureRegistry.registerFeature(std::make_unique<lancue::MouseWatcherFeature>(eventBus));
    featureRegistry.startAll();

    lancue::logInfo(QStringLiteral("lancue-core ready"));
    const int exitCode = app.exec();

    featureRegistry.stopAll();
    return exitCode;
}
