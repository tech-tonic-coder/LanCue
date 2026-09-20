#include <catch2/catch_test_macros.hpp>

#include "corelib/ui/toast_position.h"

using namespace lancue::ui;

TEST_CASE("toCode()/fromCode() round-trip all nine positions", "[ui]") {
    const ToastPosition all[] = {
        ToastPosition::TopLeft,      ToastPosition::TopCenter,    ToastPosition::TopRight,
        ToastPosition::CenterLeft,   ToastPosition::Center,       ToastPosition::CenterRight,
        ToastPosition::BottomLeft,   ToastPosition::BottomCenter, ToastPosition::BottomRight,
    };

    for (const auto position : all) {
        const QString code = toCode(position);
        CAPTURE(code);
        CHECK_FALSE(code.isEmpty());
        CHECK(fromCode(code) == position);
    }
}

TEST_CASE("fromCode() is case-insensitive", "[ui]") {
    // Same leniency as i18n::fromCode() — see settings_schema.cpp's own
    // per-field fallback philosophy for why a hand-edited settings.json
    // shouldn't fail to parse over letter case.
    CHECK(fromCode(QStringLiteral("TOP-LEFT")) == ToastPosition::TopLeft);
    CHECK(fromCode(QStringLiteral("Bottom-Right")) == ToastPosition::BottomRight);
}

TEST_CASE("fromCode() falls back for an unknown code", "[ui]") {
    CHECK(fromCode(QStringLiteral("nowhere")) == ToastPosition::BottomRight);
    CHECK(fromCode(QStringLiteral("")) == ToastPosition::BottomRight);
    CHECK(fromCode(QStringLiteral("xx"), ToastPosition::Center) == ToastPosition::Center);
}
