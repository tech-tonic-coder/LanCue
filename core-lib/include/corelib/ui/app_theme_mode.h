#pragma once

#include <QString>

namespace lancue::ui {

// Whether the toast (and later, every other window) follows the OS's
// own light/dark setting, or a mode the user picked explicitly.
enum class AppThemeMode {
    // Follow QStyleHints::colorScheme() — the only behavior before this
    // setting existed, kept as the default so an existing settings.json
    // behaves exactly as before.
    Auto,
    Light,
    Dark,
};

// "auto"/"light"/"dark" — the exact string persisted in
// Settings::appThemeMode.
QString toCode(AppThemeMode mode);

// Parses toCode()'s own output, falling back to `fallback` (Auto by
// default) for an unrecognized/empty code.
AppThemeMode fromCode(const QString& code, AppThemeMode fallback = AppThemeMode::Auto);

} // namespace lancue::ui
