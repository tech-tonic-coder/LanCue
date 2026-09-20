#include <catch2/catch_test_macros.hpp>

#include "corelib/ui/app_theme_mode.h"

using namespace lancue::ui;

TEST_CASE("toCode()/fromCode() round-trip all three theme modes", "[ui]") {
    const AppThemeMode all[] = {
        AppThemeMode::Auto,
        AppThemeMode::Light,
        AppThemeMode::Dark,
    };

    for (const auto mode : all) {
        const QString code = toCode(mode);
        CAPTURE(code);
        CHECK_FALSE(code.isEmpty());
        CHECK(fromCode(code) == mode);
    }
}

TEST_CASE("AppThemeMode fromCode() is case-insensitive", "[ui]") {
    CHECK(fromCode(QStringLiteral("DARK")) == AppThemeMode::Dark);
    CHECK(fromCode(QStringLiteral("Light")) == AppThemeMode::Light);
}

TEST_CASE("fromCode() falls back to Auto for an unknown code", "[ui]") {
    CHECK(fromCode(QStringLiteral("nightmode")) == AppThemeMode::Auto);
    CHECK(fromCode(QStringLiteral("")) == AppThemeMode::Auto);
    CHECK(fromCode(QStringLiteral("xx"), AppThemeMode::Dark) == AppThemeMode::Dark);
}
