// Custom Catch2 entry point (replacing Catch2::Catch2WithMain) — Phase 2
// introduces the project's first QTimer-based logic
// (LayoutChangeDebouncer), and QTimer::timeout only ever fires while a Qt
// event loop is running, which requires a QCoreApplication to exist.
// Earlier tests (EventBus, dispatcher, message framing) are all
// synchronous and never needed this; this file is additive infra that
// later phases' own timers (Phase 7's toast auto-dismiss, Phase 8's mouse
// debounce) can reuse rather than each reinventing it.
#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>

#include <QCoreApplication>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    return Catch::Session().run(argc, argv);
}
