#include <catch2/catch_test_macros.hpp>

#include "corelib/platform/IGlobalHotkeyManager.h"

using lancue::platform::HotkeyCombo;

TEST_CASE("HotkeyCombo equality compares modifiers and key", "[hotkey]") {
    const HotkeyCombo a{Qt::ControlModifier | Qt::AltModifier, Qt::Key_L};
    const HotkeyCombo sameCombo{Qt::ControlModifier | Qt::AltModifier, Qt::Key_L};
    const HotkeyCombo differentKey{Qt::ControlModifier | Qt::AltModifier, Qt::Key_K};
    const HotkeyCombo differentModifiers{Qt::ControlModifier, Qt::Key_L};

    CHECK(a == sameCombo);
    CHECK(a != differentKey);
    CHECK(a != differentModifiers);
}

TEST_CASE("HotkeyCombo::toString formats modifiers in a stable order", "[hotkey]") {
    const HotkeyCombo combo{Qt::ControlModifier | Qt::AltModifier, Qt::Key_L};
    CHECK(combo.toString() == QStringLiteral("Ctrl+Alt+L"));
}

TEST_CASE("HotkeyCombo::toString covers digits, function keys, and named keys", "[hotkey]") {
    CHECK(HotkeyCombo{Qt::ShiftModifier, Qt::Key_5}.toString() == QStringLiteral("Shift+5"));
    CHECK(HotkeyCombo{Qt::NoModifier, Qt::Key_F5}.toString() == QStringLiteral("F5"));
    CHECK(HotkeyCombo{Qt::MetaModifier, Qt::Key_Space}.toString() == QStringLiteral("Win+Space"));
}

TEST_CASE("the default conversion hotkey combos are well-formed", "[hotkey]") {
    // Not asserting specific combos here — only that the defaults real
    // code paths (settings::defaultSettings(), ConversionFeature) rely on
    // are real keys, not the sentinel Qt::Key_unknown, since that would
    // make every platform manager reject them as "unsupported key" at
    // startup. Renamed/expanded in Phase 6 from the single
    // kDefaultHotkeyCombo this test used to check, to the multi-hotkey
    // scheme's three combos.
    CHECK(lancue::platform::kDefaultConvertAutoHotkeyCombo.key != Qt::Key_unknown);
    CHECK(lancue::platform::kDefaultConvertToEnUsHotkeyCombo.key != Qt::Key_unknown);
    CHECK(lancue::platform::kDefaultConvertToFaIrHotkeyCombo.key != Qt::Key_unknown);
}
