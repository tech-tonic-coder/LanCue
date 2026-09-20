#include <catch2/catch_test_macros.hpp>

#include <QEventLoop>
#include <QStringList>
#include <QTimer>

#include "corelib/platform/layout_change_debouncer.h"

using lancue::platform::LayoutChangeDebouncer;

namespace {

// Runs the Qt event loop for `ms` so a QTimer's timeout has a chance to
// fire — needed because LayoutChangeDebouncer's settle notification comes
// through a real (single-shot) QTimer, not a synchronous call.
void pumpFor(int ms) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

} // namespace

TEST_CASE("LayoutChangeDebouncer collapses a burst into only the last value", "[layout][debounce]") {
    LayoutChangeDebouncer debouncer(30);
    QStringList settled;
    debouncer.setSettledCallback([&](const QString& id) { settled.append(id); });

    debouncer.onRawChange(QStringLiteral("en-us"));
    debouncer.onRawChange(QStringLiteral("fa-ir"));
    debouncer.onRawChange(QStringLiteral("en-us"));

    pumpFor(100);

    REQUIRE(settled.size() == 1);
    CHECK(settled.first() == QStringLiteral("en-us"));
}

TEST_CASE("LayoutChangeDebouncer does not re-fire for the same settled layout twice", "[layout][debounce]") {
    LayoutChangeDebouncer debouncer(20);
    int fireCount = 0;
    debouncer.setSettledCallback([&](const QString&) { ++fireCount; });

    debouncer.onRawChange(QStringLiteral("en-us"));
    pumpFor(60);
    debouncer.onRawChange(QStringLiteral("en-us"));
    pumpFor(60);

    CHECK(fireCount == 1);
}

TEST_CASE("LayoutChangeDebouncer fires again for a genuinely different layout", "[layout][debounce]") {
    LayoutChangeDebouncer debouncer(20);
    QStringList settled;
    debouncer.setSettledCallback([&](const QString& id) { settled.append(id); });

    debouncer.onRawChange(QStringLiteral("en-us"));
    pumpFor(60);
    debouncer.onRawChange(QStringLiteral("fa-ir"));
    pumpFor(60);

    REQUIRE(settled.size() == 2);
    CHECK(settled.last() == QStringLiteral("fa-ir"));
}
