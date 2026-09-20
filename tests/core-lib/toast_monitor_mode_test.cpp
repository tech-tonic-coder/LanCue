#include <catch2/catch_test_macros.hpp>

#include "corelib/ui/toast_monitor_mode.h"

using namespace lancue::ui;

TEST_CASE("toCode()/fromCode() round-trip all four monitor modes", "[ui]") {
    const ToastMonitorMode all[] = {
        ToastMonitorMode::Primary,
        ToastMonitorMode::All,
        ToastMonitorMode::Specific,
        ToastMonitorMode::CursorScreen,
    };

    for (const auto mode : all) {
        const QString code = toCode(mode);
        CAPTURE(code);
        CHECK_FALSE(code.isEmpty());
        CHECK(fromCode(code) == mode);
    }
}

TEST_CASE("ToastMonitorMode fromCode() is case-insensitive", "[ui]") {
    CHECK(fromCode(QStringLiteral("ALL")) == ToastMonitorMode::All);
    CHECK(fromCode(QStringLiteral("Primary")) == ToastMonitorMode::Primary);
}

TEST_CASE("fromCode() falls back to CursorScreen for an unknown code", "[ui]") {
    CHECK(fromCode(QStringLiteral("nowhere")) == ToastMonitorMode::CursorScreen);
    CHECK(fromCode(QStringLiteral("")) == ToastMonitorMode::CursorScreen);
    CHECK(fromCode(QStringLiteral("xx"), ToastMonitorMode::Primary) == ToastMonitorMode::Primary);
}
