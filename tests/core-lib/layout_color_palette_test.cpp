#include <catch2/catch_test_macros.hpp>

#include <QSet>

#include "corelib/ui/layout_color_palette.h"

using namespace lancue::ui;

TEST_CASE("isValidLayoutColorSwatch() accepts exactly 1-8", "[ui]") {
    CHECK_FALSE(isValidLayoutColorSwatch(0));
    for (int i = kLayoutColorSwatchMin; i <= kLayoutColorSwatchMax; ++i) {
        CHECK(isValidLayoutColorSwatch(i));
    }
    CHECK_FALSE(isValidLayoutColorSwatch(9));
    CHECK_FALSE(isValidLayoutColorSwatch(-1));
}

TEST_CASE("layoutColorSwatchHex() returns 8 distinct, well-formed hex colors", "[ui]") {
    QSet<QString> seen;
    for (int i = kLayoutColorSwatchMin; i <= kLayoutColorSwatchMax; ++i) {
        const QString hex = layoutColorSwatchHex(i);
        CAPTURE(i, hex);
        CHECK(hex.size() == 7);
        CHECK(hex.startsWith(QLatin1Char('#')));
        seen.insert(hex);
    }
    // No two swatches share a color.
    CHECK(seen.size() == kLayoutColorSwatchCount);
}

TEST_CASE("layoutColorSwatchHex() matches the roadmap's own table exactly", "[ui]") {
    CHECK(layoutColorSwatchHex(1) == QStringLiteral("#534AB7"));
    CHECK(layoutColorSwatchHex(2) == QStringLiteral("#185FA5"));
    CHECK(layoutColorSwatchHex(3) == QStringLiteral("#0F6E56"));
    CHECK(layoutColorSwatchHex(4) == QStringLiteral("#3B6D11"));
    CHECK(layoutColorSwatchHex(5) == QStringLiteral("#854F0B"));
    CHECK(layoutColorSwatchHex(6) == QStringLiteral("#993C1D"));
    CHECK(layoutColorSwatchHex(7) == QStringLiteral("#A32D2D"));
    CHECK(layoutColorSwatchHex(8) == QStringLiteral("#993556"));
}

TEST_CASE("layoutColorSwatchHex() clamps an out-of-range index defensively", "[ui]") {
    CHECK(layoutColorSwatchHex(9) == layoutColorSwatchHex(1));
    CHECK(layoutColorSwatchHex(16) == layoutColorSwatchHex(8));
    CHECK(layoutColorSwatchHex(0) == layoutColorSwatchHex(8));
    CHECK(layoutColorSwatchHex(-7) == layoutColorSwatchHex(1));
}
