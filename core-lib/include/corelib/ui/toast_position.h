#pragma once

#include <QString>

namespace lancue::ui {

// Where on the target monitor the toast notification is anchored. A
// standard 3x3 anchor grid — the same shape most OS notification
// systems, presentation tools, and window-snap pickers already use —
// rather than inventing a smaller/different set of named positions.
//
// Independent of app language / RTL: this is a screen position, not UI
// content, so (matching the reasoning already documented in
// ToastWindow::applyMetricsAndPosition() for the old hardcoded
// bottom-right default) it does not mirror for Persian.
enum class ToastPosition {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
};

// "top-left"/"top-center"/.../"bottom-right" — the exact string persisted
// in Settings::toastPosition (settings_schema.h) and round-tripped
// through settings.json, kept as plain kebab-case names for the same
// human-readable-on-disk reasoning as i18n::AppLanguage's "en"/"fa" codes.
QString toCode(ToastPosition position);

// Parses toCode()'s own output. An unrecognized/empty code (an
// old/corrupt settings file, or one from a future LanCue version with a
// position this binary doesn't know about) falls back to `fallback`
// (BottomRight by default, matching this feature's original hardcoded
// behavior before positions were configurable) rather than failing the
// whole settings load over one field — same lenient-per-field philosophy
// as every other settings_schema.cpp field.
ToastPosition fromCode(const QString& code, ToastPosition fallback = ToastPosition::BottomRight);

} // namespace lancue::ui
